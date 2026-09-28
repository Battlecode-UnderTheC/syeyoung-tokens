#include <type_traits>
#include <cstdio>
#include <cstdlib>
#include "cs15.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace cs15 {

StrategyAgent::StrategyAgent(const rl::Spawn& spawn, Params params)
    : p_(params), spawn_(spawn), board_(spawn.width, spawn.height) {
#ifdef CS15_TUNE
  auto env = [](const char* k, auto& v) { if (const char* e = getenv(k)) v = static_cast<std::remove_reference_t<decltype(v)>>(atof(e)); };
  env("P_SPLIT", p_.split_min_length); env("P_SPLITR", p_.split_until_round); env("P_FEED", p_.feed_from_round);
  env("P_LIBS", p_.min_libs); env("P_MARGIN", p_.trade_margin); env("P_HUNT", p_.hunt_radius); env("P_HUNTU", p_.hunt_min_units);
  env("P_ROOMM", p_.room_margin); env("P_ROOMMIN", p_.room_min); env("P_TRAP", p_.trap_cost); env("P_REACH", p_.strike_reach); env("P_FEEDD", p_.feed_distance); env("P_EXPL", p_.explore_value); env("P_PATV", p_.patrol_value); env("P_PATF", p_.patrol_from); env("P_PATA", p_.patrol_age); env("P_SMEM", p_.snipe_memory); env("P_SFAC", p_.snipe_factor); env("P_LSPLIT", p_.leader_split_until); env("P_ALLYM", p_.ally_memory); env("P_X1", p_.x1); env("P_X2", p_.x2); env("P_X3", p_.x3);
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
    leader_ = o.round == 0;                         // alive at the start: not a split child
    first_turn_ = false;
    for (const rl::Segment& s : o.segments)         // the part of the body in view, head to tail
      if (s.id == spawn_.id) body_.push_back(board_.Tile(s.x, s.y));
  }
  if (body_.empty() || body_.front() != head_) body_.push_front(head_);
  while (static_cast<int32_t>(body_.size()) > length_) body_.pop_back();
  std::unordered_map<int32_t, int32_t> segments;
  for (const rl::Segment& s : o.segments) {
    if (s.id == spawn_.id) continue;
    ++segments[s.id];
    Sighting& w = seen_[s.id];
    w.team = s.team;
    if (w.round != round_) w.cut = false;
    w.round = round_;
    {
      const int32_t dx = std::abs(s.x - o.head_x()), dy = std::abs(s.y - o.head_y());
      if (std::min(dx, board_.width() - dx) >= 3 || std::min(dy, board_.height() - dy) >= 3) w.cut = true;
    }
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
    have.length = fresh && (w.cut || !p_.trust_visible) ? std::max(have.length, n) : n;
    have.round = round_;
  }
  reports_[spawn_.id] = Report{spawn_.id, head_, length_, round_};
}

// ---- sonar: who the champion is ----------------------------------------------------------------------
// 64-bit word: magic (8) | team (1) | id (12) | x (6) | y (6) | length (12) | round (9) | 0 (10)
static constexpr uint64_t kMagic = 0x5C;

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
  Report& have = reports_[r.id];
  if (r.round > have.round || (r.round == have.round && r.length > have.length)) have = r;
}

