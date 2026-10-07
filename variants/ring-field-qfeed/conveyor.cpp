// g44-7 (ring-sprint-conveyor): the feeding conveyor (strategist g44 #3).
//
// Observed (arena, ring-sprint-2's late queen races; local reproduction on Around UNSW):
//  - nobody walks to the growing queen: QueenRace() only sends a worker that saw the enemy queen
//    itself within kRaceFresh rounds, which a parked enemy queen almost never allows (0 walks in
//    the reproduced game, our queen 3 -> 12 with 34 dragons and 210 length beside her at r450);
//    and the one walk step it does take is replaced by the late lookahead step (plugin.cpp keeps
//    gst's step only for "feed"/"champion" reasons or WalkingToAlly());
//  - from r450 every worker that happens to have a body segment within 3 steps of her head
//    resigns at once (feed14): several corpses can land around her in one round, interleaved
//    with live bodies, while workers further out never come.
//
// The conveyor, while our queen is growing in a queen race (the gate her own ring exit uses:
// EnemyQueenRace() from kRaceFrom, and she is not parked in her ring):
//  - a worker that sees no ally closer to her than itself (it is first in line in its own view)
//    walks to her (marked as walking to an ally, so the step is kept);
//  - a worker lays itself down only when her line runs short: fewer pearls within kLineReach free
//    steps of her head than one turn of her sprint (iteration 2; iteration 1 waited for an empty
//    zone and fed her less than the base's wave in duels). It then resigns with its head within
//    kLineReach free steps of hers and its body trailing away behind it along the free path it
//    came by, so its corpse is a line of pearls (every second segment, head first) running out
//    from beside her head that she follows with her free sprint steps. (It lays within kLineReach,
//    not kRaceReach: she keeps moving, so a feeder chasing a tile 2 from her head rarely got
//    there; within 3 the head pearl is still one or two of her turns away, and inside the
//    reserved ground (kReserve = 4) that keeps other workers off her pearls.);
//  - a worker that is not first, or finds her line busy, steps away from her while it is within
//    kQueueDist of her head (it waits out of her way), and forages as usual further out (the
//    reserved ground already keeps it off her pearls).
// One feeder lays one line at a time; the next one is already close.
#include "strategy.hpp"

#include <algorithm>
#include <string>
#ifdef CONVEYOR_DBG
#include <cstdio>
#endif

