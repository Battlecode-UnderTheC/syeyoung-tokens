// g42-5 (ring-stubguard-1): escorts remove enemy heads that come near our queen.
// See strategy.hpp (kGuardRadius) for the observed cause and the constants.
#include "strategy.hpp"
#include <algorithm>

namespace gm2 {
namespace {
// steps a dragon of length L can take this turn when the last one is a head-to-head (it dies
// there, so its paid steps cost nothing it keeps): ceil(L/4) free, then one per segment above 2
int GuardReach(int length) {
  if (length < 2) return 0;
  return std::min(kGuardMaxSteps, (length + 3) / 4 + std::max(0, length - 2));
}
}  // namespace

bool StrategyAgent::StubGuard(const rl::Observation& o, bot::Action* out) {
  if (IsQueen() || units_ <= 1) return false;
  // where our queen is: her head in view now, else the ring she broadcasts (she patrols it)
  std::vector<int> queen_tiles;
  for (int q = 0; q < 2; ++q) {
    const auto it = seen_.find(q);
    if (it == seen_.end() || it->second.team != spawn_.team || it->second.round != round_) continue;
    const int t = it->second.head;
    if (t >= 0 && board_.Current(t) && board_.at(t).occupant == q && board_.at(t).occupant_head) queen_tiles.push_back(t);
  }
  if (queen_tiles.empty() && !ring_.empty() && round_ - ring_seen_ <= 12) queen_tiles = ring_;
  if (queen_tiles.empty()) return false;
  auto is_queen_tile = [&](int t) { return std::find(queen_tiles.begin(), queen_tiles.end(), t) != queen_tiles.end(); };

  // threats: enemy heads in view now within kGuardRadius steps of the queen (tiles out of view
  // count as passable: the queen is often just outside our window)
  struct Threat { int head, id, dq; };
  std::vector<Threat> threats;
  for (const auto& [id, e] : seen_) {
    if (e.team == spawn_.team || e.round != round_ || e.head < 0 || !board_.Current(e.head)) continue;
    if (board_.at(e.head).occupant != id || !board_.at(e.head).occupant_head) continue;
    const auto paths = Search(board_, e.head, [&](int t) {
      return is_queen_tile(t) || !board_.Current(t) || board_.at(t).occupant < 0;
    }, kGuardRadius);
    int dq = -1;
    for (int t : queen_tiles)
      if (paths.dist[t] >= 0 && (dq < 0 || paths.dist[t] < dq)) dq = paths.dist[t];
    if (dq >= 0) threats.push_back({e.head, id, dq});
  }
  if (threats.empty()) return false;
  // the enemy queen in reach: TryQueenAttack (plugin.cpp) takes her, keep its move
  {
    bot::Action attack;
    if (RING12_ATTACK_MODE && TryQueenAttack(o, &attack)) return false;
  }
  std::sort(threats.begin(), threats.end(), [](const Threat& a, const Threat& b) {
    return a.dq != b.dq ? a.dq < b.dq : a.id < b.id;
  });
  auto free_now = [&](int t) { return t >= 0 && board_.Current(t) && board_.at(t).occupant < 0; };
  for (const Threat& th : threats) {
    // distances from the threat's head through free tiles in view (an ally's head is one step
    // beyond its nearest reached neighbour)
    const auto paths = Search(board_, th.head, free_now, kGuardMaxSteps);
    auto dist_from = [&](int h) {
      int best = -1;
      for (int d = 0; d < 4; ++d) {
        const int n = board_.Step(h, d);
        if (n == th.head) return 1;
        if (n >= 0 && paths.dist[n] >= 0 && (best < 0 || paths.dist[n] + 1 < best)) best = paths.dist[n] + 1;
      }
      return best;
    };
    const int mine = dist_from(head_);
    if (mine < 0) continue;
    bool someone_now = false, defer = false;
    bool nearest = true;   // no visible ally beats us on (distance, length, id)
    for (const auto& [id, e] : seen_) {
      if (id == spawn_.id || id <= 1 || e.team != spawn_.team || e.round != round_ || e.head < 0) continue;
      if (!board_.Current(e.head) || board_.at(e.head).occupant != id || !board_.at(e.head).occupant_head) continue;
      const int d = dist_from(e.head);
      if (d < 0) continue;
      if (d <= GuardReach(e.length)) {
        someone_now = true;
        // a shorter ally that acts after us this round (ids act in order) takes it more cheaply
        if (id > spawn_.id && e.length <= kGuardShort && e.length < length_) defer = true;
      }
      if (d < mine || (d == mine && (e.length < length_ || (e.length == length_ && id < spawn_.id)))) nearest = false;
    }
    if (mine <= GuardReach(length_) && !defer) {
      // walk down the distance field onto the threat's head
      std::vector<int> steps;
      int cur = head_;
      for (int left = mine; left > 0; --left) {
        int pick = -1;
        for (int d = 0; d < 4 && pick < 0; ++d) {
          const int n = board_.Step(cur, d);
          if (left == 1 ? n == th.head : (free_now(n) && paths.dist[n] == left - 1)) pick = d;
        }
        if (pick < 0) { steps.clear(); break; }
        steps.push_back(pick);
        cur = board_.Step(cur, pick);
      }
      if (!steps.empty() && cur == th.head) {
        *out = bot::Action{};
        out->kind = bot::Action::kMove;
        out->resign = false;
        out->steps = steps;
        rescue_committed_ = true;
        stubguard_ = true;
        reason_ = "stubguard: trade";
        return true;
      }
      continue;
    }
    if (someone_now || !nearest || mine < 2) continue;
    // nobody in view can reach it this turn: the nearest ally closes in by one free step
    const bool ring_fresh = !ring_.empty() && round_ - ring_seen_ <= 12;
    for (int d = 0; d < 4; ++d) {
      const int n = board_.Step(head_, d);
      if (!free_now(n) || paths.dist[n] < 0 || paths.dist[n] + 1 >= mine) continue;
      if (is_queen_tile(n) || (ring_fresh && std::find(ring_.begin(), ring_.end(), n) != ring_.end())) continue;
      if (int(ring4_observed_body_.size()) != length_) return false;
      std::vector<int32_t> body(ring4_observed_body_.begin(), ring4_observed_body_.end());
      body.insert(body.begin(), n);
      if (!board_.at(n).pearl) body.pop_back();
      if (EscapeLevel(body, EscapeCap(length_)) == 0) continue;
      body_.assign(body.begin(), body.end());
      *out = bot::Action::Move(d);
      rescue_committed_ = true;
      stubguard_ = true;
      reason_ = "stubguard: approach";
      return true;
    }
  }
  return false;
}
}  // namespace gm2
