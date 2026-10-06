// g44-8 (ring-sprint-sighting): relayed long-enemy sightings for the race queen.
//
// Observed cause (arena-r18 T3, cluster D1): in the live race (from kRaceFrom) our queen is lunged
// by long enemies that were outside her own 7x7 view: the killer was out of her view in 109 of 195
// such deaths. Her QueenRisk counts only heads she sees this round, but our workers often see the
// killer a round or two earlier.
//
// 1. A worker (from kRaceFrom - kSightHold, so a sighting made just before the race is still live
//    at its start) that sees an enemy head of length kSightMinLen+ near our queen's last known
//    head sends a sighting word (magic 0xAB) on the one or two sonar directions that point at
//    her. A worker that heard a fresh sighting (at most 1 round old) and saw no long enemy itself
//    relays it the same way, so it can reach her over a hop when she is not on its row or column.
// 2. The queen keeps each heard sighting for kSightHold rounds (unless she sees that dragon
//    herself this round, in which case QueenRisk already counts it exactly) and, while it is
//    within kSightNear of her head, adds its danger zone to QueenRisk, which the race queen's
//    beams (QueenMove's harvest beam and Ring4Exit) use.
//
// Values:
// - kSightMinLen 6: the D1 killers are long dragons sprinting from outside the queen's view; a
//   dragon of 6+ has 2 free steps a turn and a lunge reach of (L+3)/4 + L - 2 >= 6, beyond her
//   3-tile view edge. Shorter dragons reach at most 5 and she sees them in time (her own risk map).
// - kSightNear 8: QueenRisk's reach cap (min(8, ...)): a head farther than 8 can't hit her next turn.
// - kSightHold 3: the strategist's proposal; a sighting older than 3 rounds is too vague to steer by
//   (the enemy's possible position grows by its free steps every round).
// - the zone of an aged sighting: the enemy's reach plus its free steps per round of age, capped at
//   kSightReachCap (10) so a stale sighting can't cover the whole neighbourhood.
#include <cstdio>
#include "strategy.hpp"

#include <algorithm>
#include <deque>

