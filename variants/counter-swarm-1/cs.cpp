#define CS_TUNE 1
#include <type_traits>
#include <cstdio>
#include <cstdlib>
#include "cs.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace cs {

StrategyAgent::StrategyAgent(const rl::Spawn& spawn, Params params)
    : p_(params), spawn_(spawn), board_(spawn.width, spawn.height) {
#ifdef CS_TUNE
  auto env = [](const char* k, auto& v) { if (const char* e = getenv(k)) v = static_cast<std::remove_reference_t<decltype(v)>>(atof(e)); };
  env("P_SPLIT", p_.split_min_length); env("P_SPLITR", p_.split_until_round); env("P_FEED", p_.feed_from_round);
  env("P_LIBS", p_.min_libs); env("P_MARGIN", p_.trade_margin); env("P_HUNT", p_.hunt_radius); env("P_HUNTU", p_.hunt_min_units);
  env("P_ROOMM", p_.room_margin); env("P_ROOMMIN", p_.room_min); env("P_TRAP", p_.trap_cost); env("P_REACH", p_.strike_reach); env("P_X1", p_.x1); env("P_X2", p_.x2); env("P_X3", p_.x3);
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
    w.round = round_;
    if (s.head) w.head = board_.Tile(s.x, s.y);
  }
  for (const auto& [id, n] : segments) seen_[id].length = n;
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

