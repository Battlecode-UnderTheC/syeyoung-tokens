#include "strategy.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace gst2 {

StrategyAgent::StrategyAgent(const rl::Spawn& spawn, Params params)
    : p_(params), spawn_(spawn), board_(spawn.width, spawn.height) {}

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
  if (t == strat::kBlocked) return Cell::kWall;
  if (t == strat::kUnknown) return Cell::kUnknown;
  if (!Occupied(t)) return Cell::kFree;
  if (Mine(t)) return Cell::kBody;
  const strat::TileMemory& m = board_.at(t);
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

// ---- where to go --------------------------------------------------------------------------------

// A pearl seen there is worth 1, discounted by how long ago (someone may have eaten it);
// a bed is worth 1 once its countdown runs out (rounds until then: *ready), a bed that
// ran out while out of view a bit less. Occupied tiles and unseen ones are worth nothing.
double StrategyAgent::TileValue(int32_t t, int32_t dist, int32_t* ready) const {
  const strat::TileMemory& m = board_.at(t);
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

StrategyAgent::Goal StrategyAgent::ChooseGoal(const strat::Paths& paths) const {
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
  for (int32_t t = 0; t < board_.size(); ++t) {
    const int32_t d = paths.dist[t];
    if (d <= 0) continue;
    int32_t ready;
    double score = TileValue(t, d, &ready) / (ready + 1);
    for (int dir = 0; dir < 4; ++dir)
      if (board_.Step(t, dir) == strat::kUnknown) {
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

// ---- how to step ----------------------------------------------------------------------------------

bot::Action StrategyAgent::SafeStep(int preferred, bool trap_ok) {
  const int32_t need = RoomNeeded();
  // room to live in: free tiles, not counting those an enemy head can take next turn
  auto open = [&](int32_t t) { return !Occupied(t) && !Exposed(t); };
  int best = -1, best_room = -1;
  bool best_exposed = true;
  for (int k = 0; k < 4; ++k) {
    const int d = (preferred + k) % 4;                 // the preferred direction first
    const int32_t to = board_.Step(head_, d);
    if (Classify(to) != Cell::kFree) continue;
    const int32_t room = RoomAround(board_, to, open, need);
    const bool exposed = Exposed(to);
    if (k == 0 && preferred >= 0 && !exposed && (room >= need || trap_ok))
      return Because(room >= need ? "toward goal" : "into a dead end worth it", bot::Action::Move(d));
    const bool better = best < 0 || (best_exposed && !exposed) ||
                        (best_exposed == exposed && room > best_room);
    if (better) {
      best = d;
      best_room = room;
      best_exposed = exposed;
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
  return Because("trapped: no move", bot::Action::Resign());
}

// ---- the turn --------------------------------------------------------------------------------------

bot::Action StrategyAgent::Turn(const rl::Observation& o) {
  Remember(o);

  // 1. feeding: late in the game a worker dies next to a bigger ally, leaving it pearls
  const Sighting* ally = !leader_ && round_ >= p_.feed_from_round ? BiggestAlly() : nullptr;
  const strat::Paths paths = strat::Search(board_, head_, [&](int32_t t) {
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
  const Goal target = ally && paths.dist[ally->head] > 0 ? Goal{ally->head, false} : ChooseGoal(paths);
  const int32_t goal = target.tile;
  const int preferred = goal >= 0 ? paths.first[goal] : -1;
  bot::Action a = SafeStep(preferred, target.trap);
  // the last step onto an enemy head we chose to hunt is a trade, not a mistake
  if (goal >= 0 && preferred >= 0 && board_.Step(head_, preferred) == goal && Classify(goal) == Cell::kEnemyHead &&
      WorthTrading(board_.at(goal).occupant))
    return Because("hunt: trade", bot::Action::Move(preferred));
  return a;
}

}  // namespace gst2
