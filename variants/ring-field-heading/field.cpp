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
// g51-1: a pearl in view counts toward a field only with a known path at most two window widths
// long (12 steps): a pearl in the 7x7 window is at most 6 steps away in the open, so a longer way
// round is a walled room entered from far side, not food in this field
constexpr int32_t kReach = 2 * (2 * kView);
// g51-1: a pearl group off the path is worth kDetour extra steps per pearl in it (the base's rate:
// one pearl pays for one step off the path and back), up to one field's worth (kFieldBeds pearls)
constexpr int32_t kHomeRadius = 2 * kView + 1;   // one 7x7 window width from our spawn head
constexpr int32_t kRoomRadius = 2;     // the pearls one detour collects: ClusterScore's radius
}  // namespace
#ifndef FIELD_HEADING
#define FIELD_HEADING 1
#endif

// g51-1: pearls lying in the 7x7 view around `at` that have a known path from our head within kReach
double StrategyAgent::FieldReachablePearls(int32_t at) const {
  const Paths p = Search(board_, head_, [&](int32_t t) { return !Occupied(t); }, kReach);
  double s = 0;
  for (int32_t ey = -kView; ey <= kView; ++ey)
    for (int32_t ex = -kView; ex <= kView; ++ex) {
      const int32_t t = board_.Tile(board_.X(at) + ex, board_.Y(at) + ey);
      if (board_.Current(t) && board_.at(t).pearl && p.dist[t] > 0) s += 1.0;
    }
  return s;
}

// g51-1 (ring-field-heading): the known field the walk heads for (-1: none), and its tile in
// field_known_. server-r25: on Around UNSW the outer starter's round-0 facing pointed west, away
// from the bed field an ally saw at spawn (and called over sonar); it walked to the edge and through
// a portal and never reached that field (0 of 26 server games), while the opponent arrived by r11.
// A field is known when
//  (a) an ally called it (call.cpp: 3+ live or soon-due beds within 2, 2+ more than our heads
//      there), and we rank within its answerer cap (CallMine), or
//  (b) we saw it ourselves: kFieldBeds pearls lying in a 7x7 window, each with a known path from
//      us, beyond our ally heads there (the arrival test, applied where we would stand).
// Only fields outside our spawn view count: those the opponent can contest; the ground we saw at
// spawn is farmed by the queen and the dragons left home. Each is reachable by kFieldUntil (the
// known path, then torus distance for a call's unseen part: CallGoal's lower bound), and the
// nearest wins: a field we know of beats the unknown ground ahead of us. While walking to it, the
// held heading points at it, so if it empties the walk goes on the same way.
int32_t StrategyAgent::FieldKnown(const Paths& paths) {
  field_known_ = -1;
  // our spawn group's ground: in our spawn view, or within one window width (kHomeRadius) of where
  // we spawned. The queen and the dragons the mode leaves home farm it (sim, around_unsw seed 1:
  // without this, five of team 0's walkers turned back to a call from the home bed patch 5 tiles
  // from their spawn and circled there until the barren exit at r12)
  // ... or within one window width of our queen (where we saw her last, or her last sonar report):
  // she farms where she is, with her round-0 child and the home keepers
  int32_t queen = -1, queen_round = -1;
  for (const auto& [id, w] : seen_)
    if (id <= 1 && w.team == spawn_.team && w.head >= 0 && w.round > queen_round) queen = w.head, queen_round = w.round;
  for (const auto& [id, r] : reports_)
    if (id <= 1 && r.tile >= 0 && r.length > 0 && r.round > queen_round) queen = r.tile, queen_round = r.round;
  auto Home = [&](int32_t c) {
    return field_home_[c] || Torus(field_spawn_, c) <= kHomeRadius || (queen >= 0 && Torus(queen, c) <= kHomeRadius);
  };
  const int32_t left = kFieldUntil - round_;
  if (left <= 0) return -1;
  int32_t best = -1, best_eta = left + 1, via_best = -1;
  // ally heads known now (queen included: she eats too)
  const std::vector<int32_t> allies = AllyHeads(false);
  auto allies_near = [&](int32_t c) {
    int32_t n = 0;
    for (const int32_t a : allies) n += Torus(a, c) <= kView + 1;
    return n;
  };
  // (a) calls
  for (const auto& [c, k] : calls_) {
#ifdef FIELD_DBG
    {
      int32_t nearer = 0;
      for (const int32_t a : allies) nearer += Torus(a, c) < Torus(head_, c);
      std::fprintf(stderr, "  CALL r%d team%d id%d c(%d,%d) v%d age%d home%d mine%d nearer%d dist%d\n", round_, spawn_.team, spawn_.id, board_.X(c), board_.Y(c),
                   k.value, round_ - k.round, int(field_home_[c]), int(CallMine(c, k)), nearer, Torus(head_, c));
    }
#endif
    // a call whose tile we have seen since it was made is judged by what we saw ((b) below, which
    // needs a known path to its pearls), as call.cpp's CallAnswered does: sim, stripes seed 1, a
    // call from a sealed box (pearls behind kelp all round) drew the walker off its way at r11
    if (round_ - k.round > 8 || Home(c) || Torus(head_, c) <= 2 || board_.at(c).seen_round >= k.round || !CallMine(c, k)) continue;
    int32_t via = -1, eta = 1 << 20;
    for (int32_t t = 0; t < board_.size(); ++t) {
      const int32_t d = paths.dist[t];
      if (d < 0 || (d > 0 && (Occupied(t) || Banned(t)))) continue;
      const int32_t e = d + Torus(t, c);
      if (e < eta || (e == eta && via >= 0 && Torus(t, c) < Torus(via, c))) eta = e, via = t;
    }
    if (via < 0 || via == head_ || eta >= best_eta) continue;
    best = c, best_eta = eta, via_best = via;
  }
  // (b) fields we saw: a bed tile with a known path, kFieldBeds reachable pearls in the window round it
  for (int32_t t = 0; t < board_.size(); ++t) {
    const int32_t d = paths.dist[t];
    if (d <= 2 || d >= best_eta || Home(t) || !board_.at(t).pearl || Banned(t)) continue;
    int32_t n = 0;
    for (int32_t ey = -kView; ey <= kView; ++ey)
      for (int32_t ex = -kView; ex <= kView; ++ex) {
        const int32_t u = board_.Tile(board_.X(t) + ex, board_.Y(t) + ey);
        n += board_.at(u).pearl && paths.dist[u] > 0 && board_.at(u).seen_round >= 0;
      }
    if (n - allies_near(t) < kFieldBeds - 1e-9) continue;
    best = t, best_eta = d, via_best = t;
  }
  if (best < 0) return -1;
  field_known_ = best;
  // the held heading now points at the field
  double dx, dy;
  FieldDelta(head_, best, &dx, &dy);
  const double len = std::sqrt(dx * dx + dy * dy);
  if (len > 0) {
    field_hx_ = dx / len;
    field_hy_ = dy / len;
  }
#ifdef FIELD_DBG
  std::fprintf(stderr, "FKNOWN r%d team%d id%d head(%d,%d) field(%d,%d) via(%d,%d) eta%d %s\n", round_, spawn_.team, spawn_.id, board_.X(head_),
               board_.Y(head_), board_.X(best), board_.Y(best), board_.X(via_best), board_.Y(via_best), best_eta,
               calls_.count(best) && via_best != best ? "call" : "seen");
#endif
  return via_best;
}

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
    if (FieldBedsInView(false) >= kSpawnBeds - 1e-9) {
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
    field_home_.assign(board_.size(), 0);
    int32_t home = 0, home_p = 0;
    for (int32_t t = 0; t < board_.size(); ++t)
      if (board_.at(t).seen_round >= 0) {
        field_counted_[t] = 1;
        field_home_[t] = 1;
        ++home;
        home_p += board_.Current(t) && board_.at(t).pearl;
      }
    field_home_rate_ = home > 0 ? double(home_p) / home : 0.0;
    // (the founder dispatch, founder.cpp, waits while the mode runs and resumes after it)
  }
