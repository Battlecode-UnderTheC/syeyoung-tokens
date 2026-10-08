// g46-1 (ring-field-1): the committed field-first opening.
//
// Evidence (strategist g46 A, 125 server elimination losses; docs/analysis-2026-10-07/REPORT.md §3):
// the material gap opens between r20 and r40 (eaten 8.5 v 14.4 at r30) while local play is the same;
// the difference is where the heads stand (starters 7.1 tiles from spawn at r20 v 10.7, pearls near
// our heads 6.8 v 9.7). Past fixes were score terms in ChooseGoal that nearby home food outbid, so
// this mode owns the goal outright.
//
// 1. Who: every non-queen dragon alive in round 0 (the starters and the children they split off in
//    round 0, which leave along the other end of the same body), except the queen's own round-0
//    child (its tail touches hers) and a dragon that already sees a field at spawn.
// 2. Heading: the reachable frontier (a known free tile beside a never-seen edge, or an unknown
//    portal edge) with the most never-seen tiles in view there per step, weighted by how well its
//    direction from our spawn agrees with the heading we hold (committed: the heading turns only
//    slowly), never one an ally head is nearer to (allies fan out), and never back toward spawn
//    while an outward one exists.
// 3. On the way: a pearl in view is eaten only when it costs at most one step off the path (2 extra
//    steps), and every free double step at length 5+ is taken (FreeSprint).
// 4. Arrive: kFieldBeds live or soon-due beds in the 7x7 window (kFieldGated after crossing a
//    portal): the mode ends there and the normal agent farms and splits in the field; its cluster
//    call (call.cpp) tells the team. A mode dragon whose frontier is used up walks to a fresh call.
// 5. Exit: on arrival, at kFieldUntil, or with no forward goal at all.
// Children of splits made on the way are ordinary agents (they farm where they are born).
// Observation only: no board-size gates, no fixed board points, no map keys.
#include <algorithm>
#include <cmath>
#include <cstdio>

#include "strategy.hpp"

