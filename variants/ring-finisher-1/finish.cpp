// g43-8 (ring-finisher-1): finish a nearly dead enemy (see kFinishFrom in strategy.hpp).
#include "strategy.hpp"

#include <algorithm>
#include <cstdio>
#include <vector>

namespace gm2 {

// enemy ids known alive within kFinishWindow rounds, less those seen to die since
int32_t StrategyAgent::EnemyAlive() const {
  int32_t n = 0;
  for (const auto& [id, r] : enemy_alive_) {
    if (round_ - r > kFinishWindow) continue;
    if (const auto d = enemy_dead_.find(id); d != enemy_dead_.end() && d->second >= r) continue;
    ++n;
  }
  return n;
}

// A stub (length kFinishMinLen..kFinishMaxLen, not the queen, not the champion) hunts the enemy's
// last dragons: from kFinishFrom, while we have kFinishMinUnits+ dragons and this dragon knows of
// kFinishMaxEnemies or fewer live enemies, it trades head-on into an enemy head in view within
// kFinishRadius steps (the longest such head first, then the nearest): a plain step or a lunge
// when one reaches it this turn, else a step towards it over tiles in view. With no head in view,
// it steps towards the freshest head seen within kFinishFresh rounds and kFinishRadius steps.
bot::Action StrategyAgent::FinishEnemy() {
  if (IsQueen() || round_ < kFinishFrom || round_ >= 500) return {};
  if (length_ < kFinishMinLen || length_ > kFinishMaxLen || units_ < kFinishMinUnits) return {};
  if (late_champ_ == spawn_.id || Champion().id == spawn_.id) return {};
  const int32_t alive = EnemyAlive();
#ifdef FIN_DBG
  if (round_ % 5 == 0) std::fprintf(stderr, "FIN r%d team%d id%d len%d units%d alive%d\n", round_, spawn_.team, spawn_.id, length_, units_, alive);
#endif
  if (alive < 1 || alive > kFinishMaxEnemies) return {};
#ifdef FIN_DBG
  for (const auto& [id, w] : seen_)
    if (w.team != spawn_.team && w.head >= 0 && round_ - w.round <= 8)
      std::fprintf(stderr, "FINS r%d id%d sees enemy %d age%d len%d at(%d,%d) me(%d,%d)\n", round_, spawn_.id, id, round_ - w.round, w.length, board_.X(w.head), board_.Y(w.head), board_.X(head_), board_.Y(head_));
#endif
  // a breadth-first pass over free tiles in view; enemy heads in view are terminal
  struct From { int32_t prev; int8_t dir; int16_t dist; };
  std::unordered_map<int32_t, From> from;
  std::vector<int32_t> queue{head_};
  from[head_] = {-1, -1, 0};
  int32_t target = -1, target_len = -1, target_dist = 0;
  for (size_t q = 0; q < queue.size(); ++q) {
    const int32_t t = queue[q];
    const int32_t dist = from.at(t).dist;
    if (dist >= kFinishRadius) continue;
    for (int d = 0; d < 4; ++d) {
      const int32_t n = board_.Step(t, d);
      if (n < 0 || from.count(n) || !board_.Current(n)) continue;
      const Cell c = Classify(n);
      if (c == Cell::kEnemyHead) {
        from[n] = {t, static_cast<int8_t>(d), static_cast<int16_t>(dist + 1)};
        const auto it = seen_.find(board_.at(n).occupant);
        const int32_t len = it != seen_.end() ? it->second.length : 0;
        if (len > target_len) target = n, target_len = len, target_dist = dist + 1;   // BFS order: nearest first on ties
        continue;
      }
      if (c != Cell::kFree) continue;
      from[n] = {t, static_cast<int8_t>(d), static_cast<int16_t>(dist + 1)};
      queue.push_back(n);
    }
  }
  if (target < 0) {
    // none in view: the freshest enemy head seen within kFinishFresh rounds and kFinishRadius steps
    // (over tiles we remember free), one step towards where it was; it is in view again within a
    // few steps, or the sighting goes stale and the hunt stops
    const Paths paths = Search(board_, head_, [&](int32_t t) { return !Occupied(t) && Classify(t) == Cell::kFree; }, kFinishRadius);
    int32_t best = -1, best_age = 0, best_dist = 0;
    for (const auto& [id, w] : seen_) {
      if (w.team == spawn_.team || w.head < 0 || w.round >= round_ || round_ - w.round > kFinishFresh) continue;
      if (const auto d = enemy_dead_.find(id); d != enemy_dead_.end() && d->second >= w.round) continue;
      // the nearest free tile beside its remembered head
      int32_t near = -1;
      for (int d = 0; d < 4; ++d) {
        const int32_t n = board_.Step(w.head, d);
        if (n >= 0 && paths.dist[n] > 0 && (near < 0 || paths.dist[n] < paths.dist[near])) near = n;
      }
      if (near < 0) continue;
      const int32_t age = round_ - w.round;
      if (best < 0 || age < best_age || (age == best_age && paths.dist[near] < best_dist)) best = near, best_age = age, best_dist = paths.dist[near];
    }
    if (best < 0) return {};
    const int dir = paths.first[best];
    const int32_t to = board_.Step(head_, dir);
    std::vector<int32_t> body(body_.begin(), body_.end());
    body.insert(body.begin(), to);
    if (!board_.at(to).pearl) body.pop_back();
    if (EscapeLevel(body, EscapeCap(length_)) == 0) return {};
    return Because("finish: seek", bot::Action::Move(dir));
  }
  std::vector<int> steps;
  for (int32_t u = target; from.at(u).prev >= 0; u = from.at(u).prev) steps.push_back(from.at(u).dir);
  std::reverse(steps.begin(), steps.end());
  // the steps a stub can take this turn: one free, then one paid step while length > 2
  if (target_dist <= length_ - 1) return Because("finish: trade", bot::Action{bot::Action::kMove, steps, 0, false, {}});
  // too far for one turn: one step towards it, if the step leaves us room to move on
  const int32_t to = board_.Step(head_, steps[0]);
  std::vector<int32_t> body(body_.begin(), body_.end());
  body.insert(body.begin(), to);
  if (!board_.at(to).pearl) body.pop_back();
  if (EscapeLevel(body, EscapeCap(length_)) == 0) return {};
  return Because("finish: approach", bot::Action::Move(steps[0]));
}

}  // namespace gm2
