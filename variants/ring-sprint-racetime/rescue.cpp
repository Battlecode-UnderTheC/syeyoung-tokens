#include "strategy.hpp"
#include <algorithm>
#include <unordered_set>

namespace gm2 {
bot::Action StrategyAgent::RescueQueen(const bot::Action& normal, const rl::Observation& observation) {
  // Require a head directly visible now, not an old head retained in a body sighting.
  int queen = -1, qhead = -1;
  for (int q = 0; q < 2; ++q) {
    const auto it = seen_.find(q);
    if (it == seen_.end() || it->second.team != spawn_.team || it->second.round != round_) continue;
    const int t = it->second.head;
    if (t >= 0 && board_.Current(t) && board_.at(t).occupant == q && board_.at(t).occupant_head) {
      queen = q; qhead = t; break;
    }
  }
  if (queen < 0) return normal;
  std::vector<int> exits;
  bool known = true, blocking = false;
  for (int d = 0; d < 4; ++d) {
    const int t = board_.Step(qhead, d);
    if (t == kUnknown || (t >= 0 && !board_.Current(t))) { known = false; continue; }
    if (t < 0 || t == qhead) continue;
    if (std::find(exits.begin(), exits.end(), t) == exits.end()) exits.push_back(t);
    blocking |= board_.at(t).occupant == spawn_.id;
  }
  auto free_exits = [&](const std::vector<int>& body) {
    int count = 0;
    for (int t : exits) {
      const int owner = board_.at(t).occupant;
      if (owner >= 0 && owner != spawn_.id) continue;
      if (std::find(body.begin(), body.end(), t) == body.end()) ++count;
    }
    return count;
  };
  // Use a fully observed body; inherited one-head-per-turn memory may omit sprint steps.
  std::vector<int> original;
  for (const auto& segment : observation.segments)
    if (segment.id == spawn_.id) original.push_back(board_.Tile(segment.x, segment.y));
  const bool complete = int(original.size()) == length_ && !original.empty() && original.front() == head_;
  // Resignation removes the whole body even if our remembered body is partial.
  const int cleared = free_exits({});
  const int desired = QUEEN_CLEARANCE_MODE >= 3 ? std::min(2, cleared) : 1;
  int before = 0;
  for (int t : exits) if (board_.at(t).occupant < 0) ++before;
  const int free_steps = (length_ + 3) / 4;
  struct Route { std::vector<int> body, steps, eaten; };
  auto advance = [&](Route& r, int d) {
    const bool paid = int(r.steps.size()) >= free_steps;
    if (paid && r.body.size() <= 2) return false; // checked BEFORE eating a pearl
    const int t = board_.Step(r.body.front(), d);
    if (t < 0 || !board_.Current(t) || std::find(r.body.begin(), r.body.end(), t) != r.body.end()) return false;
    const int owner = board_.at(t).occupant;
    if (owner >= 0 && owner != spawn_.id) return false;
    r.steps.push_back(d); r.body.insert(r.body.begin(), t);
    if (board_.at(t).pearl && std::find(r.eaten.begin(), r.eaten.end(), t) == r.eaten.end()) r.eaten.push_back(t);
    else r.body.pop_back();
    if (paid) r.body.pop_back();
    return true;
  };
  Route predicted{original, {}, {}};
  bool predicted_valid = complete;
  if (complete && !normal.resign && normal.kind == bot::Action::kMove)
    for (int d : normal.steps) if (!advance(predicted, d)) { predicted_valid = false; break; }
  else predicted_valid = false;
  const bool would_close = known && before > 0 && predicted_valid && free_exits(predicted.body) < desired;
  const bool needs_clearance = QUEEN_CLEARANCE_MODE && known && cleared > 0 &&
                               ((before < desired && blocking) || would_close);

  std::unordered_set<int> targets;
  if (QUEEN_GUARD) {
    for (const auto& [id, e] : seen_) {
      if (e.team == spawn_.team || e.round != round_ || e.head < 0 || !board_.Current(e.head)) continue;
      const auto& cell = board_.at(e.head);
      if (cell.occupant != id || !cell.occupant_head) continue;
      // A partially seen enemy can be longer than it looks. This is a conservative
      // threat estimate, not a promise that it will choose to attack the queen.
      const int reach = std::min(8, std::max(3, (e.length + 3) / 4 + e.length - 2));
      const auto paths = Search(board_, e.head, [&](int t) {
        return t == qhead || (board_.Current(t) &&
               (board_.at(t).occupant < 0 || board_.at(t).occupant == id));
      }, reach);
      if (paths.dist[qhead] >= 0 && paths.dist[qhead] <= reach) targets.insert(e.head);
    }
  }
  if (!needs_clearance && targets.empty()) return normal;
  if (complete) {
    std::vector<Route> beam{{original, {}, {}}};
    Route yield; bool have_yield = false;
    for (int depth = 0; depth < 8 && !beam.empty(); ++depth) {
      std::vector<Route> next;
      for (const auto& route : beam) for (int d = 0; d < 4; ++d) {
        const bool paid = int(route.steps.size()) >= free_steps;
        if (paid && route.body.size() <= 2) continue;
        const int t = board_.Step(route.body.front(), d);
        if (t >= 0 && targets.count(t)) {
          // Both heads die. No later sprint steps are emitted after the collision.
          bot::Action a; a.steps = route.steps; a.steps.push_back(d);
          rescue_committed_ = true;
          return Because("rescue: intercept", a);
        }
        Route candidate = route;
        if (!advance(candidate, d)) continue;
        if (needs_clearance && free_exits(candidate.body) >= desired &&
            EscapeLevel(candidate.body, std::max(12, 2 * int(candidate.body.size()))) > 0 &&
            (!have_yield || candidate.steps.size() < yield.steps.size() ||
             (candidate.steps.size() == yield.steps.size() && free_exits(candidate.body) > free_exits(yield.body)))) {
          yield = candidate; have_yield = true;
        }
        next.push_back(std::move(candidate));
      }
      // Local bounded search: retain routes closer to an intercept or with more exits.
      auto score = [&](const Route& r) {
        int distance = 100;
        for (int target : targets) {
          int dx = std::abs(board_.X(target) - board_.X(r.body.front()));
          int dy = std::abs(board_.Y(target) - board_.Y(r.body.front()));
          distance = std::min(distance, std::min(dx, board_.width()-dx) + std::min(dy, board_.height()-dy));
        }
        return (needs_clearance ? 20 * free_exits(r.body) : 0) - distance;
      };
      std::stable_sort(next.begin(), next.end(), [&](const Route& a, const Route& b) { return score(a) > score(b); });
      if (next.size() > 32) next.resize(32);
      beam = std::move(next);
      if (have_yield && targets.empty()) break;
    }
    if (have_yield) {
      body_.assign(yield.body.begin(), yield.body.end());
      bot::Action a; a.steps = yield.steps;
      rescue_committed_ = true;
      return Because("rescue: yield", a);
    }
  }
  // Sacrifice only if our body currently blocks a needed exit, not merely to
  // avoid a bad prospective move. Mode 3 may open a second exit proactively.
  if (QUEEN_CLEARANCE_MODE >= 2 && known && before < desired && blocking && cleared >= desired && !normal.resign) {
    rescue_committed_ = true;
    return Because("rescue: clear", bot::Action::Resign());
  }
  return normal;
}
}