StrategyAgent::Report StrategyAgent::Champion() const {
  Report best;
  double best_score = 0;
  for (const auto& [id, r] : reports_) {
    if (round_ - r.round > p_.report_memory) continue;
    // (from gather-split-trade-23) the longest, less a little per round since it was last heard
    // of: a dead champion (or the stub left when it split out of a trap) fades and its heir takes over
    const double score = r.length - p_.report_fade * (round_ - r.round);
    if (best.id < 0 || score > best_score || (score == best_score && r.id < best.id)) {
      best = r;
      best_score = score;
    }
  }
  return best;
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

int32_t StrategyAgent::Torus(int32_t a, int32_t b) const {
  const int32_t dx = std::abs(board_.X(a) - board_.X(b)), dy = std::abs(board_.Y(a) - board_.Y(b));
  return std::min(dx, board_.width() - dx) + std::min(dy, board_.height() - dy);
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
  if (!leader_ && units_ >= p_.hunt_min_units && round_ < p_.hunt_until) {
    int32_t best = -1;
    for (const auto& [id, w] : seen_)
      if (w.team != spawn_.team && w.round == round_ && w.head >= 0 && paths.dist[w.head] > 0 &&
          paths.dist[w.head] <= p_.hunt_radius && WorthTrading(id) &&
          (best < 0 || paths.dist[w.head] < paths.dist[best]))
        best = w.head;
    if (best >= 0) return {best, false};
  }
  // every reachable tile scored by value per round to collect it; the unexplored edge too
  struct Candidate {
    int32_t tile;
    double score;
  };
  std::vector<Candidate> candidates;
  const bool champion = round_ >= p_.feed_from_round && Champion().id == spawn_.id;
  std::vector<int32_t> allies;   // ally heads in view now
  for (const auto& [id, w] : seen_)
    if (w.team == spawn_.team && w.round == round_ && w.head >= 0) allies.push_back(w.head);
  // (from gather-split-trade) enemy heads in view now: a pearl or bed one of them gets to first
  // is theirs (turn order, i.e. id order, settles ties only at a bed's tick)
  std::vector<std::pair<int32_t, int32_t>> enemies;  // (head tile, id)
  if (p_.contest_factor < 1.0)
    for (const auto& [id, w] : seen_)
      if (w.team != spawn_.team && w.round == round_ && w.head >= 0) enemies.push_back({w.head, id});
  for (int32_t t = 0; t < board_.size(); ++t) {
    const int32_t d = paths.dist[t];
    if (d <= 0) continue;
    int32_t ready;
    double score = TileValue(t, d, &ready) / (ready + 1);
    // another of ours is closer to it: leave it to them (a crowd on one pearl boxes itself in)
    if (score > 0 && p_.x1 > 0 && !champion)
      for (const int32_t a : allies)
        if (Torus(a, t) < (p_.x3 > 50 ? d : Torus(head_, t))) { score *= 1.0 - p_.x1; break; }
    if (score > 0 && !enemies.empty()) {
      const TileMemory& m = board_.at(t);
      const int32_t due = m.pearl ? 0 : std::max(0, m.countdown - (round_ - m.seen_round));
      for (const auto& [e, id] : enemies) {
        const int32_t theirs = std::max(Torus(e, t), due);
        if (theirs < ready || (theirs == ready && due > 0 && ready == due && id < spawn_.id)) {
          score *= p_.contest_factor;
          break;
        }
      }
    }
    for (int dir = 0; dir < 4; ++dir)
      if (board_.Step(t, dir) == kUnknown) {
        score = std::max(score, p_.explore_value / (d + 1));
        break;
      }
    // late: tiles not seen for a while are worth a look (the enemy's last dragons hide somewhere)
    if (p_.patrol_value > 0 && !leader_ && round_ >= p_.patrol_from && board_.at(t).seen_round >= 0 &&
        round_ - board_.at(t).seen_round > p_.patrol_age)
      score = std::max(score, p_.patrol_value / (d + 1));
    if (score > 0) candidates.push_back({t, score});
  }
  const size_t k = std::min<size_t>(candidates.size(), p_.trap_candidates);
  std::partial_sort(candidates.begin(), candidates.begin() + k, candidates.end(),
                    [](const Candidate& a, const Candidate& b) { return a.score > b.score; });
  // the best few, checked for dead ends: a dead end is worth its pocket's pearls less the
  // cost of splitting out of it
  const int32_t need = RoomNeeded();
  auto open = [&](int32_t t) { return !Occupied(t); };
  Goal best;
  double best_score = 0;
  std::vector<int32_t> pocket;
  for (size_t i = 0; i < k; ++i) {
    const Candidate& c = candidates[i];
    double score = c.score;
    bool trap = false;
    if (RoomAround(board_, c.tile, open, need, &pocket) < need) {
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
  return best;
}

// Shortest path of free, currently seen tiles ending on an enemy head worth trading with,
// 2..reach steps long; empty if none.
std::vector<int> StrategyAgent::StrikePath() const {
  const int32_t reach = std::min(length_ - 1, p_.strike_reach);
  std::vector<int32_t> frontier{head_}, tiles{head_};
  std::unordered_map<int32_t, std::pair<int32_t, int>> parent;   // tile -> (previous, dir)
  parent[head_] = {-1, -1};
  int32_t found = -1, found_len = -1;
  for (int32_t depth = 1; depth <= reach && found < 0; ++depth) {
    std::vector<int32_t> next;
    for (const int32_t t : frontier)
      for (int d = 0; d < 4; ++d) {
        const int32_t n = board_.Step(t, d);
        if (n < 0 || parent.count(n) || !board_.Current(n)) continue;
        const Cell c = Classify(n);
        if (c == Cell::kEnemyHead) {
          if (depth >= 2 && WorthTrading(board_.at(n).occupant)) {
            const auto it = seen_.find(board_.at(n).occupant);
            const int32_t len = it == seen_.end() ? 0 : it->second.length;
            if (len > found_len) { found = n; found_len = len; parent[n] = {t, d}; }
          }
          continue;
        }
        if (c != Cell::kFree) continue;
        parent[n] = {t, d};
        next.push_back(n);
      }
    frontier.swap(next);
  }
  std::vector<int> steps;
  if (found < 0) return steps;
  for (int32_t t = found; t != head_; t = parent[t].first) steps.push_back(parent[t].second);
  std::reverse(steps.begin(), steps.end());
  return steps;
}

// ---- split safety (from gather-split-trade-23) ---------------------------------------------------

// Can a dragon with this body (head first) get out? A timed flood from the head: an own
// segment is passable once the tail has moved off it. 2: reaches the unknown or follows its
// tail; 1: reaches `cap` tiles; 0: doomed. `blocked` tiles never free up in time.
int StrategyAgent::EscapeLevel(const std::vector<int32_t>& body, int32_t cap, const std::vector<int32_t>* blocked) const {
  const int32_t n = board_.size(), len = static_cast<int32_t>(body.size());
  std::vector<int32_t> index(n, -1);
  if (blocked)
    for (const int32_t t : *blocked) index[t] = 1 << 20;
  for (int32_t i = len - 1; i >= 0; --i) index[body[i]] = i;
  std::vector<int32_t> when(n, -1);
  std::vector<int32_t> queue{body[0]};
  when[body[0]] = 0;
  int32_t reached = 1;
  for (size_t qi = 0; qi < queue.size(); ++qi) {
    const int32_t t = queue[qi], m = when[t] + 1;
    for (int d = 0; d < 4; ++d) {
      const int32_t to = board_.Step(t, d);
      if (to == kUnknown) return 2;
      if (to < 0 || when[to] >= 0) continue;
      if (const int32_t i = index[to]; i >= 0) {
        if (i < len && i >= len - m + 1 && i > 0) return 2;
        continue;
      }
      if (board_.Current(to) && board_.at(to).occupant >= 0 && board_.at(to).occupant != spawn_.id) continue;
      when[to] = m;
      if (++reached >= cap) return 1;
      queue.push_back(to);
    }
  }
  return 0;
}

// Both halves of a split of the last n segments can get out.
bool StrategyAgent::SplitSafe(int32_t n) const {
  if (static_cast<int32_t>(body_.size()) < length_) return true;   // body not fully known
  std::vector<int32_t> parent(body_.begin(), body_.end() - n), child(body_.rbegin(), body_.rbegin() + n);
  return EscapeLevel(child, EscapeCap(n), &parent) >= 1 && EscapeLevel(parent, EscapeCap(length_ - n), &child) >= 1;
}

// ---- how to step ----------------------------------------------------------------------------------

// Free neighbours of `to` after our head moves there: our tail frees up unless we eat,
// our old head becomes body. A neighbour next to another dragon's head is contested and
// counts half (that dragon may step there first).
int32_t StrategyAgent::Liberties(int32_t to) const {
  const bool eats = board_.at(to).pearl;
  const int32_t tail = body_.empty() ? -1 : body_.back();
  int32_t libs = 0;
  for (int d = 0; d < 4; ++d) {
    const int32_t n = board_.Step(to, d);
    if (n == kUnknown) { libs += 2; continue; }
    if (n < 0 || n == head_) continue;
    const bool free_now = !Occupied(n) || (n == tail && !eats && length_ >= 2 && Mine(n));
    if (!free_now) continue;
    bool contested = false;
    for (int e = 0; e < 4 && !contested; ++e) {
      const int32_t m = board_.Step(n, e);
      if (m >= 0 && m != head_ && m != to) {
        const Cell c = Classify(m);
        contested = c == Cell::kAllyHead || c == Cell::kEnemyHead;
      }
    }
    libs += contested ? 1 : 2;
  }
  return libs;   // in halves
}

// (cs13) Every tile an enemy head seen now can reach in one MOVE: up to length - 1 free steps
// (its length as seen, a lower bound; at least 1). Nothing else moves during its turn.
void StrategyAgent::MarkThreats() {
  threat_.clear();
  if (!p_.guard_champion || round_ < p_.guard_from || length_ < p_.guard_min_length) return;
  const Report champ = Champion();
  if (champ.id != spawn_.id && length_ < p_.guard_share * champ.length) return;
  threat_.assign(board_.size(), 0);
  for (const auto& [id, w] : seen_) {
    if (w.team == spawn_.team || w.round != round_ || w.head < 0) continue;
    const int32_t reach = std::max(1, w.length - 1) + (w.cut ? p_.guard_cut_extra : 0);
    const Paths p = Search(board_, w.head, [&](int32_t t) { return !Occupied(t); }, reach);
    for (int32_t t = 0; t < board_.size(); ++t)
      if (p.dist[t] >= 0) threat_[t] = 1;
  }
}

bot::Action StrategyAgent::SafeStep(int preferred, bool trap_ok) {
  const int32_t need = RoomNeeded();
  // room to live in: free tiles, not counting those an enemy head can take next turn
  auto open = [&](int32_t t) { return !Occupied(t) && !Exposed(t); };
  int best = -1;
  long best_key = -1;
  // (cs13) a guarded champion takes only steps out of lunge reach when one is free
  bool guarded = false;
  if (!threat_.empty())
    for (int d = 0; d < 4 && !guarded; ++d) {
      const int32_t to = board_.Step(head_, d);
      guarded = Classify(to) == Cell::kFree && !threat_[to];
    }
  for (int k = 0; k < 4; ++k) {
    const int d = preferred >= 0 ? (preferred + k) % 4 : k;   // the preferred direction first (strategy-v1 never tried West without a goal)
    const int32_t to = board_.Step(head_, d);
    if (Classify(to) != Cell::kFree) continue;
    if (guarded && threat_[to]) continue;
    const int32_t room = RoomAround(board_, to, open, need);
    const bool exposed = Exposed(to);
    const int32_t libs = Liberties(to);
    if (k == 0 && preferred >= 0 && !exposed && (room >= need || trap_ok) && libs >= p_.min_libs)
      return Because(room >= need ? "toward goal" : "into a dead end worth it", bot::Action::Move(d));
    const long key = (exposed ? 0L : 1L) << 40 | (libs >= 2 ? 1L : 0L) << 39 | (long)std::min(room, need) << 8 | (long)std::min(libs, 255);
    if (best < 0 || key > best_key) {
      best = d;
      best_key = key;
    }
  }
  if (best >= 0) {
    if (guarded && (preferred < 0 || best != preferred || threat_[head_])) return Because("champion: out of reach", bot::Action::Move(best));
    return Because(preferred >= 0 ? "detour (goal step unsafe)" : "no goal: roomiest", bot::Action::Move(best));
  }
  // no free step. Split: the child's head is our tail, which points back the way we came
  // (usually open), so the child walks out with most of the length; the stub stays behind
  if (length_ >= 4 && units_ < spawn_.unit_limit) return Because("trapped: split", bot::Action::Split(length_ - 2));
  // else trade with an adjacent enemy head, else step through an unexplored portal
  for (int d = 0; d < 4; ++d)
    if (Classify(board_.Step(head_, d)) == Cell::kEnemyHead) return Because("trapped: trade", bot::Action::Move(d));
  for (int d = 0; d < 4; ++d)
    if (Classify(board_.Step(head_, d)) == Cell::kUnknown) return Because("trapped: unknown portal", bot::Action::Move(d));
  return Because("trapped: no move", bot::Action::Resign());
}

// ---- the turn --------------------------------------------------------------------------------------

bot::Action StrategyAgent::Turn(const rl::Observation& o) {
  bot::Action a = Decide(o);
  // tell the dragons around us who the champion is (a dead dragon's pings are not cast)
  const Report champ = Champion();
#ifdef CS15_DEBUG
  if (round_ >= 300) std::printf("CHAMP %d %d %d %d %s g%d c%d\n", round_, spawn_.team, spawn_.id, length_, reason_, (int)!threat_.empty(), champ.id);
#endif
  if (champ.id >= 0)
    for (int d = 0; d < 4; ++d) a.sonar[d] = Encode(champ);
  return a;
}

bot::Action StrategyAgent::Decide(const rl::Observation& o) {
  Remember(o);
  threat_.clear();

  // 1. feeding: late in the game every dragon but the champion (the longest ally the team
  // knows of, agreed over sonar) walks to it and dies next to it, leaving it pearls (a leader
  // only near the end, and only when it knows where it is)
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
      // (late-surge-6 via gst31) only feed a target clearly longer than us, by its length as known this turn
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

  // 2b. strike: a multi-step MOVE onto an enemy head 2..reach steps away (strategy-v1 never
  // looks further than one step, so it neither strikes like this nor keeps out of reach)
  if (p_.strike_reach >= 2 && length_ >= 3)
    if (auto steps = StrikePath(); !steps.empty()) {
      bot::Action a = bot::Action::Move(steps[0]);
      a.steps = steps;
      return Because("strike", a);
    }

  // 2c. the unit cap (from gather-split-trade-20): a short worker gives its slot (and pearls)
  // to a longer ally right next to it, so the slot goes to a productive split
  if (!leader_ && length_ <= p_.cap_feed_max_length && units_ >= spawn_.unit_limit - p_.cap_feed_slack &&
      round_ < p_.feed_from_round) {
    for (const auto& [id, w] : seen_) {
      if (id == spawn_.id || w.team != spawn_.team || w.round != round_ || w.head < 0 || w.length <= length_) continue;
      bool near = false;
      for (int d = 0; d < 4 && !near; ++d) {
        const int32_t n = board_.Step(head_, d);
        if (n < 0) continue;
        if (n == w.head) near = true;
        else if (!Occupied(n))
          for (int e = 0; e < 4 && !near; ++e) near = board_.Step(n, e) == w.head;
      }
      if (near) return Because("cap: feed ally", bot::Action::Resign());
    }
  }

  // 3. survival: our last dragons, outnumbered, keep away from enemy heads (losing them all loses the game)
  if (units_ <= p_.survivor_units && EnemiesSeen() > units_)
    if (const int flee = FleeDirection(); flee >= 0) {
      bot::Action a = SafeStep(flee, false);
      if (std::string(reason_) == "toward goal") reason_ = "survivor: flee";
      return a;
    }

  // 4. growth: split off a worker while there is time for it to gather
  if (length_ >= p_.split_min_length && round_ < (leader_ ? p_.leader_split_until : p_.split_until_round) &&
      units_ < spawn_.unit_limit &&
      !(round_ >= p_.champ_freeze && champ.id == spawn_.id) && (!p_.check_split || SplitSafe(length_ / 2)))
    return Because("split", bot::Action::Split(length_ / 2));

  // 5. a goal, and the first safe step towards it
  MarkThreats();
  // snipe: a big enemy dragon in reach is worth any of our small ones, late or not
  int32_t snipe = -1;
  if (!leader_ && p_.x2 > 0 && units_ >= 2)
    for (const auto& [id, w] : seen_)
      if (w.team != spawn_.team && round_ - w.round <= p_.snipe_memory && w.head >= 0 && paths.dist[w.head] > 0 &&
          paths.dist[w.head] <= p_.x3 && w.length >= p_.x2 && w.length >= p_.snipe_factor * length_ &&
          (snipe < 0 || paths.dist[w.head] < paths.dist[snipe]))
        snipe = w.head;
  Goal target = snipe >= 0 ? Goal{snipe, false}
                : ally_head >= 0 && paths.dist[ally_head] > 0 ? Goal{ally_head, false} : Goal{};
  // the champion's reported head is not reachable through the known map: head for the known
  // tile nearest to it (only while the report is fresh)
  if (target.tile < 0 && ally_head >= 0 && !in_view && round_ - champ.round <= p_.leader_feed_fresh &&
      Torus(ally_head, head_) > 2) {
    int32_t best = -1, best_cost = 1 << 30;
    for (int32_t t = 0; t < board_.size(); ++t)
      if (paths.dist[t] > 0) {
        const int32_t cost = 4 * Torus(t, ally_head) + paths.dist[t];
        if (cost < best_cost) { best_cost = cost; best = t; }
      }
    if (best >= 0 && Torus(best, ally_head) < Torus(head_, ally_head)) target = Goal{best, false};
  }
  if (target.tile < 0) target = ChooseGoal(paths);
  const int32_t goal = target.tile;
  const int preferred = goal >= 0 ? paths.first[goal] : -1;
  bot::Action step = SafeStep(preferred, target.trap);
  // the last step onto an enemy head we chose to hunt is a trade, not a mistake
  if (goal >= 0 && preferred >= 0 && board_.Step(head_, preferred) == goal && Classify(goal) == Cell::kEnemyHead &&
      WorthTrading(board_.at(goal).occupant))
    return Because("hunt: trade", bot::Action::Move(preferred));
  if (snipe >= 0 && std::string(reason_) == "toward goal") reason_ = "snipe";
  if (held && std::string(reason_) == "toward goal") reason_ = "held: peer not clearly longer";
  return step;
}

}  // namespace cs15
