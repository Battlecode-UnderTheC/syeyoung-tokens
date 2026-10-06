// g44-9 (ring-sprint-qwary): the ring queen steps off her loop when a visible enemy can reach her.
//
// Observed cause (server-r19 S1, ring-sprint-1): a 2-3-long queen parked on her 2x2/2x3 loop keeps
// patrolling while an enemy head hovers 1-4 tiles away, and when the base does flee (RingQueen's
// danger), its one-step risk ranking walks her along a wall into a corner or a pocket where the
// next step is forced (Australia 1205181 r267-273, Queen of Spades 1207483 r98-101, 1206849 r93-95).
//
// The queen acts first in a round (ids 0/1), so after her move every enemy dragon gets a turn. An
// enemy of length L kills her head-on when her head is within its step budget: ceil(L/4) free
// steps plus one paid step per segment above 2 (docs/game.md, MOVE). Over k of her turns it can
// close in with free steps first, so her k-th tile is safe from it when the path distance from its
// head (kelp and every other body blocking) exceeds B(k) = budget + (k-1)*ceil(L/4).
//
// QueenEvade scores each of her legal single steps by the turns it keeps her safe from every
// visible enemy head over a kHorizon-turn search of her own steps (her body moving with her), then
// by the worst margin at the first tile. She keeps patrolling her loop when its next tile is as
// safe as the best; otherwise she takes the best step. The queen plays safe: a step never enters
// a corridor tile (kelp on 2+ sides) or a smaller room than she has now, so she never flees into
// mazes, corridors or pockets. With no such step this returns nothing and the base decides.
#include <algorithm>
#include <cstdio>

#include "strategy.hpp"