namespace gm2 {
using namespace strat;

#ifndef FIELD_KEEPER
#define FIELD_KEEPER 1
#endif
#ifndef FIELD_HOMEFOOD
#define FIELD_HOMEFOOD 2.0
#endif
#ifndef FIELD_BARREN
#define FIELD_BARREN 1
#endif
namespace {
// the opening: strategist A's gap opens r20-r40; past r35 the scheduler owns the board again
#ifndef FIELD_UNTIL
#define FIELD_UNTIL 35
#endif
constexpr int32_t kFieldUntil = FIELD_UNTIL;
// a field: 4 pearls lying in view feed the arriving dragon and the child it splits off there (a
// 2-long dragon needs 2 pearls to reach its growth split at 4). Pearls lying now, not soon-due beds:
// a lane of slow beds always has a few due within 30 rounds by chance, and arriving there stopped
// the walk short of the dense fast field (sim, devil seed 1: arrived in a slow-bed lane at r9, 4
// tiles from the fast central beds), while fast beds carry pearls until someone eats them.
constexpr double kFieldBeds = 4.0;
// at spawn the dragon is already in a field when 4 beds in view are live or due within 30 rounds
// (BedWeight; the first gaps are fresh at r0, so a fast bed shows a small countdown). The brief said
// 3; 3 fired on a field of slow beds by chance (sim, trophy seed 2: the starter's view of 49 beds
// with gaps up to 1000 held 3 due within 30, so it never left; ~1.5 expected, P(>=3) about 20%,
// P(>=4) about 7%). Then the starter splits and farms as before.
constexpr double kSpawnBeds = 4.0;
// past a portal any 2 beds are a gated room nobody else reaches soon
constexpr double kFieldGated = 2.0;
constexpr int32_t kWindowTiles = (2 * 3 + 1) * (2 * 3 + 1);   // one 7x7 window
constexpr int32_t kBarrenTiles = 2 * kWindowTiles;              // two windows of new ground
constexpr int32_t kBarrenRound = 12;
constexpr double kHomeFood = FIELD_HOMEFOOD;
constexpr int32_t kStall = 6;          // rounds without a new tile seen: the frontier is shut (rush.cpp)
constexpr int32_t kView = 3;           // the 7x7 window's half-width
constexpr int32_t kDetour = 2;         // a pearl at most one step off the path: 2 extra steps
constexpr double kTurn = 0.3;
constexpr int32_t kAllyFresh = 8;      // an ally report this old still places it (call.cpp's kCallFresh)
constexpr double kAllyShare = 0.25;    // a frontier an ally head is nearer to is mostly its share          // how far one turn's choice pulls the held heading
}  // namespace

void StrategyAgent::FieldDelta(int32_t from, int32_t to, double* dx, double* dy) const {
  int32_t x = board_.X(to) - board_.X(from), y = board_.Y(to) - board_.Y(from);
  const int32_t w = board_.width(), h = board_.height();
  if (x > w / 2) x -= w;
  if (x < -w / 2) x += w;
  if (y > h / 2) y -= h;
  if (y < -h / 2) y += h;
  *dx = x;
  *dy = y;
}

// live_only: count the pearls lying in view now; else also beds due within 30 (BedWeight)
double StrategyAgent::FieldBedsInView(bool live_only) const {
  double s = 0;
  for (int32_t ey = -kView; ey <= kView; ++ey)
    for (int32_t ex = -kView; ex <= kView; ++ex) {
      const int32_t t = board_.Tile(board_.X(head_) + ex, board_.Y(head_) + ey);
      s += live_only ? (board_.Current(t) && board_.at(t).pearl ? 1.0 : 0.0) : BedWeight(t);
    }
  return s;
}

// the mode ends without a field: a dragon that would have been one of the g40-5 frontier explorers
// (rush.cpp) explores from here as it would have from r0, until its own rules or r30 end it. In
// arena draft 1 (no hand-over) weakhold went 3-13: in sim, seed 1, the parent's explorer reached
// the centre field (17,7) by r44 and the team had 19 dragons at r100; ours, back to normal play
// at r13, followed home calls inside the spawn pocket and the team had 6.
void StrategyAgent::FieldExit(const char* why) {
  field_ = 2;
  field_why_ = why;
  if (rush_deferred_ && rush_state_ != 1) {
    rush_state_ = 1;
    rush_since_ = round_;
    rush_seen_ = 0;
  }
  rush_deferred_ = false;
}

void StrategyAgent::UpdateField(const rl::Observation& o) {
#ifdef FIELD_DBG
  const int8_t was = field_;
  struct Log {
    const StrategyAgent* me; const int8_t& was;
    ~Log() {
      if (me->field_ != was && !(was == 0 && me->field_ == 2 && me->field_why_[0] == 0))
        std::fprintf(stderr, "FIELD r%d team%d id%d len%d head(%d,%d) %d->%d %s\n", me->round_, me->spawn_.team, me->spawn_.id,
                     me->length_, me->board_.X(me->head_), me->board_.Y(me->head_), was, me->field_, me->field_why_);
    }
  } log{this, was};
#endif
  if (IsQueen()) {
    field_ = 2;
    return;
  }
  if (field_ == 0) {
    field_ = 2;
    if (!FIELD_MODE || !leader_ || round_ != 0) return;
    // the queen's round-0 child: its tail touches her tail (docs/game.md, SPLIT)
    int32_t mine = -1;
    std::unordered_map<int32_t, int32_t> tails;
    for (const rl::Segment& s : o.segments)
      if (s.team == spawn_.team) tails[s.id] = board_.Tile(s.x, s.y);
    if (const auto it = tails.find(spawn_.id); it != tails.end()) mine = it->second;
    for (int q = 0; q < 2 && mine >= 0; ++q)
      if (const auto it = tails.find(q); it != tails.end())
        for (int d = 0; d < 4; ++d)
          if (board_.Step(mine, d) == it->second) {
            field_why_ = "queen child";
            return;
          }
    // g50-2: a spawn field keeps only the dragons it can feed. Every ally head in the 7x7 window
    // (ourselves included) shares these beds, and each eats one pearl at a time; a 2-long dragon
    // needs 2 pearls to reach its growth split at 4 (kHomeFood's reason), so the field holds us
    // only with 2 beds per head. eatrate-r24: on rich starts 87% of our worker-turns stayed home
    // (theirs 71%) while the mid zone yielded 0.22 eats per turn; this flag switched the field
    // opening off for every starter of a crowded spawn group (wg39001 224396).
    int32_t heads = 1;
    for (const rl::Segment& s : o.segments)
      if (s.head && s.team == spawn_.team && s.id != spawn_.id) ++heads;
    if (FieldBedsInView(false) >= std::max(kSpawnBeds, 2.0 * heads) - 1e-9) {
      field_why_ = "spawn field";
      return;
    }
    // g47-1: the home keeper. A round-0 child (its tail touches its parent's: docs/game.md, SPLIT)
    // whose spawn view holds kHomeFood live or soon-due beds stays and farms home while its parent
    // goes out, so each spawn group that has food keeps one farmer by it. Arena ring-field-2 lost
    // fixed-edges 0-8 and fixed-honeypot 0-8 (the parent: 77% and 81%): every non-queen r0 dragon
    // walked into the empty middle while the food lay round the spawns (sim, edges: total length
    // 31 v 53 at r50). kHomeFood 2: the pearls a 2-long child needs to reach its growth split at 4.
    if (FIELD_KEEPER && mine >= 0 && FieldBedsInView(false) >= kHomeFood - 1e-9)
      for (const auto& [id, t] : tails) {
        if (id == spawn_.id || id <= 1) continue;
        for (int d = 0; d < 4; ++d)
          if (board_.Step(mine, d) == t) {
            field_why_ = "home keeper";
            return;
          }
      }
    field_ = 1;
    field_spawn_ = head_;
    // the first heading is the way we face: a starter faces into the board, and a round-0 child
    // faces the other way along the same line (its head is the parent's old tail)
    field_hx_ = dragons::kDx[o.facing];
    field_hy_ = dragons::kDy[o.facing];
    field_since_ = 0;
    field_seen_ = 0;
    // the ground we stood on at spawn, and its pearl density (the home rate the walk must beat)
    field_counted_.assign(board_.size(), 0);
    int32_t home = 0, home_p = 0;
    for (int32_t t = 0; t < board_.size(); ++t)
      if (board_.at(t).seen_round >= 0) {
        field_counted_[t] = 1;
        ++home;
        home_p += board_.Current(t) && board_.at(t).pearl;
      }
    field_home_rate_ = home > 0 ? double(home_p) / home : 0.0;
    // (the founder dispatch, founder.cpp, waits while the mode runs and resumes after it)
  }
  if (field_ != 1) return;
  if (round_ >= kFieldUntil) {
    FieldExit("time");
    return;
  }
  // a portal crossed: last turn's head steps to this one, but the two are not neighbours on the board
  if (field_last_ >= 0 && field_last_ != head_ && Torus(field_last_, head_) > 1)
    for (int d = 0; d < 4; ++d) field_gated_ |= board_.Step(field_last_, d) == head_;
  field_last_ = head_;
  // the beds in view less one per ally head already there (the queen included): each eats one
  // pearl at a time, so only the surplus is a field for us (call.cpp's surplus rule). An ally is
  // there when seen now or reported within kAllyFresh rounds near our window.
  int32_t near = 0;
  for (const auto& [id, w] : seen_)
    if (id != spawn_.id && w.team == spawn_.team && w.head >= 0 && round_ - w.round <= kAllyFresh && Torus(w.head, head_) <= kView + 1) ++near;
  for (const auto& [id, r] : reports_)
    if (id != spawn_.id && r.tile >= 0 && r.length > 0 && round_ - r.round <= kAllyFresh && Torus(r.tile, head_) <= kView + 1 &&
        !(seen_.count(id) && round_ - seen_.at(id).round <= kAllyFresh))
      ++near;
  const double beds = FieldBedsInView(true) - near;
  if (beds >= kFieldBeds - 1e-9 || (field_gated_ && beds >= kFieldGated - 1e-9)) {
    field_ = 2;
    field_why_ = field_gated_ ? "arrived gated" : "arrived";
    return;
  }
  // no field on this heading: the ground the walk has uncovered holds pearls no denser than home.
  // Each tile counts once, with the pearl it shows when first seen. Judged once the walk has seen
  // kBarrenTiles new tiles (two full windows): one window can fall on an empty lane between fields
  // (Devil's kelp lanes), two in a row without a richer patch than home mean the heading crosses
  // sparse ground, and walking it on to r35 left the queen alone while home went unfarmed (arena
  // draft 1: weakhold 3-13, tower_defense 4-12, fresh sealed-spawn maps 7-17, total length at r50
  // 92 v 108). Home pearls are 0 on a start with nothing in view: then the walk exits only if it
  // has seen no pearl at all.
  for (int32_t ey = -kView; ey <= kView; ++ey)
    for (int32_t ex = -kView; ex <= kView; ++ex) {
      const int32_t t = board_.Tile(board_.X(head_) + ex, board_.Y(head_) + ey);
      if (field_counted_[t]) continue;
      field_counted_[t] = 1;
      ++field_new_;
      field_new_p_ += board_.at(t).pearl;
    }
  // Also judged at kBarrenRound whatever ground was seen: the top teams' starters are on the bed
  // clusters by r10-12 (STYLE.md: dt_devil_open, aa_trophy_open), and a walk that uncovered less
  // than one window of new ground by then is circling a cramped home, not crossing the board (arena
  // draft 1, sim weakhold seed 4: a starter wandered its home pocket on the heading until r35,
  // total length 8 v 92 at the end).
  if (FIELD_BARREN && (field_new_ >= kBarrenTiles || round_ >= kBarrenRound)) {
    const double rate = field_new_ > 0 ? double(field_new_p_) / field_new_ : 0.0;
    // ... and a walk that, from then on, is less than half a tile from spawn per round walked is
    // circling, not heading anywhere: the opponents' starters stand 10.7 tiles from spawn at r20
    // (strategist g46 A), half a tile a round; sim weakhold seed 4 (after the two rules above): a
    // starter that saw scattered pearls on new ground circled 6-9 tiles from spawn until r35.
    const bool circling = round_ >= kBarrenRound && 2 * Torus(field_spawn_, head_) < round_;
    if (circling || field_new_ < kWindowTiles || field_new_p_ == 0 || rate <= field_home_rate_ + 1e-9) {
      FieldExit("barren heading");
      return;
    }
  }
  int32_t seen = 0;
  for (int32_t t = 0; t < board_.size(); ++t) seen += board_.at(t).seen_round >= 0;
  if (seen > field_seen_) {
    field_seen_ = seen;
    field_since_ = round_;
  }
}

// the frontier tile on our heading (-1: none). The heading is held, not re-scored each turn: past
// tries that re-weighed it every turn turned toward whatever side lane was unseen (sim, devil seed
// 1, iteration 1: a starter walked east to the lanes 4 tiles short of the central field, then
// swung north up a lane as the heading followed its own choices). A frontier tile scores by the
// progress it makes along the heading per step of known path, then by the unseen ground in view
// there.
int32_t StrategyAgent::FieldFrontier(const Paths& paths) {
  const std::vector<int32_t> allies = AllyHeads(true);
  struct Cand { int32_t t, d, gain; bool theirs; };
  std::vector<Cand> cands;
  for (int32_t t = 0; t < board_.size(); ++t) {
    const int32_t d = paths.dist[t];
    if (d <= 0 || Banned(t) || Classify(t) != Cell::kFree) continue;
    bool frontier = false, portal = false;
    for (int k = 0; k < 4; ++k) {
      const bool unknown = board_.Step(t, k) == kUnknown;
      frontier |= unknown;
      portal |= unknown && (portal_[t] >> k & 1);
    }
    if (!frontier) continue;
    int32_t gain = 0;
    if (portal) {
      gain = (2 * kView + 1) * (2 * kView + 1);   // the far side is all unseen ground
    } else {
      const int32_t x = board_.X(t), y = board_.Y(t);
      for (int32_t ey = -kView; ey <= kView; ++ey)
        for (int32_t ex = -kView; ex <= kView; ++ex) gain += board_.at(board_.Tile(x + ex, y + ey)).seen_round < 0;
    }
    if (gain == 0) continue;
    bool theirs = false;
    for (const int32_t a : allies) theirs |= Torus(a, t) < d;
    cands.push_back({t, d, gain, theirs});
  }
  const double full = (2 * kView + 1) * (2 * kView + 1);
  int32_t best = -1;
  double best_score = 0;
  for (const Cand& c : cands) {
    double dx, dy;
    FieldDelta(head_, c.t, &dx, &dy);
    const double fwd = dx * field_hx_ + dy * field_hy_;
    if (fwd <= 0) continue;
    double score = (fwd / (c.d + 1.0)) * (1.0 + c.gain / full);
    if (c.theirs) score *= kAllyShare;
    if (c.t == field_target_) score *= 1.25;   // no dithering between near-equal frontiers
    if (score > best_score) best = c.t, best_score = score;
  }
  // no progress left on this heading (-1): the caller walks to a call or ends the mode. A second
  // heading by unseen ground per step only toured the home pocket (sim, devil seed 1, iteration 2:
  // the round-0 children facing the kelp at the board's edge explored their spawn room until r35,
  // eating only pearls on their path), while home food there is what the normal agent farms.
  if (best < 0) ++field_turns_;
  return best;
}

int32_t StrategyAgent::FieldGoal(const Paths& paths) {
  // the way on: the frontier on our heading. A dragon whose heading found nothing (no frontier
  // makes progress along it, or no new tile seen for kStall rounds) walks to a fresh cluster call
  // when there is one (an ally found a field), else the mode ends.
  field_call_ = false;
  int32_t target = round_ - field_since_ < kStall ? FieldFrontier(paths) : -1;
  if (target < 0)
    if (const int32_t c = CallGoal(paths, 0.0); c >= 0) target = c, field_call_ = true;
  if (target < 0) {
    FieldExit("no forward move");
#ifdef FIELD_DBG
    std::fprintf(stderr, "FIELD r%d team%d id%d len%d head(%d,%d) 1->2 no forward move\n", round_, spawn_.team, spawn_.id, length_,
                 board_.X(head_), board_.Y(head_));
#endif
    return -1;
  }
  field_target_ = target;
  // a pearl in view costing at most kDetour extra steps on the way to the target
  std::vector<int32_t> g(board_.size(), -1);
  std::vector<int32_t> queue{target};
  g[target] = 0;
  const int32_t limit = paths.dist[target] + kDetour + 2;
  for (size_t qi = 0; qi < queue.size(); ++qi) {
    const int32_t t = queue[qi];
    if (g[t] >= limit) continue;
    for (int d = 0; d < 4; ++d) {
      const int32_t n = board_.Step(t, d);
      if (n < 0 || g[n] >= 0 || (n != head_ && Classify(n) != Cell::kFree)) continue;
      g[n] = g[t] + 1;
      queue.push_back(n);
    }
  }
  int32_t food = -1;
  for (int32_t ey = -kView; ey <= kView; ++ey)
    for (int32_t ex = -kView; ex <= kView; ++ex) {
      const int32_t t = board_.Tile(board_.X(head_) + ex, board_.Y(head_) + ey);
      const int32_t d = paths.dist[t];
      if (d <= 0 || g[t] < 0 || Banned(t) || !board_.Current(t) || !board_.at(t).pearl) continue;
      if (d + g[t] > paths.dist[target] + kDetour) continue;
      int32_t ready;
      if (TileValue(t, d, &ready) <= 0 || ready > d) continue;
      if (food < 0 || d < paths.dist[food]) food = t;
    }
  return food >= 0 ? food : target;
}

}  // namespace gm2