const StrategyAgent::Sighting* StrategyAgent::BiggestAlly() const {
  const Sighting* best = nullptr;
  for (const auto& [id, w] : seen_)
    if (w.team == spawn_.team && w.head >= 0 && round_ - w.round <= p_.ally_memory && w.length > length_ &&
        (!best || w.length > best->length))
      best = &w;
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
  if (!leader_ && units_ >= p_.hunt_min_units && round_ < p_.feed_from_round) {
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
  std::vector<int32_t> allies;   // ally heads in view now
  for (const auto& [id, w] : seen_)
    if (w.team == spawn_.team && w.round == round_ && w.head >= 0) allies.push_back(w.head);
  for (int32_t t = 0; t < board_.size(); ++t) {
    const int32_t d = paths.dist[t];
    if (d <= 0) continue;
    int32_t ready;
    double score = TileValue(t, d, &ready) / (ready + 1);
    // another of ours is closer to it: leave it to them (a crowd on one pearl boxes itself in)
    if (score > 0 && p_.x1 > 0)
      for (const int32_t a : allies)
        if (Torus(a, t) < (p_.x3 > 50 ? d : Torus(head_, t))) { score *= 1.0 - p_.x1; break; }
    for (int dir = 0; dir < 4; ++dir)
      if (board_.Step(t, dir) == kUnknown) {
        score = std::max(score, p_.explore_value / (d + 1));
        break;
      }
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

bot::Action StrategyAgent::SafeStep(int preferred, bool trap_ok) {
  const int32_t need = RoomNeeded();
  // room to live in: free tiles, not counting those an enemy head can take next turn
  auto open = [&](int32_t t) { return !Occupied(t) && !Exposed(t); };
  int best = -1;
  long best_key = -1;
  for (int k = 0; k < 4; ++k) {
    const int d = preferred >= 0 ? (preferred + k) % 4 : k;   // the preferred direction first (strategy-v1 never tried West without a goal)
    const int32_t to = board_.Step(head_, d);
    if (Classify(to) != Cell::kFree) continue;
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
  if (best >= 0) return Because(preferred >= 0 ? "detour (goal step unsafe)" : "no goal: roomiest", bot::Action::Move(best));
  // no free step. Split: the child's head is our tail, which points back the way we came
  // (usually open), so the child walks out with most of the length; the stub stays behind
  if (length_ >= 4 && units_ < spawn_.unit_limit) return Because("trapped: split", bot::Action::Split(length_ - 2));
  // else trade with an adjacent enemy head, else step through an unexplored portal
  for (int d = 0; d < 4; ++d)
    if (Classify(board_.Step(head_, d)) == Cell::kEnemyHead) return Because("trapped: trade", bot::Action::Move(d));
  for (int d = 0; d < 4; ++d)
    if (Classify(board_.Step(head_, d)) == Cell::kUnknown) return Because("trapped: unknown portal", bot::Action::Move(d));
  if (getenv("CSDBG")) {
    fprintf(stderr, "TRAP id=%d r=%d len=%d leader=%d head=(%d,%d) body=", spawn_.id, round_, length_, leader_, board_.X(head_), board_.Y(head_));
    for (auto t : body_) fprintf(stderr, "(%d,%d)", board_.X(t), board_.Y(t));
    fprintf(stderr, " nb=");
    for (int d = 0; d < 4; ++d) { int32_t n = board_.Step(head_, d); fprintf(stderr, "%d:%d ", n, (int)Classify(n)); }
    fprintf(stderr, "\n");
    for (int dy = -3; dy <= 3; ++dy) {
      for (int dx = -3; dx <= 3; ++dx) {
        int32_t t = board_.Tile(board_.X(head_) + dx, board_.Y(head_) + dy);
        const TileMemory& m = board_.at(t);
        char c = '.';
        if (Mine(t)) c = t == head_ ? '@' : 'o';
        else if (Occupied(t)) c = m.occupant_team == spawn_.team ? (m.occupant_head ? 'A' : 'a') : (m.occupant_head ? 'E' : 'e');
        else if (m.pearl) c = '*';
        fprintf(stderr, "%c", c);
      }
      fprintf(stderr, "\n");
    }
  }
  return Because("trapped: no move", bot::Action::Resign());
}

// ---- the turn --------------------------------------------------------------------------------------

bot::Action StrategyAgent::Turn(const rl::Observation& o) {
  Remember(o);

  // 1. feeding: late in the game a worker dies next to a bigger ally, leaving it pearls
  const Sighting* ally = !leader_ && round_ >= p_.feed_from_round ? BiggestAlly() : nullptr;
  const Paths paths = Search(board_, head_, [&](int32_t t) {
    return !Occupied(t) || Classify(t) == Cell::kEnemyHead || (ally && t == ally->head);
  });
  if (ally && paths.dist[ally->head] >= 0 && paths.dist[ally->head] <= p_.feed_distance)
    return Because("feed ally", bot::Action::Resign());

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

  // 3. survival: our last dragons, outnumbered, keep away from enemy heads (losing them all loses the game)
  if (units_ <= p_.survivor_units && EnemiesSeen() > units_)
    if (const int flee = FleeDirection(); flee >= 0) {
      bot::Action a = SafeStep(flee, false);
      if (std::string(reason_) == "toward goal") reason_ = "survivor: flee";
      return a;
    }

  // 4. growth: split off a worker while there is time for it to gather
  if (length_ >= p_.split_min_length && round_ < p_.split_until_round && units_ < spawn_.unit_limit)
    return Because("split", bot::Action::Split(length_ / 2));

  // 5. a goal, and the first safe step towards it
  // snipe: a big enemy dragon in reach is worth any of our small ones, late or not
  int32_t snipe = -1;
  if (!leader_ && p_.x2 > 0 && units_ >= 2)
    for (const auto& [id, w] : seen_)
      if (w.team != spawn_.team && w.round == round_ && w.head >= 0 && paths.dist[w.head] > 0 &&
          paths.dist[w.head] <= p_.x3 && w.length >= p_.x2 && w.length >= 2 * length_ &&
          (snipe < 0 || paths.dist[w.head] < paths.dist[snipe]))
        snipe = w.head;
  const Goal target = snipe >= 0 ? Goal{snipe, false}
                      : ally && paths.dist[ally->head] > 0 ? Goal{ally->head, false} : ChooseGoal(paths);
  const int32_t goal = target.tile;
  const int preferred = goal >= 0 ? paths.first[goal] : -1;
  bot::Action a = SafeStep(preferred, target.trap);
  // the last step onto an enemy head we chose to hunt is a trade, not a mistake
  if (goal >= 0 && preferred >= 0 && board_.Step(head_, preferred) == goal && Classify(goal) == Cell::kEnemyHead &&
      WorthTrading(board_.at(goal).occupant))
    return Because("hunt: trade", bot::Action::Move(preferred));
  return a;
}

}  // namespace cs