namespace gm2 {
using namespace strat;

namespace {
// Her search depth in turns: one more than the 2-3 rounds the replays show a hunter closing from
// view range (3-4 tiles) to a strike, so a corner is seen before she is in it. 3^4 = 81 sequences.
constexpr int kHorizon = 4;
// A partially seen enemy is at least this dangerous: QueenRisk's and RingQueen's reach for it.
constexpr int kPartialBudget = 8;
// Distances beyond this are safe for every budget the horizon can need for a visible enemy.
constexpr int kEvadeSearch = 24;
// Only queens that move one step a turn (length <= 4, one free step): 97 of 164 server queen
// deaths were at length 2-3; longer queens already sprint through QueenMove's beam.
constexpr int kEvadeMaxLength = 4;
}  // namespace

bot::Action StrategyAgent::QueenEvade() {
  if (QUEVADE_MODE == 0 || queen_growing_ || length_ > kEvadeMaxLength || int(body_.size()) != length_) return bot::Action{};
  const int horizon = std::min(kHorizon, 500 - round_);
  if (horizon <= 0) return bot::Action{};
  struct Threat { std::vector<int32_t> dist; int budget, free; };
  std::vector<Threat> threats;
  bool reach_now = false;
  for (const auto& [id, e] : seen_) {
    if (e.team == spawn_.team || e.round != round_ || e.head < 0 || !board_.Current(e.head) ||
        board_.at(e.head).occupant != id || !board_.at(e.head).occupant_head) continue;
    const int f = (std::max(e.length, 2) + 3) / 4;
    int budget = f + std::max(0, e.length - 2);
    if (e.partial) budget = std::max(budget, kPartialBudget);
    const int speed = e.partial ? std::max(f, 2) : f;
    // her own segments move with her: let the enemy path cross them (conservative)
    auto paths = Search(board_, e.head, [&](int t) { return Mine(t) || !Occupied(t); }, kEvadeSearch);
    const int d = paths.dist[head_];
    if (d >= 0 && d <= budget + (horizon - 1) * speed) reach_now = true;
    threats.push_back(Threat{std::move(paths.dist), budget, speed});
  }
  if (!reach_now) return bot::Action{};
  // margin of tile t at her k-th turn (1-based): the smallest distance over budget (>0 is safe)
  auto margin = [&](int t, int k) {
    int m = 1000;
    for (const auto& th : threats) {
      const int d = th.dist[t];
      if (d < 0) continue;
      m = std::min(m, d - (th.budget + (k - 1) * th.free));
    }
    return m;
  };
  const bool in_corridor = Corridor(head_);
  const std::vector<int32_t> now(body_.begin(), body_.end());
  const int room_now = OpenRoom(head_, now, kForageRoom);
  auto legal = [&](const std::vector<int32_t>& body, int t) {
    if (t < 0 || std::find(body.begin(), body.end(), t) != body.end()) return false;
    if (board_.Current(t) && board_.at(t).occupant >= 0 && board_.at(t).occupant != spawn_.id) return false;
    if (board_.Current(t) && board_.at(t).occupant == spawn_.id && !Mine(t)) return false;
    return !(Corridor(t) && !in_corridor);
  };
  auto advance = [&](std::vector<int32_t> body, int t, std::vector<int>& eaten) {
    body.insert(body.begin(), t);
    const bool pearl = board_.Current(t) && board_.at(t).pearl && std::find(eaten.begin(), eaten.end(), t) == eaten.end();
    if (pearl) eaten.push_back(t); else body.pop_back();
    return body;
  };
  // turns survived after the first step (already safe at k = 1) along her best continuation
  auto survive = [&](auto&& self, const std::vector<int32_t>& body, std::vector<int> eaten, int k) -> int {
    if (k > horizon) return 0;
    int best = 0;
    for (int d = 0; d < 4 && best < horizon - k + 1; ++d) {
      const int t = board_.Step(body.front(), d);
      if (!legal(body, t) || margin(t, k) <= 0) continue;
      auto e2 = eaten;
      best = std::max(best, 1 + self(self, advance(body, t, e2), e2, k + 1));
    }
    return best;
  };
  int ring_next = -1;
  if (!ring_.empty()) {
    auto at = std::find(ring_.begin(), ring_.end(), head_);
    if (at != ring_.end()) ring_next = ring_[(at - ring_.begin() + 1) % ring_.size()];
  }
  struct Option { int dir = -1, tile = -1, turns = -1, margin = -1000, room = 0; bool pearl = false; };
  Option best, patrol;
  auto better = [&](const Option& a, const Option& b) {
    if (a.turns != b.turns) return a.turns > b.turns;
    if (a.turns < horizon && a.margin != b.margin) return a.margin > b.margin;
    if (a.room != b.room) return a.room > b.room;
    if (a.pearl != b.pearl) return a.pearl;
    return a.margin > b.margin;
  };
  for (int d = 0; d < 4; ++d) {
    const int t = board_.Step(head_, d);
    if (!legal(now, t)) continue;
    std::vector<int> eaten;
    const auto body = advance(now, t, eaten);
    if (EscapeLevel(body, EscapeCap(int(body.size()))) == 0) continue;
    Option o;
    o.dir = d; o.tile = t; o.margin = margin(t, 1); o.pearl = !eaten.empty();
    o.room = OpenRoom(t, body, kForageRoom);
    if (o.room < std::min(kForageRoom, room_now)) continue;   // never into a smaller pocket
    o.turns = o.margin > 0 ? 1 + survive(survive, body, eaten, 2) : 0;
    if (t == ring_next) patrol = o;
    if (best.dir < 0 || better(o, best)) best = o;
  }
  if (best.dir < 0) return bot::Action{};
#ifdef QUEVADE_DBG
  {
    int dmin = 999;
    for (const auto& th : threats) if (th.dist[head_] >= 0) dmin = std::min(dmin, th.dist[head_] - th.budget);
    std::fprintf(stderr, "EVADE r%d team%d len%d head%d,%d threats%d nearest-margin%d best(d%d T%d m%d room%d) patrol(d%d T%d m%d)\n", round_, int(spawn_.team), length_,
                 board_.X(head_), board_.Y(head_), int(threats.size()), dmin, best.dir, best.turns, best.margin, best.room, patrol.dir, patrol.turns, patrol.margin);
  }
#endif
  ++evade_turns_;
  // the loop's next tile is as safe as anything else: keep the ring (its patrol and splits)
  if (patrol.dir >= 0 && patrol.turns >= best.turns && (patrol.turns == horizon || patrol.margin >= best.margin))
    return bot::Action{};
  body_.push_front(best.tile);
  if (!best.pearl) body_.pop_back();
  if (std::find(ring_.begin(), ring_.end(), best.tile) == ring_.end()) ring_.clear();
  ++evade_moves_;
  return Because(best.turns >= horizon ? "evade: off the loop (safe)" : best.turns > 0 ? "evade: off the loop (gains turns)" : "evade: off the loop (widest margin)",
                 bot::Action::Move(best.dir));
}

}  // namespace gm2
