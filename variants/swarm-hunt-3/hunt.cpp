#include "hunt.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace swh3 {

using strat::kBlocked;
using strat::kUnknown;
using strat::Paths;
using strat::RoomAround;
using strat::Search;
using strat::TileMemory;

const Params& DefaultParams() {
  static const Params params;
  return params;
}

HuntAgent::HuntAgent(const rl::Spawn& spawn, const Params& params)
    : p_(params), spawn_(spawn), board_(spawn.width, spawn.height) {}

// ---- what this dragon sees ------------------------------------------------------------------------

void HuntAgent::Remember(const rl::Observation& o) {
  board_.Update(o);
  head_ = board_.Tile(o.head_x(), o.head_y());
  length_ = o.length;
  units_ = o.unit_count;
  round_ = o.round;
  if (first_turn_) {
    // map dragons see no ECHOES on their first turn; split children inherit PROTOCOL 3 and do
    keeper_ = o.round == 0 && !o.echo_present;
    first_turn_ = false;
    for (const rl::Segment& s : o.segments)
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
  for (const auto& [id, n] : segments) {
    Sighting& w = seen_[id];
    // a lower bound: only the segments in view are counted
    w.length = w.round == round_ ? std::max(n, 0) : n;
  }
}

bool HuntAgent::Mine(int32_t t) const { return std::find(body_.begin(), body_.end(), t) != body_.end(); }

bool HuntAgent::Occupied(int32_t t) const { return Mine(t) || (board_.Current(t) && board_.at(t).occupant >= 0); }

HuntAgent::Cell HuntAgent::Classify(int32_t t) const {
  if (t == kBlocked) return Cell::kWall;
  if (t == kUnknown) return Cell::kUnknown;
  if (!Occupied(t)) return Cell::kFree;
  if (Mine(t)) return Cell::kBody;
  const TileMemory& m = board_.at(t);
  if (!m.occupant_head || m.occupant == spawn_.id) return Cell::kBody;
  return m.occupant_team == spawn_.team ? Cell::kAllyHead : Cell::kEnemyHead;
}

bool HuntAgent::WorthTrading(int32_t enemy_id) const {
  if (units_ <= 1) return false;
  const auto it = seen_.find(enemy_id);
  const int32_t theirs = it == seen_.end() ? 0 : it->second.length;
  if (keeper_) return theirs > length_;
  switch (p_.trade_rule) {
    case 1: return true;
    case 2: return units_ >= p_.hunt_min_units || theirs >= length_ + p_.trade_margin;
    default: return theirs >= length_ + p_.trade_margin;
  }
}

bool HuntAgent::Exposed(int32_t t) const {
  if (keeper_ && !threat_.empty()) return threat_[t] != 0;
  if (!p_.avoid_heads) return false;
  for (int d = 0; d < 4; ++d)
    if (const int32_t n = board_.Step(t, d);
        n >= 0 && Classify(n) == Cell::kEnemyHead && !WorthTrading(board_.at(n).occupant))
      return true;
  return false;
}

const HuntAgent::Sighting* HuntAgent::BiggestAlly() const {
  const Sighting* best = nullptr;
  for (const auto& [id, w] : seen_)
    if (w.team == spawn_.team && w.head >= 0 && round_ - w.round <= p_.ally_memory && w.length > length_ &&
        (!best || w.length > best->length))
      best = &w;
  return best;
}

int32_t HuntAgent::TorusDistance(int32_t a, int32_t b) const {
  const int32_t dx = std::abs(board_.X(a) - board_.X(b)), dy = std::abs(board_.Y(a) - board_.Y(b));
  return std::min(dx, board_.width() - dx) + std::min(dy, board_.height() - dy);
}

bool HuntAgent::AllyCloser(int32_t t, int32_t dist) const {
  for (const int32_t h : ally_heads_)
    if (TorusDistance(h, t) < dist) return true;
  return false;
}

// ---- where to go --------------------------------------------------------------------------------

double HuntAgent::TileValue(int32_t t, int32_t dist, int32_t* ready) const {
  const TileMemory& m = board_.at(t);
  *ready = dist;
  if (m.seen_round < 0 || Occupied(t)) return 0.0;
  const int32_t age = round_ - m.seen_round;
  if (m.pearl) return std::pow(p_.pearl_decay, age);
  if (m.countdown < 0) return 0.0;
  const int32_t due = m.countdown - age;
  if (due > 0) {
    *ready = std::max(dist, due);
    return 1.0;
  }
  return age == 0 ? 1.0 : p_.ripe_bed_value;
}

int32_t HuntAgent::RoomNeeded() const { return std::max(length_ + p_.room_margin, p_.room_min); }

HuntAgent::Goal HuntAgent::ChooseGoal(const Paths& paths) const {
  // hunting: close in on an enemy head worth trading for, until a lunge reaches it
  if (!keeper_ && units_ >= p_.hunt_min_units && round_ < p_.feed_from_round) {
    int32_t best = -1;
    for (const auto& [id, w] : seen_)
      if (w.team != spawn_.team && w.round == round_ && w.head >= 0 && paths.dist[w.head] > 0 &&
          paths.dist[w.head] <= p_.hunt_radius && WorthTrading(id) &&
          (best < 0 || paths.dist[w.head] < paths.dist[best]))
        best = w.head;
    if (best >= 0) return {best, false};
  }
  struct Candidate {
    int32_t tile;
    double score;
  };
  std::vector<Candidate> candidates;
  for (int32_t t = 0; t < board_.size(); ++t) {
    const int32_t d = paths.dist[t];
    if (d <= 0) continue;
    if (p_.ally_yield && AllyCloser(t, d)) continue;
    int32_t ready;
    double score = TileValue(t, d, &ready) / (ready + 1);
    for (int dir = 0; dir < 4; ++dir)
      if (board_.Step(t, dir) == kUnknown) {
        score = std::max(score, p_.explore_value / (d + 1));
        break;
      }
    if (score > 0 && p_.crowd_penalty > 0) score /= 1.0 + p_.crowd_penalty * CrowdNear(t);
    if (score > 0) candidates.push_back({t, score});
  }
  const size_t k = std::min<size_t>(candidates.size(), p_.trap_candidates);
  std::partial_sort(candidates.begin(), candidates.begin() + k, candidates.end(),
                    [](const Candidate& a, const Candidate& b) { return a.score > b.score; });
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

// ---- the lunge ------------------------------------------------------------------------------------

// Breadth-first over free tiles, at most length-1 steps: the nearest enemy head worth a
// trade, and the steps there. Nothing else moves during our turn, so the strike lands.
bool HuntAgent::Lunge(bot::Action* out) {
  const int32_t reach = std::min<int32_t>(length_ - 1, 6);
  if (reach < 1) return false;
  std::unordered_map<int32_t, std::pair<int32_t, int8_t>> from;   // tile -> (previous tile, step)
  std::vector<std::pair<int32_t, int32_t>> frontier{{head_, 0}};
  from[head_] = {-1, -1};
  for (size_t q = 0; q < frontier.size(); ++q) {
    const auto [t, dist] = frontier[q];
    if (dist >= reach) continue;
    for (int d = 0; d < 4; ++d) {
      const int32_t n = board_.Step(t, d);
      if (n < 0 || from.count(n)) continue;
      const Cell c = Classify(n);
      if (c == Cell::kEnemyHead && WorthTrading(board_.at(n).occupant)) {
        std::vector<int> steps{d};
        for (int32_t u = t; from[u].first >= 0; u = from[u].first) steps.push_back(from[u].second);
        std::reverse(steps.begin(), steps.end());
        *out = bot::Action{bot::Action::kMove, steps, 0, false, {}};
        return true;
      }
      if (c != Cell::kFree) continue;
      from[n] = {t, static_cast<int8_t>(d)};
      frontier.push_back({n, dist + 1});
    }
  }
  return false;
}

int32_t HuntAgent::CrowdNear(int32_t t) const {
  int32_t n = ally_body_[t];
  for (int d = 0; d < 4; ++d) {
    const int32_t a = board_.Step(t, d);
    if (a < 0) continue;
    n += ally_body_[a];
    for (int e = 0; e < 4; ++e)
      if (const int32_t b = board_.Step(a, e); b >= 0 && b != t) n += ally_body_[b];
  }
  return n;
}

// ---- how to step ----------------------------------------------------------------------------------

bot::Action HuntAgent::SafeStep(int preferred, bool trap_ok) {
  const int32_t need = RoomNeeded();
  // ally tails move away before we could get there: count them as room (never as a step)
  auto open = [&](int32_t t) {
    return (!Occupied(t) || ally_tail_[t]) && !Exposed(t) && !(p_.crowd_room && crowd_[t]);
  };
  if (length_ <= 3) trap_ok = false;   // too short to split out of a dead end
  int best = -1, best_room = -1, best_crowd = 0;
  bool best_exposed = true;
  for (int k = 0; k < 4; ++k) {
    const int d = (preferred + 4 + k) % 4;
    const int32_t to = board_.Step(head_, d);
    if (Classify(to) != Cell::kFree) continue;
    const int32_t room = RoomAround(board_, to, open, need);
    const bool exposed = Exposed(to);
    if (k == 0 && preferred >= 0 && !exposed && (room >= need || trap_ok))
      return Because(room >= need ? "toward goal" : "into a dead end worth it", bot::Action::Move(d));
    const int32_t crowd = CrowdNear(to);
    const bool better = best < 0 || (best_exposed && !exposed) ||
                        (best_exposed == exposed && (room > best_room || (room == best_room && crowd < best_crowd)));
    if (better) {
      best = d;
      best_room = room;
      best_exposed = exposed;
      best_crowd = crowd;
    }
  }
  if (best >= 0) return Because(preferred >= 0 ? "detour (goal step unsafe)" : "no goal: roomiest", bot::Action::Move(best));
  if (length_ >= 4 && units_ < spawn_.unit_limit) return Because("trapped: split", bot::Action::Split(length_ - 2));
  for (int d = 0; d < 4; ++d)
    if (Classify(board_.Step(head_, d)) == Cell::kEnemyHead) return Because("trapped: trade", bot::Action::Move(d));
  for (int d = 0; d < 4; ++d)
    if (Classify(board_.Step(head_, d)) == Cell::kUnknown) return Because("trapped: unknown portal", bot::Action::Move(d));
  return Because("trapped: no move", bot::Action::Resign());
}

// ---- the turn --------------------------------------------------------------------------------------

bot::Action HuntAgent::Turn(const rl::Observation& o) {
  Remember(o);

  // allies' heads this turn: the tiles next to them may be taken before we get there
  ally_heads_.clear();
  crowd_.assign(board_.size(), 0);
  ally_body_.assign(board_.size(), 0);
  ally_tail_.assign(board_.size(), 0);
  for (size_t i = 0; i < o.segments.size(); ++i) {
    const rl::Segment& s = o.segments[i];
    if (s.id == spawn_.id || s.team != spawn_.team) continue;
    const int32_t t = board_.Tile(s.x, s.y);
    ally_body_[t] = 1;
    // the last segment listed for a dragon whose head is in view: its tail, gone after its move
    if (!s.head && (i + 1 == o.segments.size() || o.segments[i + 1].id != s.id)) ally_tail_[t] = 1;
  }
  for (const auto& [id, w] : seen_)
    if (w.team == spawn_.team && w.round == round_ && w.head >= 0) {
      ally_heads_.push_back(w.head);
      for (int d = 0; d < 4; ++d)
        if (const int32_t n = board_.Step(w.head, d); n >= 0) crowd_[n] = 1;
    }

  // keepers: tiles an enemy head could strike this coming turn
  threat_.clear();
  if (keeper_ && p_.keeper_guard > 1) {
    threat_.assign(board_.size(), 0);
    for (const auto& [id, w] : seen_) {
      if (w.team == spawn_.team || w.round != round_ || w.head < 0 || WorthTrading(id)) continue;
      const int32_t reach = std::max(1, std::min(p_.keeper_guard, w.length - 1));
      const Paths near = Search(board_, w.head, [&](int32_t t) { return !Occupied(t); }, reach);
      for (int32_t t = 0; t < board_.size(); ++t)
        if (near.dist[t] > 0) threat_[t] = 1;
    }
  }

  const Sighting* ally = !keeper_ && round_ >= p_.feed_from_round ? BiggestAlly() : nullptr;
  const Paths paths = Search(board_, head_, [&](int32_t t) {
    return !Occupied(t) || Classify(t) == Cell::kEnemyHead || (ally && t == ally->head);
  });
  if (ally && paths.dist[ally->head] >= 0 && paths.dist[ally->head] <= p_.feed_distance)
    return Because("feed ally", bot::Action::Resign());

  // strike: an adjacent head, or one in lunge reach
  for (int d = 0; d < 4; ++d) {
    const int32_t to = board_.Step(head_, d);
    if (Classify(to) == Cell::kEnemyHead && WorthTrading(board_.at(to).occupant))
      return Because("strike", bot::Action::Move(d));
  }
  if (p_.lunge) {
    bot::Action a;
    if (Lunge(&a)) return Because("lunge", a);
  }

  const int32_t split_until = keeper_ ? p_.keeper_split_until : p_.split_until_round;
  if (length_ >= p_.split_min_length && round_ < split_until && units_ < spawn_.unit_limit - p_.unit_reserve) {
    // split only with room for both halves: children born into a crowd box each other in
    const int32_t need = p_.split_room * RoomNeeded();
    auto open = [&](int32_t t) { return !Occupied(t) && !crowd_[t]; };
    const int32_t tail = body_.empty() ? head_ : body_.back();
    if (p_.split_room <= 0 || static_cast<int32_t>(body_.size()) < length_ || (RoomAround(board_, head_, open, need) >= need &&
                                                         RoomAround(board_, tail, open, need) >= need))
      return Because("split", bot::Action::Split(length_ / 2));
  }

  const Goal target = ally && paths.dist[ally->head] > 0 ? Goal{ally->head, false} : ChooseGoal(paths);
  const int32_t goal = target.tile;
  const int preferred = goal >= 0 ? paths.first[goal] : -1;
  bot::Action a = SafeStep(preferred, target.trap);
  if (goal >= 0 && preferred >= 0 && board_.Step(head_, preferred) == goal && Classify(goal) == Cell::kEnemyHead &&
      WorthTrading(board_.at(goal).occupant))
    return Because("hunt: trade", bot::Action::Move(preferred));
  return a;
}

}  // namespace swh3