namespace gm2 {
namespace {
#ifdef QSIGHT_DBG
int g_ray_dragon = 0;
#endif
constexpr uint64_t kSightMagic = 0xAB;
constexpr int32_t kSightMinLen = 6, kSightNear = 8, kSightHold = 3, kSightReachCap = 10;
// a worker's view of the queen: her report may be stale; she moves about 2 tiles a turn at most
// when she isn't sprinting, so the queen-distance test at the sender gets that much slack per round
constexpr int32_t kQueenAgeMax = 5, kQueenDrift = 2;
}  // namespace

uint64_t StrategyAgent::SightingWord(const RelayedSighting& s) const {
  return (kSightMagic << 56) | (uint64_t(spawn_.team & 1) << 55) | (uint64_t(s.id & 4095) << 43) |
         (uint64_t(board_.X(s.tile) & 63) << 37) | (uint64_t(board_.Y(s.tile) & 63) << 31) |
         (uint64_t(std::min(s.length, 63)) << 25) | (uint64_t(s.round & 511) << 10);
}

void StrategyAgent::HearSightings(const rl::Observation& o) {
  facing_ = int(o.facing);
  for (auto it = sightings_.begin(); it != sightings_.end();)
    it = round_ - it->second.round >= kSightHold ? sightings_.erase(it) : std::next(it);
  for (const uint64_t w : o.inbox) {
    if ((w >> 56) != kSightMagic || int32_t((w >> 55) & 1) != (spawn_.team & 1)) continue;
    RelayedSighting s;
    s.id = int32_t((w >> 43) & 4095);
    const int32_t x = int32_t((w >> 37) & 63), y = int32_t((w >> 31) & 63);
    s.length = int32_t((w >> 25) & 63);
    s.round = int32_t((w >> 10) & 511);
    if (x >= board_.width() || y >= board_.height() || s.round > round_ || round_ - s.round >= kSightHold) continue;
    s.tile = board_.Tile(x, y);
    RelayedSighting& have = sightings_[s.id];
    if (s.round > have.round) have = s;
#ifdef QSIGHT_DBG
    if (IsQueen()) std::fprintf(stderr, "QSIGHT hear r%d team%d enemy%d len%d seen_r%d dist%d\n", round_, spawn_.team, s.id, s.length, s.round, Torus(s.tile, head_));
#endif
  }
}

bool StrategyAgent::QueenWhere(int32_t* tile, int32_t* age) const {
  if (queen_id_ < 0) return false;
  if (const auto q = seen_.find(queen_id_); q != seen_.end() && q->second.team == spawn_.team && q->second.round == round_ && q->second.head >= 0) {
    *tile = q->second.head;
    *age = 0;
    return true;
  }
  if (const auto r = reports_.find(queen_id_); r != reports_.end() && r->second.tile >= 0 && r->second.length > 0 && round_ - r->second.round <= kQueenAgeMax) {
    *tile = r->second.tile;
    *age = round_ - r->second.round;
    return true;
  }
  return false;
}

void StrategyAgent::SendSighting(bot::Action& a) const {
  if (IsQueen() || round_ < kRaceFrom - kSightHold || round_ >= 499 || a.resign) return;
  int32_t queen = -1, age = 0;
  if (!QueenWhere(&queen, &age)) return;
  const int32_t near = kSightNear + kQueenDrift * age;
  // our own sighting this round: the long enemy head nearest our queen
  RelayedSighting best;
  int32_t best_dist = 1 << 30;
  for (const auto& [id, e] : seen_) {
    if (e.team == spawn_.team || e.round != round_ || e.head < 0 || e.length < kSightMinLen || !board_.Current(e.head) ||
        board_.at(e.head).occupant != id || !board_.at(e.head).occupant_head)
      continue;
    const int32_t d = Torus(e.head, queen);
    if (d <= near && d < best_dist) { best_dist = d; best = {id, e.head, e.length, round_}; }
  }
  // else relay the freshest one we heard (at most a round old), so it can reach her over a hop
  if (best.id < 0)
    for (const auto& [id, s] : sightings_)
      if (round_ - s.round <= 1 && s.length >= kSightMinLen && Torus(s.tile, queen) <= near &&
          (best.id < 0 || s.round > best.round || (s.round == best.round && Torus(s.tile, queen) < best_dist))) {
        best = s;
        best_dist = Torus(s.tile, queen);
      }
  if (best.id < 0) return;
  // Aim: a ping goes straight until kelp or the first dragon (any segment), so the word reaches
  // her only along a clear line to her body. After the action our head is at `from`, facing
  // `facing`; a ping opposite the facing starts at our tail, so that direction is not aimed.
  int32_t from = head_;
  int facing = facing_;
  if (a.kind == bot::Action::kMove)
    for (const int d : a.steps) {
      const int32_t n = board_.Step(from, d);
      if (n < 0) break;
      from = n;
      facing = d;
    }
  // per direction: 2 = the ray meets one of her segments in view, 1 = it passes her last known
  // head with nothing seen in the way, 0 = neither
  int aim[4] = {};
#ifdef QSIGHT_DBG
  char ray[4] = {'-', '-', '-', '-'};
#endif
  for (int d = 0; d < 4; ++d) {
    if (d == (facing + 2) % 4) continue;
    int32_t t = from;
#ifdef QSIGHT_DBG
    ray[d] = 'E';
#endif
    for (int k = 0; k < board_.width() + board_.height(); ++k) {
      t = board_.Step(t, d);
#ifdef QSIGHT_DBG
      if (t == kBlocked) ray[d] = 'K';
      if (t == kUnknown) ray[d] = 'U';
      if (t >= 0 && board_.Current(t) && board_.at(t).occupant >= 0) ray[d] = board_.at(t).occupant == queen_id_ ? 'Q' : board_.at(t).occupant == spawn_.id ? 'S' : board_.at(t).occupant_team == spawn_.team ? 'A' : 'X';
#endif
      if (t < 0) break;
      const auto& cell = board_.at(t);
      if (board_.Current(t) && cell.occupant >= 0) {
        if (cell.occupant == queen_id_) aim[d] = 2;
#ifdef QSIGHT_DBG
        else ++g_ray_dragon;
#endif
        break;
      }
      // her head as last reported, or a tile her body was seen on in the last kSightHold rounds
      // (out of view now; her body only moves up behind her head, so it is likely still hers)
      if (t == queen || (cell.occupant == queen_id_ && round_ - cell.seen_round <= kSightHold)) { aim[d] = 1; break; }
    }
  }
  const uint64_t w = SightingWord(best);
  int sent = 0;
  for (int level = 2; level >= 1 && !sent; --level)
    for (int d = 0; d < 4; ++d)
      if (aim[d] == level) { a.sonar[d] = w; ++sent; }
  if (!sent) {
    // no clear line: ping every direction we can aim (not the tail's), so the allies around us
    // hear it and one nearer her, or on a line to her body, relays it next turn
    for (int d = 0; d < 4; ++d)
      if (d != (facing + 2) % 4) a.sonar[d] = w;
  }
#ifdef QSIGHT_DBG
  std::fprintf(stderr, "QSIGHT send r%d team%d me%d enemy%d len%d seen_r%d qdist%d qage%d aim%d ray%c%c%c%c\n", round_, spawn_.team, spawn_.id, best.id, best.length, best.round, best_dist, age, sent ? std::max(std::max(aim[0], aim[1]), std::max(aim[2], aim[3])) : 0, ray[0], ray[1], ray[2], ray[3]);
#endif
}

// the race queen's extra risk from relayed sightings (0 elsewhere); same scale as QueenRisk's
// distance term (9 - distance), never the 16 of a seen adjacent head
int StrategyAgent::RelayedRisk(int32_t tile) const {
  if (!IsQueen() || round_ < kRaceFrom) return 0;
  if (relayed_round_ != round_) {
    relayed_round_ = round_;
    relayed_risk_.assign(board_.size(), 0);
    relayed_active_ = 0;
    for (const auto& [id, s] : sightings_) {
      const int32_t age = round_ - s.round;
#ifdef QSIGHT_DBG
      std::fprintf(stderr, "QSIGHT consider r%d team%d enemy%d age%d len%d dist%d seen_now%d\n", round_, spawn_.team, id, age, s.length, Torus(s.tile, head_), int(seen_.count(id) && seen_.at(id).round == round_));
#endif
      if (age >= kSightHold || s.length < kSightMinLen || Torus(s.tile, head_) > kSightNear) continue;
      // seen by us this round: QueenRisk already counts it from where it really is
      if (const auto e = seen_.find(id); e != seen_.end() && e->second.round == round_) continue;
      const int32_t reach = std::min(8, (s.length + 3) / 4 + s.length - 2);
      const int32_t grow = age * ((s.length + 3) / 4);
      const int32_t cap = std::min(kSightReachCap, reach + grow);
      ++relayed_active_;
      // flood from where it was seen through tiles not known to be occupied now
      std::deque<int32_t> queue{s.tile};
      std::vector<int8_t> dist(board_.size(), -1);
      dist[s.tile] = 0;
      while (!queue.empty()) {
        const int32_t t = queue.front();
        queue.pop_front();
        ++work_;
        if (dist[t] >= cap) continue;
        for (int d = 0; d < 4; ++d) {
          int32_t n = board_.Step(t, d);
          // an edge we never saw (the enemy was seen out of our view, often on ground we never
          // walked): assume it open, as the worst case for us
          if (n == kUnknown) n = board_.Tile(board_.X(t) + dragons::kDx[d], board_.Y(t) + dragons::kDy[d]);
          if (n < 0 || dist[n] >= 0) continue;
          if (board_.Current(n) && board_.at(n).occupant >= 0 && board_.at(n).occupant != spawn_.id) continue;
          dist[n] = int8_t(dist[t] + 1);
          queue.push_back(n);
        }
      }
      for (int32_t t = 0; t < board_.size(); ++t) {
        if (dist[t] < 0) continue;
        const int32_t eff = std::max(1, int32_t(dist[t]) - grow);
        if (eff <= reach) relayed_risk_[t] = std::max<int8_t>(relayed_risk_[t], int8_t(9 - eff));
      }
    }
#ifdef QSIGHT_DBG
    if (relayed_active_) {
      int tiles = 0, near = 0;
      for (int32_t t = 0; t < board_.size(); ++t) if (relayed_risk_[t] > 0) { ++tiles; if (Torus(t, head_) <= 2) ++near; }
      std::fprintf(stderr, "QSIGHT queen r%d team%d active%d len%d risk_tiles%d near_head%d\n", round_, spawn_.team, relayed_active_, length_, tiles, near);
    }
#endif
  }
  return relayed_risk_[tile];
}

}  // namespace gm2
