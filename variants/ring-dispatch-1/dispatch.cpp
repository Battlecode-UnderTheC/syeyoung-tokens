// g43-2 (ring-dispatch-1): the team task board, the second scheduling step after ring-sched-1.
//
// Evidence (arena-r17, server r15): in the opening our workers bunch at home while the opponent
// takes the central fields (middle eats r0-59 11.5 v 16.0; server losses r0-60 middle 1.7 v 7.1,
// far 3.5 v 8.3; devtest1 spread at r50 7.0 v 10.5). ring-sched-1's matching only sees allies in
// the 7x7 view, so far food is double-booked, and every explorer picks the best frontier tile on
// its own, so they pile into the same direction; goals flip between food and exploring on 22% of
// turns.
//
// 1. TASK word (magic 0xBB, 64 bits): team (1, bit 55) | id (11, 44-54) | role (2, 42-43) |
//    target x (6, 36-41) | y (6, 30-35) | head x (6, 24-29) | y (6, 18-23) | round mod 16 (4, 14-17) |
//    home kind (2, 12-13: 0 none, 1 a queen sighting, 2 the ring) | home x (6, 6-11) | y (6, 0-5).
//    The sender's sector is that of its target, from the receiver's home. Home: the ring if
//    known (40% of worker turns before r60 didn't know it in sim), else a ring-based home heard
//    in a TASK word, else our queen as last seen, else any home heard. Roles: HARVEST (food tile), EXPLORE (frontier tile, sector),
//    CONTEST (food raced against an enemy head), HOLD (a contested cluster an explorer holds).
//    The ring word fills all four directions; a worker now sends it on two opposite directions
//    (alternating each round) and its TASK on the other two. The queen, and workers within
//    kTaskQueenNear of her or her ring, keep the ring word on all four. Only before kTaskUntil.
// 2. Matching (strategy.cpp ChooseGoal): an ally whose TASK (HARVEST/CONTEST/HOLD) we heard within
//    kTaskFresh rounds and that is out of view joins with one pair, its announced food, arriving
//    at its reported distance less the rounds since (it has been walking there).
// 3. Sectors: frontier tiles are bucketed into kTaskSectors directions from home. Explorers
//    (workers the matching leaves without food within kSchedHarvest) are matched to sectors
//    greedily by distance, at most one per sector; a sector whose frontier an ally's fresh EXPLORE
//    word names is that ally's. A sector held by an ally scores its frontier x kSectorOther, so
//    explorers fan out instead of piling into one direction. (Our own sector is not boosted:
//    x2 cost 4-12 on fresh anti-centre maps; explorers already carry the hunger/unseen multiplier.)
// 4. Hysteresis: our food goal of last turn scores x1.3 in our own goal choice (a new one must
//    beat it by 30%; sched-1 measured food/explore flips on 22% of turns), last turn's sector
//    competes at distance x 10/13, and a worker that harvested last turn stays a harvester up to
//    kSchedHarvest * 1.3 rounds. Not inside the matching: a discount on a dragon's own last target
//    is known only to allies that heard its TASK word, so matchings diverged (open fresh maps
//    4-12, 16 v 25 dragons at r50).
// TASK_ABL (ablation, default 0): 1 no out-of-view allies, 2 no hysteresis, 3 no sectors,
// 4 no TASK words sent, 6 no role hysteresis.
#include <algorithm>
#include <cmath>
#include <cstdio>

#include "strategy.hpp"

