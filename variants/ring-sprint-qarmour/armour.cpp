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
  // v3: relayed heads she can't see herself (a tile in her view shows what is there now)
  for (const RelayHead& h : relay_heads_) {
    if (board_.Current(h.tile)) continue;
    const int32_t age = round_ - h.round;
    const int32_t reach = h.partial ? kUnbounded : std::max(3, (h.length + 3) / 4 + h.length - 2) + age;
    armour_src_.push_back({h.tile, 0, reach, true, true});
  }
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
      if (src.relay) {
        // out of view: the path is unknown, so the torus distance (a lower bound on any path)
        for (int32_t t = 0; t < board_.size(); ++t) {
          if (!board_.Current(t)) continue;
          ++work_;
          const int32_t md = Torus(src.tile, t);
          if (md <= src.reach) armour_risk_[t] = std::max(armour_risk_[t], md <= 1 ? 16 : std::max(1, 9 - md));
        }
        continue;
      }
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

void StrategyAgent::ReadThreats(const rl::Observation& o) {
  relay_heads_.erase(std::remove_if(relay_heads_.begin(), relay_heads_.end(),
                                    [&](const RelayHead& h) { return round_ - h.round > kRelayFresh; }),
                     relay_heads_.end());
  if (!QARMOUR || !IsQueen() || round_ < kRaceFrom) return;
  for (const uint64_t w : o.inbox) {
    if ((w >> 56) != kThreatMagic || int32_t((w >> 55) & 1) != (spawn_.team & 1)) continue;
    const int32_t x = int32_t((w >> 37) & 63), y = int32_t((w >> 31) & 63), age = (round_ - int32_t((w >> 10) & 511)) & 511;
    if (x >= board_.width() || y >= board_.height() || age > kRelayFresh) continue;
    relay_heads_.push_back({board_.Tile(x, y), int32_t((w >> 24) & 127), round_ - age, ((w >> 23) & 1) != 0});
  }
}

void StrategyAgent::SendThreat(const rl::Observation& o, bot::Action& a) const {
  if (!QARMOUR || IsQueen() || round_ < kRaceFrom - 1 || round_ >= 499 || a.resign) return;
  const auto q = seen_.find(queen_id_);
  if (q == seen_.end() || q->second.team != spawn_.team || q->second.round != round_ || q->second.head < 0) return;
  const int32_t qh = q->second.head;
  int32_t best = -1, best_d = kRelayRadius + 1, len = 0;
  bool partial = false;
  for (const auto& [id, w] : seen_) {
    if (w.team == spawn_.team || w.round != round_ || w.head < 0) continue;
    int32_t dx = std::abs(board_.X(w.head) - board_.X(qh)), dy = std::abs(board_.Y(w.head) - board_.Y(qh));
    dx = std::min(dx, board_.width() - dx), dy = std::min(dy, board_.height() - dy);
    if (std::max(dx, dy) <= 3) continue;   // in her own view
    if (dx + dy < best_d) { best_d = dx + dy; best = w.head; len = w.length; partial = w.partial; }
  }
  if (best < 0) return;
  const uint64_t word = (kThreatMagic << 56) | (uint64_t(spawn_.team & 1) << 55) | (uint64_t(board_.X(best) & 63) << 37) |
                        (uint64_t(board_.Y(best) & 63) << 31) | (uint64_t(std::min(len, 127)) << 24) |
                        (uint64_t(partial ? 1 : 0) << 23) | (uint64_t(round_ & 511) << 10);
  // where our pings start: the head after this action; the one opposite our facing starts at the tail
  int32_t from = head_, facing = o.facing;
  if (a.kind == bot::Action::kMove)
    for (int d : a.steps) { const int32_t n = board_.Step(from, d); if (n < 0) return; from = n; facing = d; }
  for (int d = 0; d < 4; ++d) {
    if (d == (facing + 2) % 4) continue;
    if (a.sonar[d] && (*a.sonar[d] >> 56) == 0xBA) continue;   // the ring relay keeps its slot
    int32_t t = from;
    for (int k = 0; k < 7; ++k) {
      t = board_.Step(t, d);
      if (t < 0 || !board_.Current(t)) break;
      const int32_t occ = board_.at(t).occupant;
      if (occ < 0) continue;
      if (occ == queen_id_) a.sonar[d] = word;
      break;
    }
  }
}

// Growth can't change the result: the enemy queen seen dying, or ours already longer than any enemy
// we have seen plus what a fed queen can grow in the rounds left.
bool StrategyAgent::GrowthMoot() const {
  if (!QARMOUR || round_ < kRaceFrom || !IsQueen()) return false;
  if (enemy_queen_dead_ > 0 && enemy_queen_dead_ >= EnemyQueenKnown()) return true;
  return length_ > enemy_len_max_ + kArmourFeedRate * (499 - round_);
}

}  // namespace gm2
