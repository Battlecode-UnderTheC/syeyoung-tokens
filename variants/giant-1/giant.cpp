// #define GIANT_TUNING 1   // dev only: parameters from $GIANT, debug output to stderr
#include "giant.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "plugin.hpp"

namespace giant {

using strat::kBlocked;
using strat::kUnknown;

namespace {
constexpr int32_t kFar = 1 << 20;
}

#ifdef GIANT_TUNING
static Params FromEnv(Params p) {
  const char* e = std::getenv("GIANT");
  if (!e) return p;
  std::string s(e);
  size_t pos = 0;
  while (pos < s.size()) {
    size_t end = s.find(',', pos);
    if (end == std::string::npos) end = s.size();
    const std::string kv = s.substr(pos, end - pos);
    const size_t eq = kv.find('=');
    const std::string k = kv.substr(0, eq);
    const double v = std::atof(kv.c_str() + eq + 1);
#define P(name) if (k == #name) p.name = static_cast<decltype(p.name)>(v);
    P(pearl_decay) P(ripe_bed_value) P(explore_value) P(room_margin) P(room_min) P(threat_radius) P(evade_distance)
    P(feed_from_round) P(feed_max_length) P(giant_split_until) P(giant_child_length) P(helper_split_until) P(split_min_length)
    P(hunt_radius) P(hunt_min_units) P(helper_room_margin) P(helper_room_min) P(outnumbered_factor)
    P(king_ttl) P(promote_length) P(heir_by_sonar) P(avoid_giant_len) P(anchor_radius) P(strict_early) P(king_decay) P(demote_ratio) P(intercept_ahead) P(unknown_room) P(body_time_factor) P(tight_factor) P(feed_early_length) P(feed_distance)
#undef P
    pos = end + 1;
  }
  return p;
}
#else
static Params FromEnv(Params p) { return p; }
#endif

GiantAgent::GiantAgent(const rl::Spawn& spawn, Params params)
    : p_(FromEnv(params)), spawn_(spawn), board_(spawn.width, spawn.height) {}

void GiantAgent::Remember(const rl::Observation& o) {
  board_.Update(o);
  head_ = board_.Tile(o.head_x(), o.head_y());
  length_ = o.length;
  units_ = o.unit_count;
  round_ = o.round;
  std::vector<int32_t> visible;   // own segments in view, head to tail
  for (const rl::Segment& s : o.segments)
    if (s.id == spawn_.id) visible.push_back(board_.Tile(s.x, s.y));
  if (first_turn_) {
    first_turn_ = false;
    // alive at the start, or a long child (a giant that split its way out of a trap)
    // alive at the start, or the heir of a giant that split its way out of a trap (it pings us)
    bool heir = false;
    for (const uint64_t w : o.inbox)
      if (w == HeirWord()) heir = true;
    giant_ = o.round == 0 || (p_.heir_by_sonar ? heir : o.length >= p_.giant_child_length);
    body_.assign(visible.begin(), visible.end());
    blacklist_until_.assign(board_.size(), -1);
  }
  strict_ = giant_ && (round_ >= p_.giant_split_until || p_.strict_early);
  if (static_cast<int32_t>(visible.size()) == length_) body_.assign(visible.begin(), visible.end());
  if (body_.empty() || body_.front() != head_) body_.push_front(head_);
  while (static_cast<int32_t>(body_.size()) > length_) body_.pop_back();

  const int32_t n = board_.size();
  own_idx_.assign(n, -1);
  for (int32_t i = static_cast<int32_t>(body_.size()) - 1; i >= 0; --i) own_idx_[body_[i]] = i;
  for (int32_t i = 0; i < static_cast<int32_t>(visible.size()); ++i)   // what we see beats what we remember
    if (own_idx_[visible[i]] < 0) own_idx_[visible[i]] = i;   // its index is at least i
  other_.assign(n, 0);
  enemy_heads_.clear();
  ally_heads_.clear();
  trade_heads_.clear();
  seen_len_.clear();
  head_of_.clear();
  head_id_.assign(n, -1);
  for (const rl::Segment& s : o.segments)
    if (s.id != spawn_.id) {
      ++seen_len_[s.id];
      if (s.team != spawn_.team) enemy_seen_[s.id] = round_;
    }
  for (const rl::Segment& s : o.segments) {
    if (s.id == spawn_.id) continue;
    const int32_t t = board_.Tile(s.x, s.y);
    other_[t] = 1;
    if (s.head) {
      head_of_[s.id] = t;
      head_facing_[s.id] = s.facing;
      head_id_[t] = s.id;
      if (s.team != spawn_.team && Tradeable(t)) {   // not a threat: a trade we want
        trade_heads_.push_back(t);
        continue;
      }
      (s.team == spawn_.team ? ally_heads_ : enemy_heads_).push_back(t);
      // a helper that isn't feeding keeps out of a giant's way, as if its head were an enemy's
      if (s.team == spawn_.team && !giant_ && p_.avoid_giant_len > 0 && round_ < p_.feed_from_round &&
          (s.id == king_.id || seen_len_[s.id] >= p_.avoid_giant_len))
        enemy_heads_.push_back(t);
    }
  }
}

// ---- gossip ----------------------------------------------------------------------------------------

namespace {
constexpr uint64_t kMagic = 0xB1A7;
}

uint64_t GiantAgent::HeirWord() const { return (kMagic + 1) << 48 | static_cast<uint64_t>(spawn_.team & 1); }

uint64_t GiantAgent::Encode(const King& k) const {
  const uint64_t x = static_cast<uint64_t>(board_.X(k.tile)), y = static_cast<uint64_t>(board_.Y(k.tile));
  return kMagic << 48 | static_cast<uint64_t>(spawn_.team & 1) << 43 | static_cast<uint64_t>(k.id & 1023) << 33 |
         static_cast<uint64_t>(std::min(k.length, 1023)) << 23 | static_cast<uint64_t>(k.round & 511) << 14 | y << 7 | x;
}

bool GiantAgent::Decode(uint64_t w, King* k) const {
  if (w >> 48 != kMagic || static_cast<int32_t>((w >> 43) & 1) != (spawn_.team & 1)) return false;
  const int32_t x = w & 127, y = (w >> 7) & 127;
  if (x >= board_.width() || y >= board_.height()) return false;
  k->tile = board_.Tile(x, y);
  k->round = (w >> 14) & 511;
  k->length = (w >> 23) & 1023;
  k->id = (w >> 33) & 1023;
  return k->round <= round_;
}

void GiantAgent::Consider(const King& k) {
  if (k.id < 0 || round_ - k.round > p_.king_ttl) return;
  if (k.id == king_.id) {
    if (k.round >= king_.round) king_ = k;
    return;
  }
  // bigger wins, but a report goes stale (the giant may have died, or split into an heir)
  auto score = [&](const King& x) { return x.length - (round_ - x.round) * p_.king_decay; };
  if (!KingKnown() || score(k) > score(king_) || (score(k) == score(king_) && k.id < king_.id)) king_ = k;
}

int32_t GiantAgent::EnemiesSeen() const {
  int32_t k = 0;
  for (const auto& [id, r] : enemy_seen_)
    if (round_ - r <= p_.enemy_memory) ++k;
  return k;
}

bool GiantAgent::Tradeable(int32_t enemy_head) const {
  if (units_ < 2) return false;
  const int32_t id = head_id_[enemy_head];
  const auto it = seen_len_.find(id);
  const int32_t theirs = it == seen_len_.end() ? 0 : it->second;
  if (strict_) return length_ <= 2;
  if (giant_) return theirs > length_;   // still splitting: a leader, trades only up
  if (p_.outnumbered_factor > 0 && units_ < EnemiesSeen()) return theirs >= p_.outnumbered_factor * length_;
  return theirs >= length_;
}

// Steps each tile is from the nearest enemy head, through tiles free of dragons.
void GiantAgent::ComputeThreats() {
  enemy_dist_.assign(board_.size(), kFar);
  std::deque<int32_t> q;
  for (const int32_t h : enemy_heads_) {
    enemy_dist_[h] = 0;
    q.push_back(h);
  }
  while (!q.empty()) {
    const int32_t t = q.front();
    q.pop_front();
    if (enemy_dist_[t] >= p_.threat_radius) continue;
    for (int d = 0; d < 4; ++d) {
      const int32_t n = board_.Step(t, d);
      if (n < 0 || enemy_dist_[n] != kFar) continue;
      enemy_dist_[n] = enemy_dist_[t] + 1;
      if (other_[n] || own_idx_[n] >= 0) continue;   // reached, but not passed through
      q.push_back(n);
    }
  }
}

// Tiles reachable after stepping onto `to`, knowing our own segments move away in time:
// segment i of today's body is gone once enough further steps are taken.
int32_t GiantAgent::TimeRoom(int32_t to, bool grow, int32_t limit) const {
  const int32_t new_len = length_ + (grow ? 1 : 0);
  std::vector<int32_t> dist(board_.size(), -1);
  std::deque<int32_t> q{to};
  dist[to] = 0;
  int32_t count = 1;
  while (!q.empty() && count < limit) {
    const int32_t t = q.front();
    q.pop_front();
    for (int d = 0; d < 4; ++d) {
      const int32_t n = board_.Step(t, d);
      if (n == kUnknown) {   // unexplored: probably a way on, but maybe a dead end
        if (p_.unknown_room <= 0) return limit;
        count += p_.unknown_room;
        if (count >= limit) return limit;
        continue;
      }
      if (n < 0 || dist[n] >= 0 || other_[n]) continue;
      if (enemy_dist_[n] <= 1) continue;
      const int32_t i = own_idx_[n];
      if (i >= 0 && dist[t] + 1 < (new_len - i) * p_.body_time_factor) continue;
      dist[n] = dist[t] + 1;
      ++count;
      q.push_back(n);
    }
  }
  return std::min(count, limit);
}

int32_t GiantAgent::Torus(int32_t a, int32_t b) const {
  const int32_t dx = std::abs(board_.X(a) - board_.X(b)), dy = std::abs(board_.Y(a) - board_.Y(b));
  return std::min(dx, board_.width() - dx) + std::min(dy, board_.height() - dy);
}

int32_t GiantAgent::ChooseGoal(const std::vector<int32_t>& dist, int32_t anchor) const {
  int32_t best = -1;
  double best_score = 0;
  for (int32_t t = 0; t < board_.size(); ++t) {
    const int32_t d = dist[t];
    if (d <= 0 || blacklist_until_[t] > round_) continue;
    if (anchor >= 0 && Torus(t, anchor) > p_.anchor_radius) continue;
    const strat::TileMemory& m = board_.at(t);
    double score = 0;
    if (m.seen_round >= 0) {
      const int32_t age = round_ - m.seen_round;
      int32_t ready = d;
      double value = 0;
      if (m.pearl) {
        value = std::pow(p_.pearl_decay, age);
      } else if (m.countdown >= 0) {
        const int32_t due = m.countdown - age;
        if (due > 0) {
          ready = std::max(d, due);
          value = 1.0;
        } else {
          value = age == 0 ? 1.0 : p_.ripe_bed_value;
        }
      }
      if (enemy_dist_[t] <= 2) value *= 0.3;
      score = value / (ready + 1);
    }
    for (int dir = 0; dir < 4; ++dir)
      if (board_.Step(t, dir) == kUnknown) {
        score = std::max(score, p_.explore_value / (d + 1));
        break;
      }
    if (score > best_score) {
      best_score = score;
      best = t;
    }
  }
  return best;
}

bot::Action GiantAgent::Turn(const rl::Observation& o) {
  Remember(o);
  ComputeThreats();
  for (const uint64_t w : o.inbox) {
    King k;
    if (Decode(w, &k)) Consider(k);
  }
  if (giant_) Consider({spawn_.id, head_, round_, length_});
  // a long helper that no longer splits is a giant too (unless the king dwarfs it)
  if (!giant_ && p_.promote_length > 0 && length_ >= p_.promote_length && round_ >= p_.helper_split_until &&
      !(KingKnown() && length_ * p_.demote_ratio < king_.length)) {
    giant_ = true;
    demoted_ = false;
  }
  // a much smaller giant steps down: it gathers and later feeds the king
  if (giant_ && round_ >= p_.giant_split_until && KingKnown() && king_.id != spawn_.id &&
      length_ * p_.demote_ratio < king_.length) {
    giant_ = false;
    demoted_ = true;
  }
  if (const auto it = head_of_.find(king_.id); it != head_of_.end() && king_.id != spawn_.id)
    Consider({king_.id, it->second, round_, std::max(king_.length, seen_len_[king_.id])});
  const bot::Action a = Decide();
#ifdef GIANT_TUNING
  if (std::getenv("GIANT_DEBUG") && giant_ && std::string(reason_) == "trapped: split") {
    std::string why;
    for (int d = 0; d < 4; ++d) {
      const int32_t t = board_.Step(head_, d);
      why += t < 0 ? 'W' : own_idx_[t] >= 0 ? 'M' : other_[t] ? (board_.at(t).occupant_team == spawn_.team ? 'A' : 'E') : enemy_dist_[t] <= 1 ? 'L' : '?';
    }
    std::sort(why.begin(), why.end());
    std::fprintf(stderr, "TRAPCLASS %s r%d\n", why.c_str(), round_ / 100 * 100);
    std::fprintf(stderr, "TRAP r%d id%d len %d facing %d\n", round_, spawn_.id, length_, o.facing);
    for (int r = 0; r < 7; ++r) {
      for (int c = 0; c < 7; ++c) {
        const int k = r * 7 + c;
        const int32_t t = board_.Tile(o.tile_x[k], o.tile_y[k]);
        char ch = '.';
        if (o.pearl[k]) ch = 'o';
        if (own_idx_[t] >= 0) ch = own_idx_[t] == 0 ? '@' : (own_idx_[t] == length_ - 1 ? 't' : 'm');
        for (const rl::Segment& sg : o.segments)
          if (sg.id != spawn_.id && board_.Tile(sg.x, sg.y) == t) ch = sg.team == spawn_.team ? (sg.head ? 'A' : 'a') : (sg.head ? 'E' : 'e');
        const bool wl = o.vedge[r * 8 + c] == -2, wn = o.hedge[r * 7 + c] == -2;
        std::fprintf(stderr, "%c%c%c", wl ? '|' : ' ', ch, wn ? '^' : ' ');
      }
      std::fprintf(stderr, "\n");
    }
  }
  if (std::getenv("GIANT_DEBUG2") && !giant_ && round_ % 5 == 0 && round_ >= 300 && spawn_.team == 0)
    std::fprintf(stderr, "H r%d id%d at %d,%d len %d king %d at %d,%d len %d age %d dist %d %s\n", round_, spawn_.id, board_.X(head_), board_.Y(head_), length_, king_.id, board_.X(king_.tile), board_.Y(king_.tile), king_.length,
                 round_ - king_.round, king_.tile < 0 ? -1 : std::abs(board_.X(king_.tile) - board_.X(head_)) + std::abs(board_.Y(king_.tile) - board_.Y(head_)), reason_);
  if (std::getenv("GIANT_DEBUG") && (giant_ || length_ >= 10) && round_ % 25 == 0)
    std::fprintf(stderr, "r%d id%d team%d %s len %d king %d(%d) %s\n", round_, spawn_.id, spawn_.team, giant_ ? "G" : "h", length_, king_.id, king_.length, reason_);
#endif
  bot::Action out = a;
  if (KingKnown())
    for (int d = 0; d < 4; ++d) out.sonar[d] = Encode(king_);
  if (giant_ && a.kind == bot::Action::kSplit && a.split == length_ - 2)   // our child carries on as the giant
    for (int d = 0; d < 4; ++d) out.sonar[d] = HeirWord();
  return out;
}

bot::Action GiantAgent::Decide() {

  auto is_trade = [&](int32_t t) {
    return t >= 0 && std::find(trade_heads_.begin(), trade_heads_.end(), t) != trade_heads_.end();
  };
  // an enemy head next to ours, and the trade pays
  for (int d = 0; d < 4; ++d)
    if (is_trade(board_.Step(head_, d))) return Because("kamikaze", bot::Action::Move(d));

  // a helper gives its body to the giant: late in the game, or early when long enough
  int32_t feed_target = -1;       // the giant's head in view
  int32_t feed_walk = -1;         // else where it was last heard of
  const bool feeding = !giant_ && units_ >= 2 && KingKnown() && king_.id != spawn_.id &&
                       (round_ >= p_.feed_from_round ||
                        (p_.feed_early_length > 0 && round_ >= p_.giant_split_until && length_ >= p_.feed_early_length));
  if (feeding) {
    if (const auto it = head_of_.find(king_.id); it != head_of_.end())
      feed_target = it->second;
    else if (round_ >= p_.feed_from_round)
      feed_walk = king_.tile;
  }

  // numbers: split while there is time for the child to gather
  // (a giant never splits in round 0: every dragon acting in round 0 before any split is an original)
  if (!demoted_ && (giant_ ? round_ >= 1 && round_ < p_.giant_split_until : round_ < p_.helper_split_until) && length_ >= p_.split_min_length &&
      units_ < spawn_.unit_limit && feed_target < 0)
    return Because("split", bot::Action::Split(length_ / 2));

  // paths from the head, knowing our tail moves away
  const int32_t n = board_.size();
  std::vector<int32_t> dist(n, -1);
  std::vector<int8_t> first(n, -1);
  {
    std::deque<int32_t> q{head_};
    dist[head_] = 0;
    while (!q.empty()) {
      const int32_t t = q.front();
      q.pop_front();
      for (int d = 0; d < 4; ++d) {
        const int32_t m = board_.Step(t, d);
        if (m < 0 || dist[m] >= 0) continue;
        if (m == feed_target || is_trade(m)) {
          dist[m] = dist[t] + 1;
          first[m] = static_cast<int8_t>(t == head_ ? d : first[t]);
          continue;
        }
        if (other_[m] || enemy_dist_[m] <= 1) continue;
        const int32_t i = own_idx_[m];
        if (i >= 0 && dist[t] + 1 < length_ - i + 1) continue;
        dist[m] = dist[t] + 1;
        first[m] = static_cast<int8_t>(t == head_ ? d : first[t]);
        q.push_back(m);
      }
    }
  }
  auto torus = [&](int32_t a, int32_t b) {
    const int32_t dx = std::abs(board_.X(a) - board_.X(b)), dy = std::abs(board_.Y(a) - board_.Y(b));
    return std::min(dx, board_.width() - dx) + std::min(dy, board_.height() - dy);
  };
  // the reached tile nearest (on the torus) to `where`
  auto nearest_to = [&](int32_t where) {
    int32_t best = -1, best_d = torus(head_, where);
    for (int32_t t = 0; t < n; ++t)
      if (dist[t] > 0) {
        const int32_t d = torus(t, where);
        if (d < best_d || (d == best_d && best >= 0 && dist[t] < dist[best])) {
          best_d = d;
          best = t;
        }
      }
    return best;
  };
  if (feed_target >= 0 && torus(head_, feed_target) <= p_.feed_distance)
    return Because("feed giant", bot::Action::Resign());

  int32_t goal = -1;
  if (feed_target >= 0) {
    // meet it head on: aim a couple of steps in front of it
    int32_t ahead = feed_target;
    const int f = head_facing_[king_.id];
    for (int k = 0; k < p_.intercept_ahead; ++k)
      if (const int32_t nx = board_.Step(ahead, f); nx >= 0) ahead = nx;
    goal = ahead != feed_target && dist[ahead] > 0 ? ahead : nearest_to(feed_target);
  } else if (feed_walk >= 0) {
    goal = nearest_to(feed_walk);
  }
  const bool to_giant = goal >= 0 && (goal == feed_target || feed_walk >= 0);
  if (goal < 0 && !giant_ && units_ >= p_.hunt_min_units && round_ < p_.feed_from_round)
    for (const int32_t h : trade_heads_)
      if (dist[h] > 0 && dist[h] <= p_.hunt_radius && (goal < 0 || dist[h] < dist[goal])) goal = h;
  if (goal >= 0 && is_trade(goal) && dist[goal] == 1) return Because("hunt: trade", bot::Action::Move(first[goal]));
  // the king stays near one spot late in the game, so its helpers can find it
  if (giant_ && king_.id == spawn_.id && p_.anchor_radius > 0 && round_ >= p_.feed_from_round) {
    if (anchor_ < 0) anchor_ = head_;
  } else {
    anchor_ = -1;
  }
  if (goal < 0 && anchor_ >= 0) goal = ChooseGoal(dist, anchor_);
  if (goal < 0) goal = ChooseGoal(dist, -1);
  const int preferred = goal >= 0 ? first[goal] : -1;

  struct Cand {
    int d;
    int32_t to, room, edist;
    bool lethal;
  };
  std::vector<Cand> cands;
  const int32_t need = strict_ ? std::max(length_ + p_.room_margin, p_.room_min)
                              : std::max(length_ + p_.helper_room_margin, p_.helper_room_min);
  bool unknown_step[4] = {false, false, false, false};
  for (int d = 0; d < 4; ++d) {
    const int32_t to = board_.Step(head_, d);
    if (to == kUnknown) unknown_step[d] = true;
    if (to < 0 || own_idx_[to] >= 0 || other_[to]) continue;
    const bool grow = board_.Current(to) && board_.at(to).pearl;
    const int32_t edist = enemy_dist_[to];
    cands.push_back({d, to, TimeRoom(to, grow, need), edist, edist <= 1});
  }

  // a portal exit out of view may hold a part of our body we don't know of (a child knows only
  // what it saw): with a step in view that's not in reach of an enemy, don't gamble on it
  {
    bool seen_ok = false;
    for (const Cand& c : cands)
      if (board_.Current(c.to) && !c.lethal) seen_ok = true;
    if (seen_ok)
      cands.erase(std::remove_if(cands.begin(), cands.end(), [&](const Cand& c) { return !board_.Current(c.to); }),
                  cands.end());
  }
  const Cand* best = nullptr;
  bool evade = false;
  for (const Cand& c : cands)
    if (c.edist < p_.evade_distance) evade = true;
  auto capped = [&](const Cand& c) { return std::min(c.edist, p_.evade_distance + 1); };
  for (const Cand& c : cands) {
    if (c.lethal || c.room < need) continue;
    if (!evade && c.d == preferred) return Because(to_giant ? "toward giant" : "toward goal", bot::Action::Move(c.d));
    if (!best) {
      best = &c;
      continue;
    }
    if (evade) {
      if (capped(c) != capped(*best)) {
        if (capped(c) > capped(*best)) best = &c;
        continue;
      }
      if ((c.d == preferred) != (best->d == preferred)) {
        if (c.d == preferred) best = &c;
        continue;
      }
    }
    if (c.room > best->room) best = &c;
  }
  if (best) {
    if (!evade && goal >= 0 && preferred >= 0) blacklist_until_[goal] = round_ + 20;
    const char* why = evade ? "evade" : preferred < 0 ? "roomiest" : "detour: gone";
    for (const Cand& c : cands)
      if (!evade && c.d == preferred) why = c.lethal ? "detour: lethal" : "detour: room";
    return Because(why, bot::Action::Move(best->d));
  }

  // no step is both out of reach and roomy
  const Cand* roomiest = nullptr;
  for (const Cand& c : cands)
    if (!c.lethal && (!roomiest || c.room > roomiest->room)) roomiest = &c;
  const bool can_split = length_ >= 5 && units_ < spawn_.unit_limit;
  if (roomiest && (roomiest->room * p_.tight_factor >= need || !can_split))
    return Because("tight: roomiest", bot::Action::Move(roomiest->d));
  if (can_split) return Because("trapped: split", bot::Action::Split(length_ - 2));
  if (!cands.empty()) {
    const Cand* c = &cands[0];
    for (const Cand& x : cands)
      if (x.room > c->room) c = &x;
    return Because("cornered", bot::Action::Move(c->d));
  }
  for (int d = 0; d < 4; ++d) {
    const int32_t to = board_.Step(head_, d);
    if (to >= 0 && std::find(enemy_heads_.begin(), enemy_heads_.end(), to) != enemy_heads_.end())
      return Because("trapped: trade", bot::Action::Move(d));
  }
  for (int d = 0; d < 4; ++d)
    if (unknown_step[d]) return Because("trapped: unknown portal", bot::Action::Move(d));
  if (length_ >= 4 && units_ < spawn_.unit_limit) return Because("trapped: split", bot::Action::Split(length_ - 2));
  return Because("trapped: no move", bot::Action::Resign());
}

}  // namespace giant

LEAGUE_VARIANT(giant::GiantAgent)
