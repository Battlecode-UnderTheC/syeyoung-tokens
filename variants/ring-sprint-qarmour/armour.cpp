// g46-4 (ring-sprint-qarmour): queen armour, the lunge guard of the workers ported to the queen.
//
// Observed (strategist g46 C, 279 server losses): 29 losses lost the queen after r430 while ahead;
// 19 of them were queens of 5-58 killed by 5-21-long enemies lunging from 2-3 tiles, her last moves
// 4-6-step sprints along feed lines. In ring-sprint-21's own arena replays of such deaths
// (214505, 214519, 214540: the killer's body was in her view and its head was not; 214517: its
// head was in view 3 tiles away) her endpoint lay inside the killer's reach by the rules' cost.
// QueenRisk capped every enemy's reach at 8 and took a body-only sighting as no threat at all.
//
// From kRaceFrom (when she leaves her ring to grow, and where those deaths are):
//  - an enemy head in view reaches ceil(Le/4) + Le - 2 tiles (the rules: free steps plus one paid
//    step per segment above 2), at least 3 as before (a seen length is a lower bound); a head whose
//    body leaves the view, and a body seen without its head, are of unknown length: unbounded reach
//    within view (the head of a headless body is at least one step beyond its frontmost segment, and
//    a split can turn its tail into a head that acts this very round);
//  - QueenRisk returns this field, so her whole move, sprints included, is judged by where it ends
//    (she moves first in the round: id 0/1);
//  - a feeder lays its line only if none of its pearl tiles is in that reach (conveyor.cpp);
//  - she stops chasing pearls once growth can't change the result (GrowthMoot).
#include "strategy.hpp"

#include <algorithm>
#include <limits>

namespace gm2 {

void StrategyAgent::CollectArmour(const rl::Observation& o) {
  armour_round_ = round_;
  armour_src_.clear();
  for (const auto& [id, w] : seen_)
    if (w.team != spawn_.team && w.round == round_) enemy_len_max_ = std::max(enemy_len_max_, w.length);
  if (!QARMOUR || round_ < kRaceFrom) return;
  constexpr int32_t kUnbounded = std::numeric_limits<int32_t>::max() / 2;
  int32_t last = -1;
  for (const auto& s : o.segments) {
    if (s.team == spawn_.team || int32_t(s.id) == last) continue;
    last = s.id;   // segments come by dragon id, head to tail: this is its frontmost one in view
    const int32_t t = board_.Tile(s.x, s.y);
    if (t < 0) continue;
    if (s.head) {
      const auto it = seen_.find(s.id);
      const bool partial = it == seen_.end() || it->second.partial;
      const int32_t le = it == seen_.end() ? 2 : it->second.length;
      const int32_t reach = partial ? kUnbounded : std::max(3, (le + 3) / 4 + le - 2);
      armour_src_.push_back({t, 0, reach, true});
    } else {
      armour_src_.push_back({t, 1, kUnbounded, false});
    }
  }
}

int StrategyAgent::ArmourRisk(int32_t tile) const {
  if (tile < 0) return 0;
  if (armour_field_round_ != round_) {
    armour_field_round_ = round_;
    armour_risk_.assign(board_.size(), 0);
    armour_dist_.assign(board_.size(), -1);
    for (const ArmourSrc& src : armour_src_) {
      std::fill(armour_dist_.begin(), armour_dist_.end(), -1);
      armour_queue_.assign(1, src.tile);
      armour_dist_[src.tile] = src.offset;
      if (src.head)
        for (int d = 0; d < 4; ++d)
          if (const int32_t n = board_.Step(src.tile, d); n >= 0) armour_risk_[n] = 16;
      for (size_t qi = 0; qi < armour_queue_.size(); ++qi) {
        const int32_t u = armour_queue_[qi], du = armour_dist_[u];
        ++work_;
        if (du >= src.reach) continue;
        for (int d = 0; d < 4; ++d) {
          const int32_t n = board_.Step(u, d);
          if (n < 0 || armour_dist_[n] >= 0 || !board_.Current(n)) continue;
          if (board_.at(n).occupant >= 0 && board_.at(n).occupant != spawn_.id) continue;
          armour_dist_[n] = du + 1;
          armour_risk_[n] = std::max(armour_risk_[n], std::max(1, 9 - (du + 1)));
          armour_queue_.push_back(n);
        }
      }
    }
  }
  return armour_risk_[tile];
}

// Growth can't change the result: the enemy queen seen dying, or ours already longer than any enemy
// we have seen plus what a fed queen can grow in the rounds left.
bool StrategyAgent::GrowthMoot() const {
  if (!QARMOUR || round_ < kRaceFrom || !IsQueen()) return false;
  if (enemy_queen_dead_ > 0 && enemy_queen_dead_ >= EnemyQueenKnown()) return true;
  return length_ > enemy_len_max_ + kArmourFeedRate * (499 - round_);
}

}  // namespace gm2