namespace gm2 {
using namespace strat;

namespace {
constexpr uint64_t kTaskMagic = 0xBB;
}

int32_t StrategyAgent::SectorOf(int32_t t) const {
  if (home_ < 0 || t < 0) return -1;
  int32_t dx = board_.X(t) - board_.X(home_), dy = board_.Y(t) - board_.Y(home_);
  const int32_t w = board_.width(), h = board_.height();
  if (dx > w / 2) dx -= w;
  if (dx < -w / 2) dx += w;
  if (dy > h / 2) dy -= h;
  if (dy < -h / 2) dy += h;
  if (std::abs(dx) + std::abs(dy) < 3) return -1;   // the ring's own ground (the queen's pasture)
  const double a = std::atan2(double(dy), double(dx));   // -pi..pi
  int32_t s = int32_t(std::floor((a + M_PI) / (2 * M_PI) * kTaskSectors + 0.5)) % kTaskSectors;
  if (s < 0) s += kTaskSectors;
  return s;
}

void StrategyAgent::HearTasks(const rl::Observation& o) {
  if (!ring_.empty()) {
    home_ = ring_[0];
    home_kind_ = 2;
  } else if (home_kind_ < 2) {
    for (const auto& [id, w] : seen_)
      if (id <= 1 && w.team == spawn_.team && w.round == round_ && w.head >= 0) {
        home_ = w.head;
        home_kind_ = 1;
      }
  }
  for (const uint64_t w : o.inbox) {
    if ((w >> 56) != kTaskMagic || int32_t((w >> 55) & 1) != (spawn_.team & 1)) continue;
    const int32_t id = int32_t((w >> 44) & 2047), r = round_ - ((round_ - int32_t((w >> 14) & 15)) & 15);
    if (id == spawn_.id || round_ - r > kTaskFresh) continue;
    const int32_t tx = int32_t((w >> 36) & 63), ty = int32_t((w >> 30) & 63), hx = int32_t((w >> 24) & 63),
                  hy = int32_t((w >> 18) & 63), kind = int32_t((w >> 12) & 3), mx = int32_t((w >> 6) & 63), my = int32_t(w & 63);
    if (tx >= board_.width() || ty >= board_.height() || hx >= board_.width() || hy >= board_.height()) continue;
    if (((kind == 2 && home_kind_ < 2) || (kind == 1 && home_kind_ == 0)) && mx < board_.width() && my < board_.height()) {
      home_ = board_.Tile(mx, my);
      home_kind_ = kind;
    }
    HeardTask& h = tasks_heard_[id];
    if (r < h.round) continue;
    h.role = int32_t((w >> 42) & 3);
    h.target = board_.Tile(tx, ty);
    h.head = board_.Tile(hx, hy);
    h.round = r;
  }
  for (auto it = tasks_heard_.begin(); it != tasks_heard_.end();)
    if (round_ - it->second.round > kTaskFresh) it = tasks_heard_.erase(it);
    else {
      it->second.sector = it->second.role == kTaskExplore ? SectorOf(it->second.target) : -1;
      ++it;
    }
}

// our task this turn, from the goal Decide walks to
void StrategyAgent::SetTask(int32_t goal, const Paths& paths) {
#ifndef DISPATCH_DBG
  if (!TASK_MODE) return;
#endif
  if (IsQueen() || round_ >= kTaskUntil) return;
  int32_t role = -1, target = -1, sector = -1;
  if (contest_goal_ >= 0) {
    role = kTaskContest;
    target = contest_goal_;
  } else if (rush_state_ == 2 && rush_target_ >= 0) {
    role = kTaskHold;
    target = rush_target_;
  } else if (goal >= 0 && paths.dist[goal] > 0) {
    int32_t ready = 0;
    if (TileValue(goal, paths.dist[goal], &ready) > 0) {
      role = kTaskHarvest;
      target = goal;
    } else {
      role = kTaskExplore;
      target = goal;
      sector = SectorOf(goal);
    }
  }
  if (role < 0) return;
#ifdef DISPATCH_DBG
  if (round_ < 60) {
    const bool kept = task_round_ == round_ - 1 && role == task_role_ &&
                      (role == kTaskExplore ? sector == task_sector_ : target == task_target_);
    const bool food_explore_flip = task_round_ == round_ - 1 && (role == kTaskExplore) != (task_role_ == kTaskExplore);
    std::fprintf(stderr, "TDBG r%d team%d id%d role%d sec%d headsec%d kept%d flip%d prev%d home%d heard%d\n", round_, spawn_.team,
                 spawn_.id, role, sector, SectorOf(head_), int(kept), int(food_explore_flip), int(task_round_ == round_ - 1),
                 home_kind_, int(tasks_heard_.size()));
  }
#endif
  task_role_ = role;
  task_target_ = target;
  task_sector_ = sector;
  task_round_ = round_;
}

uint64_t StrategyAgent::TaskWord() const {
  const uint64_t home = home_ >= 0 ? (uint64_t(home_kind_ & 3) << 12) | (uint64_t(board_.X(home_) & 63) << 6) | uint64_t(board_.Y(home_) & 63) : 0;
  return (kTaskMagic << 56) | (uint64_t(spawn_.team & 1) << 55) | (uint64_t(spawn_.id & 2047) << 44) |
         (uint64_t(task_role_ & 3) << 42) | (uint64_t(board_.X(task_target_) & 63) << 36) |
         (uint64_t(board_.Y(task_target_) & 63) << 30) | (uint64_t(board_.X(head_) & 63) << 24) |
         (uint64_t(board_.Y(head_) & 63) << 18) | (uint64_t(round_ & 15) << 14) | home;
}

// the TASK word on two opposite directions (the ones the ring word leaves this round)
void StrategyAgent::SendTask(bot::Action& a) const {
  if (!TASK_MODE || TASK_ABL == 4 || IsQueen() || round_ >= kTaskUntil || task_round_ != round_ || task_target_ < 0) return;
  if (a.resign) return;
  // near the queen or her ring: the ring word keeps all four (the ring's readers are there)
  for (const auto& [id, w] : seen_)
    if (id <= 1 && w.team == spawn_.team && w.round == round_ && w.head >= 0 && Torus(w.head, head_) <= kTaskQueenNear) return;
  for (const int32_t t : ring_)
    if (Torus(t, head_) <= kTaskQueenNear) return;
  const uint64_t word = TaskWord();
  const int d0 = 1 - (round_ & 1);   // the ring word is on round&1 and round&1 + 2
  for (const int d : {d0, d0 + 2}) {
    const uint64_t m = a.sonar[d] ? (*a.sonar[d] >> 56) : 0;
    if (!a.sonar[d] || m == 0xBA || m == 0xA7) a.sonar[d] = word;
  }
}

}  // namespace gm2
