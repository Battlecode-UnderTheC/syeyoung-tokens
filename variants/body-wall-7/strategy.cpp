#include "strategy.hpp"

#include <algorithm>
#include <cmath>
#include <tuple>
#include <functional>
#ifdef BW7_TUNE
#include <cstdlib>
#include <string>
#endif
#include <unordered_map>
#include <unordered_set>
#ifdef BW7_DEBUG
#include <atomic>
#include <cstdio>
#include <cstdlib>
#endif

namespace bw7 {
#ifdef BW7_DEBUG
static long g_escape = 0, g_turnout = 0, g_calls = 0;
#define BW7_COUNT(x) ++(x)
// idle-loop measurement: turns where the last 12 heads cover <= 6 tiles with nothing eaten
static std::atomic<long> g_turns{0}, g_idle{0}, g_eaten{0}, g_cross{0}, g_bans{0};
static struct IdleReport {
  ~IdleReport() {
    if (g_turns) std::fprintf(stderr, "BW7 turns %ld idle %ld (%.2f%%) eaten %ld crossings %ld bans %ld\n", g_turns.load(),
                              g_idle.load(), 100.0 * g_idle / g_turns, g_eaten.load(), g_cross.load(), g_bans.load());
  }
} g_idle_report;
#else
#define BW7_COUNT(x)
#endif

StrategyAgent::StrategyAgent(const rl::Spawn& spawn, Params params)
    : p_(params), spawn_(spawn), board_(spawn.width, spawn.height), portal_(spawn.width * spawn.height, 0) {
#ifdef BW7_TUNE
  // BW7_P="name=value,..." overrides parameters (tuning builds only)
  if (const char* env = std::getenv("BW7_P")) {
    std::string all(env);
    size_t pos = 0;
    while (pos < all.size()) {
      size_t end = all.find(',', pos);
      if (end == std::string::npos) end = all.size();
      const std::string kv = all.substr(pos, end - pos);
      pos = end + 1;
      const size_t eq = kv.find('=');
      if (eq == std::string::npos) continue;
      const std::string k = kv.substr(0, eq);
      const double v = std::atof(kv.c_str() + eq + 1);
#define BW7_SET(name) if (k == #name) p_.name = static_cast<decltype(p_.name)>(v);
      BW7_SET(split_min_length) BW7_SET(split_until_round) BW7_SET(feed_from_round) BW7_SET(wall_until) BW7_SET(trap_cost)
      BW7_SET(room_margin) BW7_SET(room_min) BW7_SET(escape_slack) BW7_SET(escape_min) BW7_SET(check_split)
      BW7_SET(liberties) BW7_SET(share_factor) BW7_SET(patrol) BW7_SET(hunt_radius) BW7_SET(explore_value)
      BW7_SET(escape_steps) BW7_SET(trap_small)
      BW7_SET(split_headroom) BW7_SET(turnout_budget) BW7_SET(doom_first) BW7_SET(ally_memory) BW7_SET(avoid_blind) BW7_SET(doomed_split_room) BW7_SET(leader_split_until) BW7_SET(share_leader) BW7_SET(feed_distance) BW7_SET(big_split_length)
      BW7_SET(portal_cross) BW7_SET(portal_explore) BW7_SET(loop_break) BW7_SET(loop_ban)
#undef BW7_SET
    }
  }
#endif
}

// ---- what this dragon sees ------------------------------------------------------------------------

void StrategyAgent::Remember(const rl::Observation& o) {
  board_.Update(o);
  head_ = board_.Tile(o.head_x(), o.head_y());
  length_ = o.length;
  units_ = o.unit_count;
  round_ = o.round;
  if (first_turn_) {
    leader_ = o.round == 0;
    waller_ = !leader_ && p_.waller_mod > 0 && spawn_.id % p_.waller_mod == 0;
    born_ = o.round;                         // alive at the start: not a split child
    first_turn_ = false;
    for (const rl::Segment& s : o.segments)         // the part of the body in view, head to tail
      if (s.id == spawn_.id) body_.push_back(board_.Tile(s.x, s.y));
  }
  if (body_.empty() || body_.front() != head_) body_.push_front(head_);
  while (static_cast<int32_t>(body_.size()) > length_) body_.pop_back();
  std::unordered_map<int32_t, int32_t> segments;
  bodies_.clear();
  // dragons whose last segment in view is on the window's border: their body may go on out of view
  std::unordered_map<int32_t, bool> cut;
  auto on_border = [&](const rl::Segment& s) {
    int32_t dx = std::abs(s.x - o.head_x()), dy = std::abs(s.y - o.head_y());
    dx = std::min(dx, board_.width() - dx), dy = std::min(dy, board_.height() - dy);
    return dx >= 3 || dy >= 3;
  };
  for (const rl::Segment& s : o.segments) {
    if (s.id == spawn_.id) continue;
    ++segments[s.id];
    cut[s.id] = on_border(s);
    bodies_[s.id].push_back(board_.Tile(s.x, s.y));
    Sighting& w = seen_[s.id];
    w.team = s.team;
    w.round = round_;
    if (s.head) w.head = board_.Tile(s.x, s.y);
  }
  for (const auto& [id, n] : segments) seen_[id].length = n;
  for (const uint64_t w : o.inbox) Decode(w);
  // allies in view: their head position now (their length only if the whole body is in view:
  // otherwise keep the reported one)
  for (const auto& [id, n] : segments) {
    const Sighting& w = seen_[id];
    if (w.team != spawn_.team || w.head < 0 || w.round != round_) continue;
    Report& have = reports_[id];
    const bool fresh = have.id >= 0 && round_ - have.round <= p_.report_memory;
    have.id = id;
    have.tile = w.head;
    have.length = fresh && (cut[id] || !p_.trust_visible) ? std::max(have.length, n) : n;
    have.round = round_;
  }
  reports_[spawn_.id] = Report{spawn_.id, head_, length_, round_};
  // portal edges in view (both tiles beside each)
  {
    constexpr int R = dragons::kVisionRadius, V = dragons::kVision;
    const int32_t hx = o.head_x(), hy = o.head_y();
    for (int r = 0; r <= V; ++r)
      for (int c = 0; c < V; ++c)
        if (o.hedge[r * V + c] >= 0) {   // north side of (x, y)
          portal_[board_.Tile(hx - R + c, hy - R + r)] |= 1 << 0;
          portal_[board_.Tile(hx - R + c, hy - R + r - 1)] |= 1 << 2;
        }
    for (int r = 0; r < V; ++r)
      for (int c = 0; c <= V; ++c)
        if (o.vedge[r * (V + 1) + c] >= 0) {   // west side of (x, y)
          portal_[board_.Tile(hx - R + c, hy - R + r)] |= 1 << 3;
          portal_[board_.Tile(hx - R + c - 1, hy - R + r)] |= 1 << 1;
        }
  }
  // last turn's ping through an unknown portal beside our new head: clean (no dragon) echo?
  exit_clear_ = false;
  if (ping_round_ == round_ - 1 && ping_tile_ == head_ && o.echo_present) {
    bool any = false;
    for (int k = 1; k < 5; ++k) any |= o.echo[k] != "0";   // ally, ally head, enemy, enemy head
    exit_clear_ = !any;
  }
  // the loop breaker's history
  if (length_ > last_length_ && round_ > born_) last_eat_ = round_;
  last_length_ = length_;
  recent_.push_back(head_);
  while (static_cast<int32_t>(recent_.size()) > p_.loop_window) recent_.pop_front();
}

bool StrategyAgent::Looping() const {
  if (static_cast<int32_t>(recent_.size()) < p_.loop_window || round_ - last_eat_ < p_.loop_window) return false;
  std::vector<int32_t> v(recent_.begin(), recent_.end());
  std::sort(v.begin(), v.end());
  return std::unique(v.begin(), v.end()) - v.begin() <= p_.loop_distinct;
}

bool StrategyAgent::BodyInView() const {
  if (static_cast<int32_t>(body_.size()) < length_) return false;
  for (const int32_t t : body_)
    if (!board_.Current(t)) return false;
  return true;
}

bool StrategyAgent::Mine(int32_t t) const { return std::find(body_.begin(), body_.end(), t) != body_.end(); }

bool StrategyAgent::Occupied(int32_t t) const { return Mine(t) || (board_.Current(t) && board_.at(t).occupant >= 0); }

StrategyAgent::Cell StrategyAgent::Classify(int32_t t) const {
  if (t == kBlocked) return Cell::kWall;
  if (t == kUnknown) return Cell::kUnknown;
  if (!Occupied(t)) return Cell::kFree;
  if (Mine(t)) return Cell::kBody;
  const TileMemory& m = board_.at(t);
  if (!m.occupant_head || m.occupant == spawn_.id) return Cell::kBody;
  return m.occupant_team == spawn_.team ? Cell::kAllyHead : Cell::kEnemyHead;
}

int32_t StrategyAgent::EnemiesSeen() const {
  int32_t n = 0;
  for (const auto& [id, w] : seen_)
    if (w.team != spawn_.team && round_ - w.round <= p_.enemy_memory) ++n;
  return n;
}

// Both die in a head-to-head. Worth it when theirs is the bigger loss, or (for a worker)
// when we clearly outnumber them; never with our last dragon. The enemies seen are a
// lower bound on their numbers, hence the margin.
bool StrategyAgent::WorthTrading(int32_t enemy_id) const {
  if (units_ <= 1) return false;
  const auto it = seen_.find(enemy_id);
  const int32_t theirs = it == seen_.end() ? 0 : it->second.length;
  if (leader_) return theirs > length_;
  // trades are one for one: when they have more dragons, even trades walk us to elimination
  if (p_.outnumbered_factor > 0 && units_ < EnemiesSeen()) return theirs >= p_.outnumbered_factor * length_;
  return theirs >= length_ + p_.trade_margin ||
         (p_.outnumber_factor > 0 && units_ >= p_.hunt_min_units &&
          units_ >= p_.outnumber_factor * std::max(1, EnemiesSeen()));
}

bool StrategyAgent::Exposed(int32_t t) const {
  if (!leader_ && !p_.avoid_heads) return false;
  for (int d = 0; d < 4; ++d)
    if (const int32_t n = board_.Step(t, d); n >= 0 && Classify(n) == Cell::kEnemyHead && !WorthTrading(board_.at(n).occupant))
      return true;
  return false;
}

// Torus distance to the nearest enemy head seen in the last few rounds, for each step.
int StrategyAgent::FleeDirection() const {
  std::vector<int32_t> heads;
  for (const auto& [id, w] : seen_)
    if (w.team != spawn_.team && w.head >= 0 && round_ - w.round <= 3) heads.push_back(w.head);
  if (heads.empty()) return -1;
  auto distance = [&](int32_t a, int32_t b) {
    const int32_t dx = std::abs(board_.X(a) - board_.X(b)), dy = std::abs(board_.Y(a) - board_.Y(b));
    return std::min(dx, board_.width() - dx) + std::min(dy, board_.height() - dy);
  };
  int best = -1, best_far = -1;
  for (int d = 0; d < 4; ++d) {
    const int32_t to = board_.Step(head_, d);
    if (Classify(to) != Cell::kFree) continue;
    int32_t nearest = 1 << 30;
    for (const int32_t h : heads) nearest = std::min(nearest, distance(to, h));
    if (nearest > best_far) {
      best_far = nearest;
      best = d;
    }
  }
  return best;
}

// ---- sonar: who the champion is ----------------------------------------------------------------------
// 64-bit word: magic (8) | team (1) | id (12) | x (6) | y (6) | length (12) | round (9)
static constexpr uint64_t kMagic = 0xA7;

uint64_t StrategyAgent::Encode(const Report& r) const {
  const uint64_t x = board_.X(r.tile) & 63, y = board_.Y(r.tile) & 63;
  return (kMagic << 56) | (uint64_t(spawn_.team & 1) << 55) | (uint64_t(r.id & 4095) << 43) | (x << 37) | (y << 31) |
         (uint64_t(std::min(r.length, 4095)) << 19) | (uint64_t(r.round & 511) << 10);
}

void StrategyAgent::Decode(uint64_t w) {
  if ((w >> 56) != kMagic || int32_t((w >> 55) & 1) != (spawn_.team & 1) || (w & 1023) != 0) return;
  Report r;
  r.id = int32_t((w >> 43) & 4095);
  const int32_t x = int32_t((w >> 37) & 63), y = int32_t((w >> 31) & 63);
  if (x >= board_.width() || y >= board_.height()) return;
  r.tile = board_.Tile(x, y);
  r.length = int32_t((w >> 19) & 4095);
  r.round = int32_t((w >> 10) & 511);
  if (r.round > round_ || r.id == spawn_.id) return;
  Note(r);
}

void StrategyAgent::Note(const Report& r) {
  Report& have = reports_[r.id];
  if (r.round > have.round || (r.round == have.round && r.length > have.length)) have = r;
}

StrategyAgent::Report StrategyAgent::Champion() const {
  // the longest, less a little per round since it was last heard of: a champion that died
  // (or split its mass off into a child out of a trap, leaving a stub under its id) is only
  // ever relayed with its last round, so its report fades and the heir takes over
  Report best;
  double best_score = 0;
  for (const auto& [id, r] : reports_) {
    if (round_ - r.round > p_.report_memory) continue;
    const double score = r.length - p_.report_fade * (round_ - r.round);
    if (best.id < 0 || score > best_score || (score == best_score && r.id < best.id)) {
      best = r;
      best_score = score;
    }
  }
  return best;
}

// ---- where to go --------------------------------------------------------------------------------

// A pearl seen there is worth 1, discounted by how long ago (someone may have eaten it);
// a bed is worth 1 once its countdown runs out (rounds until then: *ready), a bed that
// ran out while out of view a bit less. Occupied tiles and unseen ones are worth nothing.
double StrategyAgent::TileValue(int32_t t, int32_t dist, int32_t* ready) const {
  const TileMemory& m = board_.at(t);
  *ready = dist;
  if (m.seen_round < 0 || Occupied(t)) return 0.0;
  const int32_t age = round_ - m.seen_round;
  if (m.pearl) return std::pow(p_.pearl_decay, age);
  if (m.countdown < 0) return 0.0;                         // not a bed
  const int32_t due = m.countdown - age;                   // rounds until it grows a pearl
  if (due > 0) {
    *ready = std::max(dist, due);
    return 1.0;
  }
  return age == 0 ? 1.0 : p_.ripe_bed_value;               // due now (blocked while occupied)
}

int32_t StrategyAgent::RoomNeeded() const { return std::max(length_ + p_.room_margin, p_.room_min); }

StrategyAgent::Goal StrategyAgent::ChooseGoal(const Paths& paths) const {
  // hunting: a worker with dragons to spare goes for enemy heads it just saw
  if (!leader_ && units_ >= p_.hunt_min_units && round_ < p_.feed_from_round) {
    int32_t best = -1;
    for (const auto& [id, w] : seen_)
      if (w.team != spawn_.team && w.round == round_ && w.head >= 0 && paths.dist[w.head] > 0 &&
          paths.dist[w.head] <= p_.hunt_radius && WorthTrading(id) &&
          (best < 0 || paths.dist[w.head] < paths.dist[best]))
        best = w.head;
    if (best >= 0) return {best, false};
  }
  // a waller long enough to wall goes where enemy heads are (seen in the last few rounds)
  const bool at_cap = units_ >= spawn_.unit_limit - p_.split_headroom;
  const bool cap_waller = !leader_ && at_cap && p_.cap_waller_mod > 0 && spawn_.id % p_.cap_waller_mod == 0;
  if ((waller_ || cap_waller) && length_ >= p_.squeeze_length && round_ < p_.wall_until) {
    int32_t best = -1;
    for (const auto& [id, w] : seen_)
      if (w.team != spawn_.team && round_ - w.round <= p_.waller_chase_memory && w.head >= 0 && paths.dist[w.head] > 0 &&
          paths.dist[w.head] <= p_.waller_chase_radius && (best < 0 || paths.dist[w.head] < paths.dist[best]))
        best = w.head;
    if (best >= 0) return {best, false};
  }
  // every reachable tile scored by value per round to collect it; the unexplored edge too
  struct Candidate {
    int32_t tile;
    double score;
  };
  std::vector<Candidate> candidates;
  // allies in view now: a tile one of them is nearer to is probably theirs
  std::vector<int32_t> allies;
  const bool champion = round_ >= p_.feed_from_round && Champion().id == spawn_.id;
  if (p_.share_factor < 1.0 && (!leader_ || p_.share_leader) && !champion)
    for (const auto& [id, w] : seen_)
      if (w.team == spawn_.team && w.round == round_ && w.head >= 0) allies.push_back(w.head);
  // enemy heads in view now, with their ids: a pearl or bed one of them gets to first is theirs.
  // Turn order (id order) settles ties only at a bed's tick: both waiting next to it, the
  // lower id eats the pearl. For a pearl already there we move first from now, so we win ties.
  std::vector<std::pair<int32_t, int32_t>> enemies;  // (head tile, id)
  if (p_.contest_factor < 1.0)
    for (const auto& [id, w] : seen_)
      if (w.team != spawn_.team && w.round == round_ && w.head >= 0) enemies.push_back({w.head, id});
  auto torus = [&](int32_t a, int32_t b) {
    const int32_t dx = std::abs(board_.X(a) - board_.X(b)), dy = std::abs(board_.Y(a) - board_.Y(b));
    return std::min(dx, board_.width() - dx) + std::min(dy, board_.height() - dy);
  };
  for (int32_t t = 0; t < board_.size(); ++t) {
    const int32_t d = paths.dist[t];
    if (d <= 0) continue;
    int32_t ready;
    double score = TileValue(t, d, &ready) / (ready + 1);
    if (score > 0)
      for (const int32_t a : allies)
        if (torus(a, t) < d) {
          score *= p_.share_factor;
          break;
        }
    if (score > 0 && !enemies.empty()) {
      const TileMemory& m = board_.at(t);
      const int32_t due = m.pearl ? 0 : std::max(0, m.countdown - (round_ - m.seen_round));
      for (const auto& [e, id] : enemies) {
        const int32_t theirs = std::max(torus(e, t), due);
        if (theirs < ready || (theirs == ready && due > 0 && ready == due && id < spawn_.id)) {
          score *= p_.contest_factor;
          break;
        }
      }
    }
    for (int dir = 0; dir < 4; ++dir)
      if (board_.Step(t, dir) == kUnknown && !(p_.portal_explore && (portal_[t] >> dir & 1) && !WillCross())) {
        score = std::max(score, p_.explore_value / (d + 1));
        break;
      }
    if (score > 0 && !Banned(t)) candidates.push_back({t, score});
  }
  const size_t k = std::min<size_t>(candidates.size(), p_.trap_candidates);
  std::partial_sort(candidates.begin(), candidates.begin() + k, candidates.end(),
                    [](const Candidate& a, const Candidate& b) { return a.score > b.score; });
  // the best few, checked for dead ends: a dead end is worth its pocket's pearls less the
  // cost of splitting out of it
  const int32_t need = RoomNeeded();
  const bool no_trap = p_.cap_safety && length_ >= 4 && !CanCountOnSplit();
  auto open = [&](int32_t t) { return !Occupied(t); };
  Goal best;
  double best_score = 0;
  std::vector<int32_t> pocket;
  for (size_t i = 0; i < k; ++i) {
    const Candidate& c = candidates[i];
    double score = c.score;
    bool trap = false;
    if (RoomAround(board_, c.tile, open, need, &pocket) < need) {
      if (no_trap) continue;                          // near the cap: a dead end is a grave (gst27)
      double gain = 0;
      for (const int32_t t : pocket) {
        int32_t ready;
        gain += TileValue(t, paths.dist[t] < 0 ? paths.dist[c.tile] : paths.dist[t], &ready);
      }
      score = (gain - p_.trap_cost) / (paths.dist[c.tile] + 1);
      trap = true;
    }
    if (score > best_score) {
      best_score = score;
      best = {c.tile, trap};
    }
  }
  // nothing worth going for: patrol, towards what we have not seen for longest
  if (best.tile < 0 && p_.patrol) {
    double top = 0;
    for (int32_t t = 0; t < board_.size(); ++t) {
      const int32_t d = paths.dist[t];
      if (d <= 0 || Occupied(t) || Banned(t)) continue;
      const double score = (round_ - board_.at(t).seen_round) / (d + 4.0);
      if (score > top) {
        top = score;
        best = {t, false};
      }
    }
  }
  return best;
}

// ---- how to step ----------------------------------------------------------------------------------

#ifdef BW7_DEBUG
void StrategyAgent::Dump(const char* why) const {
  std::fprintf(stderr, "== id %d len %d round %d born %d units %d leader %d: %s (last: %s)\n", spawn_.id, length_, round_, born_, units_, leader_, why, reason_);
  const int R = 5;
  for (int dy = -R; dy <= R; ++dy) {
    std::string row, below;
    for (int dx = -R; dx <= R; ++dx) {
      const int32_t t = board_.Tile(board_.X(head_) + dx, board_.Y(head_) + dy);
      char c = '.';
      const TileMemory& m = board_.at(t);
      if (m.seen_round < 0) c = ' ';
      else if (t == head_) c = 'H';
      else if (Mine(t)) c = 'b';
      else if (board_.Current(t) && m.occupant >= 0) c = m.occupant_team == spawn_.team ? (m.occupant_head ? 'A' : 'a') : (m.occupant_head ? 'E' : 'e');
      else if (m.pearl) c = 'o';
      row += c;
      const int32_t e = board_.Step(t, 1), sdn = board_.Step(t, 2);
      row += e == kBlocked ? '|' : e == kUnknown ? ':' : (e != board_.Tile(board_.X(t) + 1, board_.Y(t)) ? 'p' : ' ');
      below += sdn == kBlocked ? '-' : sdn == kUnknown ? '~' : (sdn != board_.Tile(board_.X(t), board_.Y(t) + 1) ? 'p' : ' ');
      below += ' ';
    }
    std::fprintf(stderr, "%s\n%s\n", row.c_str(), below.c_str());
  }
}
#endif

// Exact search for a way out of a small region: the dragon's own moves, its body simulated
// (other dragons stand still). 2: it gets back onto a tile its body left, or to the unexplored
// edge; 0: every line of play dies within max_depth moves; 1: out of budget.
int StrategyAgent::TurnOut(const std::vector<int32_t>& body, int32_t max_depth) const {
  std::vector<uint8_t> orig(board_.size(), 0), occ(board_.size(), 0);
  for (const int32_t t : body) orig[t] = 1, ++occ[t];
  std::deque<int32_t> snake(body.begin(), body.end());
  int32_t budget = p_.turnout_budget;
  // returns 2 success, 0 dead, 1 budget
  std::function<int(int32_t)> go = [&](int32_t depth) -> int {
    if (depth >= max_depth) return 0;
    int result = 0;
    const int32_t head = snake.front();
    for (int d = 0; d < 4; ++d) {
      const int32_t n = board_.Step(head, d);
      if (n == kUnknown) return 2;
      if (n < 0 || occ[n]) continue;
      if (board_.Current(n) && board_.at(n).occupant >= 0 && board_.at(n).occupant != spawn_.id) continue;
      if (orig[n]) return 2;
      if (--budget < 0) return 1;
      BW7_COUNT(g_turnout);
      const int32_t tail = snake.back();
      snake.pop_back();
      --occ[tail];
      snake.push_front(n);
      ++occ[n];
      const int r = go(depth + 1);
      snake.pop_front();
      --occ[n];
      snake.push_back(tail);
      ++occ[tail];
      if (r == 2) return 2;
      result = std::max(result, r);
    }
    return result;
  };
  return go(0);
}

// Both halves of a split of the last n segments can get out.
bool StrategyAgent::SplitSafe(int32_t n) const {
  if (static_cast<int32_t>(body_.size()) < length_) return true;   // body not fully known
  std::vector<int32_t> parent(body_.begin(), body_.end() - n), child(body_.rbegin(), body_.rbegin() + n);
  return EscapeLevel(child, EscapeCap(n), nullptr, &parent) >= 1 &&
         EscapeLevel(parent, EscapeCap(length_ - n), nullptr, &child) >= 1;
}

// The next turn's options after stepping onto `to` (body after the step: `body`):
// 2 a free neighbour no other head touches, 1 only free neighbours another head could take
// first, 0 none.
int StrategyAgent::Liberties(int32_t to, const std::vector<int32_t>& body) const {
  int libs = 0;
  for (int d = 0; d < 4; ++d) {
    const int32_t n = board_.Step(to, d);
    if (n == kUnknown) return 2;
    if (n < 0 || std::find(body.begin(), body.end(), n) != body.end()) continue;
    if (board_.Current(n) && board_.at(n).occupant >= 0) continue;
    bool contested = false;
    for (int e = 0; e < 4 && !contested; ++e) {
      const int32_t m = board_.Step(n, e);
      contested = m >= 0 && m != to && board_.Current(m) && board_.at(m).occupant >= 0 && board_.at(m).occupant_head &&
                  board_.at(m).occupant != spawn_.id;
    }
    libs = std::max(libs, contested ? 1 : 2);
  }
  return libs;
}

int StrategyAgent::EscapeLevel(const std::vector<int32_t>& body, int32_t cap, int32_t* count,
                               const std::vector<int32_t>* blocked) const {
  const int32_t n = board_.size(), len = static_cast<int32_t>(body.size());
  std::vector<int32_t> index(n, -1);                   // own segment index, head 0
  if (blocked)
    for (const int32_t t : *blocked) index[t] = 1 << 20;   // never freed in time
  for (int32_t i = len - 1; i >= 0; --i) index[body[i]] = i;
  std::vector<int32_t> when(n, -1);                    // time a tile was entered
  std::vector<int32_t> queue{body[0]};
  when[body[0]] = 0;
  int32_t reached = 1;
  auto done = [&](int level) {
    if (count) *count = std::min(reached, cap);
    return level;
  };
  BW7_COUNT(g_calls);
  for (size_t qi = 0; qi < queue.size(); ++qi) {
    BW7_COUNT(g_escape);
    const int32_t t = queue[qi], m = when[t] + 1;     // the move that would enter a neighbour
    for (int d = 0; d < 4; ++d) {
      const int32_t to = board_.Step(t, d);
      if (to == kUnknown) return done(2);
      if (to < 0 || when[to] >= 0) continue;
      if (const int32_t i = index[to]; i >= 0) {
        if (i < len && i >= len - m + 1 && i > 0) return done(2);   // freed by then: follow the tail
        continue;                                      // still there (maybe later via a longer way)
      }
      if (board_.Current(to) && board_.at(to).occupant >= 0 && board_.at(to).occupant != spawn_.id) continue;
      when[to] = m;
      if (++reached >= cap) return done(1);
      queue.push_back(to);
    }
  }
  return done(0);
}

bool StrategyAgent::CanSplitNow() const {
  if (length_ < 4 || FreeSlots() < 1) return false;
  return length_ >= p_.reserve_length || FreeSlots() > p_.reserve_slots;
}

bool StrategyAgent::CanCountOnSplit() const {
  if (length_ < 4) return false;
  if (!p_.cap_safety) return true;
  const int32_t reserve = length_ >= p_.reserve_length ? 0 : p_.reserve_slots;
  return FreeSlots() >= p_.cap_margin + reserve;
}

bot::Action StrategyAgent::SafeStep(int preferred, bool trap_ok) {
  // near the cap a long dragon can't split out of a dead end: plan like a short one (gst27)
  const bool can_split_out = CanCountOnSplit();
  if (p_.cap_safety && length_ >= 4 && !can_split_out) trap_ok = false;
  const int32_t need = RoomNeeded(), cap = EscapeCap(length_);
  // room to live in: free tiles, not counting those an enemy head can take next turn
  auto open = [&](int32_t t) { return !Occupied(t) && !Exposed(t); };
  std::vector<int32_t> body(body_.begin(), body_.end());
  body.insert(body.begin(), -1);
  if (static_cast<int32_t>(body.size()) > length_) body.pop_back();
  int best = -1, best_level = -1, best_count = -1, best_room = -1, best_libs = -1;
  bool best_exposed = true;
  for (int k = 0; k < 4; ++k) {
    const int d = ((preferred < 0 ? 0 : preferred) + k) % 4;   // the preferred direction first
    const int32_t to = board_.Step(head_, d);
    if (Classify(to) != Cell::kFree) continue;
    const int32_t room = RoomAround(board_, to, open, need);
    const bool exposed = Exposed(to);
    body[0] = to;
    int32_t count = 0;
    int level = p_.escape_steps ? EscapeLevel(body, cap, &count) : 1;
    if (level == 0 && p_.turnout_budget > 0) level = TurnOut(body, count + length_ + 2) == 0 ? 0 : 2;
    int libs = p_.liberties ? Liberties(to, body) : 2;
    // through a portal to a tile out of view: someone may be lying there
    const bool blind = p_.avoid_blind && !board_.Current(to);
    if (blind) libs = std::min(libs, 1);
    if (k == 0 && preferred >= 0 && !exposed && libs >= 2 && ((level >= 1 && room >= need) || (trap_ok && level >= 1)))
      return Because(room >= need ? "toward goal" : "into a dead end worth it", bot::Action::Move(d));
    if (k == 0 && preferred >= 0 && !exposed && libs >= 2 && trap_ok && (can_split_out || !p_.trap_small))
      return Because("into a dead end worth it", bot::Action::Move(d));
    // a sure death in a dead end is worse than a tile an enemy head might take (that would be a trade)
    auto key = [&](bool e, int lv, int lb, int c, int r) {
      return std::make_tuple(p_.doom_first ? lv > 0 : true, !e, lv, lb, c, r);
    };
    if (best < 0 || key(exposed, level, libs, count, room) > key(best_exposed, best_level, best_libs, best_count, best_room)) {
      best = d;
      best_level = level;
      best_count = count;
      best_room = room;
      best_libs = libs;
      best_exposed = exposed;
    }
  }
  // every step is a dead end we can't turn round in: split while we still can (the child
  // walks out the way we came)
  if (best >= 0 && best_level == 0 && best_count <= p_.doomed_split_room && CanSplitNow())
    return Because("doomed: split", bot::Action::Split(length_ - 2));
  if (best >= 0) return Because(preferred >= 0 ? "detour (goal step unsafe)" : "no goal: roomiest", bot::Action::Move(best));
  // no free step. Split: the child's head is our tail, which points back the way we came
  // (usually open), so the child walks out with most of the length; the stub stays behind
  if (CanSplitNow()) return Because("trapped: split", bot::Action::Split(length_ - 2));
  // else trade with an adjacent enemy head, else step through an unexplored portal
  for (int d = 0; d < 4; ++d)
    if (Classify(board_.Step(head_, d)) == Cell::kEnemyHead) return Because("trapped: trade", bot::Action::Move(d));
  for (int d = 0; d < 4; ++d)
    if (Classify(board_.Step(head_, d)) == Cell::kUnknown) return Because("trapped: unknown portal", bot::Action::Move(d));
#ifdef BW7_DEBUG
  Dump("trapped: no move");
#endif
  return Because("trapped: no move", bot::Action::Resign());
}

// ---- the lunge (ported from swarm-hunt-2) -------------------------------------------------------

// Breadth-first over free tiles, at most length-1 steps (every step after the first costs a
// tail segment): the nearest enemy head worth a trade. Nothing else moves during our turn.
bool StrategyAgent::Lunge(bot::Action* out) const {
  const int32_t reach = std::min<int32_t>(length_ - 1, p_.lunge_reach);
  if (reach < 2) return false;   // one step is the plain kamikaze
  std::unordered_map<int32_t, std::pair<int32_t, int8_t>> from;
  std::vector<std::pair<int32_t, int32_t>> frontier{{head_, 0}};
  from[head_] = {-1, -1};
  for (size_t q = 0; q < frontier.size() && q < 256; ++q) {
    const auto [t, dist] = frontier[q];
    if (dist >= reach) continue;
    for (int d = 0; d < 4; ++d) {
      const int32_t n = board_.Step(t, d);
      if (n < 0 || from.count(n)) continue;
      const Cell c = Classify(n);
      if (c == Cell::kEnemyHead && WorthTrading(board_.at(n).occupant)) {
        std::vector<int> steps{d};
        for (int32_t u = t; from.at(u).first >= 0; u = from.at(u).first) steps.push_back(from.at(u).second);
        std::reverse(steps.begin(), steps.end());
        *out = bot::Action{bot::Action::kMove, steps, 0, false, {}};
        return true;
      }
      if (c != Cell::kFree || !board_.Current(n)) continue;
      from[n] = {t, static_cast<int8_t>(d)};
      frontier.push_back({n, dist + 1});
    }
  }
  return false;
}

// ---- the body wall ---------------------------------------------------------------------------

// The tiles enemy `id` (body head first, as seen) can reach, up to `cap`, with our body
// `ours` (head first, after our move). Timed: its own tail and our tail free tiles as they
// move on, and reaching its freed tail is a way out (returns cap). Static: every body stays.
// Timed: leaving the window is a way out (we can't tell); static: only the room in view counts.
int32_t StrategyAgent::EnemyReach(int32_t id, const std::vector<int32_t>& enemy, const std::vector<int32_t>& ours,
                                  bool timed, int32_t cap) const {
  const int32_t n = board_.size(), elen = static_cast<int32_t>(enemy.size()), olen = static_cast<int32_t>(ours.size());
  std::vector<int32_t> ei(n, -1), oi(n, -1), when(n, -1);
  for (int32_t i = elen - 1; i >= 0; --i) ei[enemy[i]] = i;
  for (int32_t i = olen - 1; i >= 0; --i)
    if (ours[i] >= 0) oi[ours[i]] = i;
  std::vector<int32_t> queue{enemy[0]};
  when[enemy[0]] = 0;
  int32_t reached = 1;
  for (size_t q = 0; q < queue.size(); ++q) {
    const int32_t t = queue[q], m = when[t] + 1;
    for (int d = 0; d < 4; ++d) {
      const int32_t to = board_.Step(t, d);
      if (to == kUnknown) return cap;
      if (to < 0 || when[to] >= 0) continue;
      if (!board_.Current(to)) {                         // out of view: can't tell
        if (timed) return cap;
        continue;                                        // static: count the room in view only
      }
      if (const int32_t i = ei[to]; i >= 0) {
        if (timed && i > 0 && i >= elen - m + 1) return cap;   // follows its own tail out
        continue;
      }
      if (const int32_t j = oi[to]; j >= 0) {
        if (!timed || olen - j > m) continue;            // our segment still there then
      } else if (board_.at(to).occupant >= 0 && board_.at(to).occupant != spawn_.id && board_.at(to).occupant != id) {
        continue;                                        // another dragon (assumed to stay)
      }
      when[to] = m;
      if (++reached >= cap) return cap;
      queue.push_back(to);
    }
  }
  return reached;
}

// A step that seals enemy heads in view into pockets they die in (more enemy length sealed
// than by our other safe steps); failing that, for a long dragon, the step that shrinks the
// room around enemy heads most (closing a pocket behind them).
bool StrategyAgent::Wall(bot::Action* out, bool* squeezed) {
  if (!p_.wall || length_ < p_.wall_min_length) return false;
  if (round_ >= p_.feed_from_round && Champion().id == spawn_.id) return false;
  if (leader_ && round_ >= p_.wall_leader_round) return false;
  if (static_cast<int32_t>(body_.size()) < length_) return false;   // own body not fully known
  std::vector<std::pair<int32_t, const std::vector<int32_t>*>> enemies, allies;
  for (const auto& [id, b] : bodies_) {
    const auto it = seen_.find(id);
    if (it == seen_.end() || it->second.round != round_ || b.empty() || it->second.head != b[0]) continue;
    if (it->second.team == spawn_.team) {
      if (p_.ally_room_cost > 0) allies.push_back({id, &b});
    } else {
      enemies.push_back({id, &b});
    }
  }
  if (enemies.empty()) return false;
  const bool squeeze = p_.squeeze_room > 0 && length_ >= p_.squeeze_length && round_ < p_.wall_until;
  int best = -1;
  int32_t best_seal = -1, best_sq = -1, worst_seal = 1 << 30, worst_sq = 1 << 30;
  std::vector<int32_t> ours(body_.begin(), body_.end());
  // the room each allied head in view has now (our body as it is), to charge the squeeze what it takes
  std::vector<int32_t> ally_room;
  if (squeeze)
    for (const auto& [id, b] : allies) ally_room.push_back(EnemyReach(id, *b, ours, false, p_.squeeze_room));
  ours.insert(ours.begin(), -1);
  ours.pop_back();
  for (int d = 0; d < 4; ++d) {
    const int32_t to = board_.Step(head_, d);
    if (Classify(to) != Cell::kFree || !board_.Current(to) || Exposed(to)) continue;
    ours[0] = to;
    if (EscapeLevel(ours, EscapeCap(length_)) < 1 || Liberties(to, ours) < 1) continue;
    int32_t seal = 0, sq = 0;
    for (const auto& [id, b] : enemies) {
      if (EnemyReach(id, *b, ours, true, p_.wall_cap) < p_.wall_cap) {
        seal += static_cast<int32_t>(b->size());
      } else if (squeeze) {
        sq += p_.squeeze_room - EnemyReach(id, *b, ours, false, p_.squeeze_room);
      }
    }
    if (squeeze && seal == 0)
      for (size_t k = 0; k < allies.size(); ++k)
        sq -= static_cast<int32_t>(p_.ally_room_cost *
                                   std::max(0, ally_room[k] - EnemyReach(allies[k].first, *allies[k].second, ours, false, p_.squeeze_room)));
    worst_seal = std::min(worst_seal, seal);
    worst_sq = std::min(worst_sq, sq);
    if (std::make_pair(seal, sq) > std::make_pair(best_seal, best_sq)) {
      best_seal = seal;
      best_sq = sq;
      best = d;
    }
  }
  if (best < 0) return false;
  if (best_seal > worst_seal) {
    *squeezed = false;
  } else if (squeeze && best_sq > worst_sq && best_sq >= p_.squeeze_gain) {
    *squeezed = true;
  } else {
    return false;
  }
  *out = bot::Action::Move(best);
  return true;
}

// ---- the turn --------------------------------------------------------------------------------------

bot::Action StrategyAgent::Turn(const rl::Observation& o) {
  bot::Action a = Decide(o);
  // tell the dragons around us who the champion is (a dead dragon's pings are not cast)
  const Report champ = Champion();
  if (champ.id >= 0)
    for (int d = 0; d < 4; ++d) a.sonar[d] = Encode(champ);
  // stepping next to an unknown portal: ping only through it, so next turn's echoes say
  // whether a dragon lies past its exit
  if (WillCross() && a.kind == bot::Action::kMove && !a.resign && a.steps.size() == 1) {
    const int d = a.steps[0];
    const int32_t n = board_.Step(head_, d);
    if (n >= 0 && board_.Current(n))
      for (int e = 0; e < 4; ++e)
        if (e != (d + 2) % 4 && UnknownPortal(n, e)) {
          a.sonar = {};
          a.sonar[e] = champ.id >= 0 ? Encode(champ) : 1;
          ping_round_ = round_;
          ping_tile_ = n;
          ping_dir_ = e;
          break;
        }
  }
#ifdef BW7_DEBUG
  ++g_turns;
  if (Looping()) ++g_idle;
  if (length_ > 0 && round_ == last_eat_ && round_ > born_) ++g_eaten;
  if (std::string(reason_) == "portal: explore") ++g_cross;
#endif
  return a;
}

bot::Action StrategyAgent::Decide(const rl::Observation& o) {
  Remember(o);

  // 1. feeding: late in the game every dragon but the champion walks to it and dies next to
  // it, leaving it pearls (a leader only near the end, and only when it knows where it is)
  const Report champ = Champion();
  const bool feeding = champ.id >= 0 && champ.id != spawn_.id && champ.length >= length_ &&
                       (leader_ ? round_ >= p_.leader_feed_from && round_ - champ.round <= p_.leader_feed_fresh
                                : round_ >= p_.feed_from_round);
  int32_t ally_head = -1;
  bool in_view = false;                              // only die next to it when we see it now
  bool held = false;
  if (feeding) {
    if (const auto it = seen_.find(champ.id); it != seen_.end() && it->second.round == round_ && it->second.head >= 0) {
      ally_head = it->second.head;
      in_view = true;
      // (late-surge-6) only feed a target clearly longer than us, by its length as known this turn
      const auto rit = reports_.find(champ.id);
      const int32_t tl = rit != reports_.end() ? rit->second.length : champ.length;
      if (length_ * 10 >= tl * p_.feed_clear_tenths) {
        ally_head = -1;                              // hold: keep gathering instead
        in_view = false;
        held = true;
      }
    } else if (champ.tile != head_ && !board_.Current(champ.tile)) {
      ally_head = champ.tile;                        // where it was last reported (out of view)
    }
  }
  const Paths paths = Search(board_, head_, [&](int32_t t) {
    return !Occupied(t) || Classify(t) == Cell::kEnemyHead || t == ally_head;
  });
  if (in_view && paths.dist[ally_head] >= 0 && paths.dist[ally_head] <= p_.feed_distance)
    return Because("feed champion", bot::Action::Resign());

  // 2. kamikaze: an enemy head next to ours, and the trade pays off
  for (int d = 0; d < 4; ++d) {
    const int32_t to = board_.Step(head_, d);
    if (Classify(to) == Cell::kEnemyHead && WorthTrading(board_.at(to).occupant))
      return Because("kamikaze", bot::Action::Move(d));
  }

  // 2b. lunge: an enemy head worth a trade within a multi-step MOVE (never the champion: it only grows)
  if (units_ > 1 && !(round_ >= p_.feed_from_round && champ.id == spawn_.id)) {
    bot::Action lunge;
    if (Lunge(&lunge)) return Because("lunge", lunge);
  }

  // 2c. body wall: seal an enemy head into a pocket it dies in
  {
    bot::Action wall;
    bool squeezed = false;
    if (Wall(&wall, &squeezed)) return Because(squeezed ? "wall: squeeze" : "wall: seal", wall);
  }

  // 3. survival: our last dragons, outnumbered, keep away from enemy heads (losing them all loses the game)
  if (units_ <= p_.survivor_units && EnemiesSeen() > units_)
    if (const int flee = FleeDirection(); flee >= 0) {
      bot::Action a = SafeStep(flee, false);
      if (std::string(reason_) == "toward goal") reason_ = "survivor: flee";
      return a;
    }

  // 4. growth: split off a worker while there is time for it to gather
  if (length_ >= (waller_ ? p_.waller_split : p_.split_min_length) && round_ < (leader_ ? p_.leader_split_until : p_.split_until_round) &&
      FreeSlots() > std::max(p_.split_headroom, p_.reserve_slots) &&
      !(round_ >= p_.champ_freeze && champ.id == spawn_.id) &&
      (!p_.check_split || SplitSafe(length_ / 2)))
    return Because("split", bot::Action::Split(length_ / 2));

  // 5. a goal, and the first safe step towards it
  const Goal target = ally_head >= 0 && paths.dist[ally_head] > 0 ? Goal{ally_head, false} : ChooseGoal(paths);
  const int32_t goal = target.tile;
  // loop breaker: circling without eating: ban this goal for a while
  // (not while waiting beside a bed for its countdown: circling there is the plan)
  const bool waiting = goal >= 0 && paths.dist[goal] > 0 && [&] {
    int32_t ready;
    return TileValue(goal, paths.dist[goal], &ready) > 0.0 && ready > paths.dist[goal];
  }();
  if (p_.loop_break && goal >= 0 && !waiting && Looping() && !(ally_head >= 0 && goal == ally_head)) {
    banned_[goal] = round_ + p_.loop_ban;
    if (last_goal_ >= 0) banned_[last_goal_] = round_ + p_.loop_ban;
#ifdef BW7_DEBUG
    ++g_bans;
#endif
    recent_.clear();
  }
  last_goal_ = goal;
  // exploring beside an unknown portal whose exit our ping found clear: step through it
  if (WillCross() && exit_clear_ && ping_tile_ == head_ && !target.trap && length_ >= 2 && UnknownPortal(head_, ping_dir_) &&
      BodyInView() && (goal < 0 || [&] {
        int32_t ready;
        return TileValue(goal, paths.dist[goal], &ready) == 0.0;
      }())) {
    // (a waller chasing a recently seen enemy head is not exploring: it doesn't cross)
    bool enemy = false;
    for (const auto& [id, w] : seen_)
      enemy |= w.team != spawn_.team && w.head >= 0 && (w.round == round_ || (goal >= 0 && w.head == goal));
    if (!enemy) return Because("portal: explore", bot::Action::Move(ping_dir_));
  }
  const int preferred = goal >= 0 ? paths.first[goal] : -1;
  bot::Action a = SafeStep(preferred, target.trap);
  // the last step onto an enemy head we chose to hunt is a trade, not a mistake
  if (goal >= 0 && preferred >= 0 && board_.Step(head_, preferred) == goal && Classify(goal) == Cell::kEnemyHead &&
      WorthTrading(board_.at(goal).occupant))
    return Because("hunt: trade", bot::Action::Move(preferred));
  if (held && std::string(reason_) == "toward goal") reason_ = "held: peer not clearly longer";
  return a;
}

}  // namespace bw7