#ifdef FIELD_DBG
  if (field_ == 1) {
    std::string rs;
    for (const auto& [id, r] : reports_) if (id != spawn_.id && r.tile >= 0) rs += " " + std::to_string(id) + "@" + std::to_string(board_.X(r.tile)) + "," + std::to_string(board_.Y(r.tile)) + "/r" + std::to_string(r.round);
    std::fprintf(stderr, "FKNOW r%d team%d id%d inbox%zu calls%zu reports:%s\n", round_, spawn_.team, spawn_.id, o.inbox.size(), calls_.size(), rs.c_str());
  }
#endif
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
  // g51-1: only pearls we can walk to count (FieldReachablePearls): Stripes' sealed boxes showed 4
  // pearls in view at r8 and ended the walk beside food nobody can reach (sim, stripes seed 1)
  const double beds = (FIELD_HEADING ? FieldReachablePearls(head_) : FieldBedsInView(true)) - near;
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
  int32_t target = FIELD_HEADING ? FieldKnown(paths) : -1;   // g51-1: a known field first
  if (target < 0) target = round_ - field_since_ < kStall ? FieldFrontier(paths) : -1;
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
  const int32_t room_max = FIELD_HEADING ? kDetour * static_cast<int32_t>(kFieldBeds) : kDetour;
  const int32_t limit = paths.dist[target] + room_max + 2;
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
  int32_t food = -1, food_extra = 0;
  for (int32_t ey = -kView; ey <= kView; ++ey)
    for (int32_t ex = -kView; ex <= kView; ++ex) {
      const int32_t t = board_.Tile(board_.X(head_) + ex, board_.Y(head_) + ey);
      const int32_t d = paths.dist[t];
      if (d <= 0 || g[t] < 0 || Banned(t) || !board_.Current(t) || !board_.at(t).pearl) continue;
      // g51-1: the detour a pearl may cost grows with the pearls lying round it (a walled room's
      // pearls are one detour; server-r25 Stripes: the walker passed the pearl rooms by its path)
      int32_t room = 1;
      if (FIELD_HEADING) {
        room = 0;
        for (int32_t ry = -kRoomRadius; ry <= kRoomRadius; ++ry)
          for (int32_t rx = -kRoomRadius + std::abs(ry); rx <= kRoomRadius - std::abs(ry); ++rx) {
            const int32_t u = board_.Tile(board_.X(t) + rx, board_.Y(t) + ry);
            room += board_.Current(u) && board_.at(u).pearl && paths.dist[u] > 0;
          }
      }
      const int32_t extra = d + g[t] - paths.dist[target];
      if (extra > std::min(room_max, kDetour * std::max(1, room))) continue;
      int32_t ready;
      if (TileValue(t, d, &ready) <= 0 || ready > d) continue;
      if (food < 0 || d < paths.dist[food]) food = t, food_extra = extra;
    }
  field_room_ = food >= 0 && food_extra > kDetour;
#ifdef FIELD_DBG
  if (food >= 0 && food_extra > kDetour)
    std::fprintf(stderr, "FROOM r%d team%d id%d head(%d,%d) pearl(%d,%d) extra%d\n", round_, spawn_.team, spawn_.id, board_.X(head_), board_.Y(head_),
                 board_.X(food), board_.Y(food), food_extra);
#endif
  return food >= 0 ? food : target;
}

}  // namespace gm2