namespace gm2 {

// g48-3: an enemy head in view (this round) with a free path to qhead within its strike reach
// (ceil(L/4) + L - 2, at least 3, at most 8; a partly seen one 8, as RingQueen) + extra
bool StrategyAgent::QThreat(int32_t qhead, int extra) const {
  if (qhead < 0) return false;
  for (const auto& [id, e] : seen_) {
    if (e.team == spawn_.team || e.round != round_ || e.head < 0 || !board_.Current(e.head) ||
        board_.at(e.head).occupant != id || !board_.at(e.head).occupant_head) continue;
    const int reach = (QUEEN_ENDGAME && e.partial ? 8 : std::min(8, std::max(3, (e.length + 3) / 4 + e.length - 2))) + extra;
    if (Torus(e.head, qhead) > reach) continue;
    const Paths p = Search(board_, e.head, [&](int32_t t) { return t == qhead || !Occupied(t); }, reach);
    if (p.dist[qhead] >= 0 && p.dist[qhead] <= reach) return true;
  }
  return false;
}

// g48-3: the tiles of her pen she reaches from her head (her own body counts: it moves with her;
// other bodies, kelp and unseen tiles don't), up to cap
int StrategyAgent::PenSpace(int cap) const {
  if (!qpen_on_ || head_ < 0) return 0;
  qs_dist_.Begin(board_.size());
  qs_queue_.assign(1, head_);
  qs_dist_.Set(head_);
  int reached = 1;
  for (size_t qi = 0; qi < qs_queue_.size() && reached < cap; ++qi) {
    const int32_t u = qs_queue_[qi];
    for (int d = 0; d < 4; ++d) {
      const int32_t n = board_.Step(u, d);
      if (n < 0 || qs_dist_.Has(n) || !qpen_[n] || !board_.Current(n)) continue;
      if (board_.at(n).occupant >= 0 && board_.at(n).occupant != spawn_.id) continue;
      qs_dist_.Set(n);
      ++reached;
      qs_queue_.push_back(n);
    }
  }
  return std::min(reached, cap);
}

// g48-3: the queen's state each turn: her pen (refreshed while she patrols a ring), her threat
// clock (an enemy head able to reach her within its reach + 2), and whether she may grow
void StrategyAgent::QFeedUpdate() {
  qgrow_ = false;
  if (!QFEED_MODE || round_ < kQFeedFrom - kQFeedQuiet || round_ >= kRaceFrom || queen_growing_) return;
  if (QThreat(head_, 2)) qthreat_round_ = round_;
  if (!ring_.empty() && std::find(ring_.begin(), ring_.end(), head_) != ring_.end()) {
    qpen_.assign(board_.size(), 0);
    for (int32_t t = 0; t < board_.size(); ++t)
      for (const int32_t r : ring_)
        if (Torus(t, r) <= kQFeedPen) { qpen_[t] = 1; break; }
    qpen_on_ = true;
    qanchor_ = ring_;
  }
  if (!QFeedWindow() || !qpen_on_) return;
  qgrow_ = round_ - qthreat_round_ > kQFeedQuiet && !EnemyQueenDeadKnown() && PenSpace(length_ + 1) >= length_ + 1;
}

int StrategyAgent::QPenOut(int32_t t) const {
  if (!qpen_on_ || t < 0 || qpen_[t]) return 0;
  int best = 1 << 20;
  for (const int32_t a : qanchor_) best = std::min(best, int(Torus(t, a)));
  return std::max(0, best - kQFeedPen);
}

bot::Action StrategyAgent::Conveyor() {
  // g48-3: from kQFeedFrom, idle stubs (length <= kQFeedStub, no food within kQFeedWalk rounds last
  // turn, not just born) feed her in place while she is threat-free and the enemy queen isn't
  // known dead; from kRaceFrom the conveyor runs as before
  const bool early = QFEED_MODE && round_ >= kQFeedFrom && round_ < kRaceFrom;
  if (early) {
    if (length_ < kFeedMin || length_ > kQFeedStub || EnemyQueenDeadKnown() || round_ - born_ < 2 ||
        round_ - food_goal_round_ <= 1) return {};
  } else if (round_ < kRaceFrom || round_ >= 499 || length_ < kFeedMin || !EnemyQueenRace()) return {};
  if (int(body_.size()) != length_) return {};   // our own body must be fully known to lay it
  int32_t target = -1;
  bool in_view = false;
  if (const auto it = seen_.find(queen_id_); it != seen_.end() && it->second.team == spawn_.team && it->second.round == round_ &&
      it->second.head >= 0 && board_.Current(it->second.head) && board_.at(it->second.head).occupant == queen_id_ &&
      board_.at(it->second.head).occupant_head) {
    target = it->second.head;
    in_view = true;
  } else if (const auto r = reports_.find(queen_id_); r != reports_.end() && r->second.length > 0 && r->second.tile >= 0 &&
             round_ - r->second.round <= kRaceFresh / 3) {
    target = r->second.tile;   // relayed every round; late she moves slowly, so a recent report is close
    // the reported tile is in view and she isn't on it: the report is stale, and we don't know
    // where she went (traced: feeders 1-3 tiles from a stale report, walking to an empty tile)
    if (board_.Current(target)) return {};
  }
  if (target < 0) return {};
  // she is still parked in her ring (no exit yet): she isn't growing, so don't feed her (as Ring4Supply)
  if (!early && round_ - ring_seen_ <= 12 && std::find(ring_.begin(), ring_.end(), target) != ring_.end()) return {};
  // g48-3: she is threatened as far as we can see: no feeding (her growth pauses too)
  if (early && QThreat(target, 2)) return {};
  // ... or she says her growth is paused (her pen can't hold more, a threat, the enemy queen dead)
  if (early)
    if (const auto r = reports_.find(queen_id_); r != reports_.end() && r->second.paused && round_ - r->second.round <= kQFeedQuiet) return {};
  // first in line: no ally head in view is closer to her (ties: the lower id goes first)
  const int32_t mine = Torus(head_, target);
  bool first = true;
  for (const auto& [id, w] : seen_)
    if (w.team == spawn_.team && w.round == round_ && w.head >= 0 && id > 1 && id != spawn_.id) {
      const int32_t d = Torus(w.head, target);
      if (d < mine || (d == mine && id < spawn_.id)) first = false;
    }
  // "nearby" (as QueenRace): we arrive with at least as many rounds left for her to eat as we walked
  const int32_t budget = early ? kQFeedWalk : (500 - round_) / 2;
  if (budget < 1 || mine > budget + kRaceReach) return {};
  // her zone: free tiles within kLineReach steps of her head
  const Paths zone = Search(board_, target, [&](int32_t t) { return t == head_ || !Occupied(t); }, kLineReach);
  // the line is busy while her zone holds at least one turn of her sprint in pearls (ceil(L/4)
  // free steps: the corpse lines' pearls lie 2 apart and she also eats as she turns), so the
  // next line goes down as she reaches the end of the last one and never piles up around her.
  // Dragons act one after another, so a feeder that lays this round is counted by the next.
  int32_t qlen = 0;
  if (const auto it = seen_.find(queen_id_); it != seen_.end()) qlen = it->second.length;
  if (const auto r = reports_.find(queen_id_); r != reports_.end()) qlen = std::max(qlen, r->second.length);
  const int32_t need = std::max(1, (qlen + 3) / 4);
  int32_t pending = 0;
  if (in_view)
    for (int32_t t = 0; t < board_.size(); ++t) pending += zone.dist[t] > 0 && board_.Current(t) && board_.at(t).pearl;
  const bool busy = in_view && pending >= need;
#ifdef CONVEYOR_DBG
  std::fprintf(stderr, "CVD r%d team%d id%d len%d mine%d first%d busy%d view%d zd%d\n", round_, spawn_.team, spawn_.id, length_, mine, int(first), int(busy), int(in_view), zone.dist[head_]);
#endif
  // lay: in view of her, within her zone, and her line is not busy (first in line or not: whoever
  // is already beside her when the line runs short)
  if (in_view && !busy && zone.dist[head_] > 0 && zone.dist[head_] <= kLineReach) {
    rescue_committed_ = true;
    return Because("conveyor: lay feed line", bot::Action::Resign());
  }
  if (!first || busy) {
    // wait in line, out of her way: within kQueueDist of her head, step away from her (a waiting
    // body beside her is a wall: in the first trace a feeder's head and her own coil boxed her in);
    // further out, forage as usual
    if (mine > kQueueDist) return {};
    int32_t away = -1, best = -1;
    for (int d = 0; d < 4; ++d) {
      const int32_t to = board_.Step(head_, d);
      if (Classify(to) != Cell::kFree || (RING5_MODE == 3 && Ring5Reserved(to))) continue;
      const int32_t dt = Torus(to, target);
      if (dt > best) { best = dt; away = d; }
    }
    if (away < 0 || best <= mine) return {};
    bot::Action a = SafeStep(away, false);
    if (std::string(reason_) != "toward goal") return {};
    reason_ = "conveyor: wait in line";
    return a;
  }
  const Paths paths = Search(board_, head_, [&](int32_t t) {
    return (RING5_MODE != 3 || !Ring5Reserved(t)) && !Occupied(t);
  }, budget + kRaceReach);
  const int32_t goal = paths.Nearest([&](int32_t t) { return t != target && zone.dist[t] > 0; });
  if (goal < 0 || paths.dist[goal] <= 0) return {};
  cur_goal_ = goal;
  bot::Action a = SafeStep(paths.first[goal], false);
  cur_goal_ = -1;
  if (std::string(reason_) != "toward goal") return {};   // no safe step toward her: forage instead
  walking_to_ally_ = true;   // plugin.cpp keeps this step (not the late lookahead's)
  reason_ = "conveyor: walk to queen";
  return a;
}

}  // namespace gm2
