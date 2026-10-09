// g42-1 (ring-clustercall-1): a team-wide early cluster call over sonar.
//
// Evidence (g41): on centre-cluster maps the winning line (ring-sched-8) grew from 5.8 to 12
// dragons in r20-40 by piling onto rich bed clusters before any contact; ring-sched-17's frontier
// explorers find such clusters, but only the finder farms them. Here the find is told to the team.
//
// 1. Find: before kCallUntil, any dragon (queen included: she sees too) scores each tile in its
//    7x7 view by ClusterScore (live or soon-due beds within 2, rush.cpp). The best tile is a find
//    when it has at least kCallMin beds and kCallSurplus more beds than our heads within
//    kContestReach of it (one dragon eats one pearl at a time: the surplus is food nobody of ours
//    is there to eat; 2 spare beds bring one 2-long worker to its growth split).
// 2. Word (64-bit, magic 0xA9): team (1) | x (6) | y (6) | surplus (5) | round found (9). Sent on
//    two opposite directions each turn (alternating N/S and E/W), so the champion word keeps the
//    other two (the finder: all four). Once the queen's ring is known every dragon relays the ring
//    word on all four directions instead; the call then rides in that word's spare bits 26-52.
//    Every dragon relays the best fresh call it heard (with its original round, so a call dies
//    kCallFresh rounds after its finder stops seeing the surplus).
// 3. Answer: a worker (not the queen, not a frontier explorer) farther than kCallReached from the
//    cluster, and not having seen it since the call, values the call as a known bed in a field of
//    `surplus` beds (ChooseGoal's cluster bonus) per (rounds to get there + 1), the travel a lower
//    bound (known path to a tile, then torus distance), and walks there when that beats the score
//    of the goal it chose. Once there it sees the beds and the scheduler takes over (exact ticks).
// Observation only: no board-size gates, no fixed board points, no map keys.
#include <algorithm>
#include <cstdio>

#include "strategy.hpp"

namespace gm2 {
using namespace strat;

namespace {
constexpr uint64_t kCallMagic = 0xA9;
constexpr int32_t kCallUntil = 40;     // the opening race for clusters (g41: held by r20-40)
constexpr int32_t kCallFresh = 8;      // a call not refreshed for this long: the finder left or allies came
constexpr int32_t kCallMin = 3;        // beds within 2: more than the frontier's 'worthwhile' 2 (rush.cpp)
// surplus beds over our heads there: a 2-long worker needs 2 pearls to reach the growth-split
// length 4, so 2 spare beds bring one more worker to its split (4 was tried: arena eval .47 v .52)
constexpr int32_t kCallSurplus = 2;
constexpr int32_t kCallNear = 4;       // our heads this near the cluster are already farming it (kContestReach)
constexpr int32_t kCallReached = 2;    // within the cluster's radius: the beds are in view, ChooseGoal decides
// g53-18 (strategist D g53 #3, rally at contact): in 2-starter opening collapses our heads were
// outnumbered within 4 tiles at 49% of contacts (1.77 v 2.41), theirs at 3%: every rule spreads
// our workers while theirs arrive in groups. A find with enemy heads within kCallNear (kContestReach)
// becomes a rally: its answerer cap is their heads + 1 (enough to outnumber them), and workers
// within kRallyReach answer it even when their own goal is food. 12: the find is relayed for
// kCallFresh (8) rounds; a worker 12 steps out still arrives within the window the find's rival
// heads take to eat a cluster of kCallMin+ beds (Devil 267190: lone walkers died r17/r25 while
// four farmed 8-14 steps away; Trophy 265787: three orange heads held the cross 2-4 from our founder).
constexpr int32_t kRallyReach = 12;
}  // namespace

uint64_t StrategyAgent::CallWord(int32_t tile, const Call& c) const {
  return (kCallMagic << 56) | (uint64_t(spawn_.team & 1) << 55) | (uint64_t(board_.X(tile) & 63) << 37) |
         (uint64_t(board_.Y(tile) & 63) << 31) | (uint64_t(std::min(c.value, 31)) << 26) | (uint64_t(c.round & 511) << 10) |
         (uint64_t(c.contested) << 9);
}

// the ring word (ring.cpp: magic 0xBA, bits 0-25) with the call in bits 36-55.
// g42-m: bits 26-35 carry ring-qgate-1's last enemy-queen sighting (round 26-34, valid 35), so the
// call moved up and was packed to the 20 free bits: x (6) | y (6) | surplus (4, capped at 15; 0 = no
// call) | round found mod 16 (4; a call older than kCallFresh = 8 is never relayed, so mod 16 is exact)
// g53-18: the surplus field is 3 bits (capped at 7) and bit 51 is the rally flag
uint64_t StrategyAgent::CallInRing(uint64_t ring_word, int32_t tile, const Call& c) const {
  ring_word &= ~(((uint64_t(1) << 20) - 1) << 36);
  return ring_word | (uint64_t(board_.X(tile) & 63) << 36) | (uint64_t(board_.Y(tile) & 63) << 42) |
         (uint64_t(std::min(c.value, 7)) << 48) | (uint64_t(c.contested) << 51) | (uint64_t(c.round & 15) << 52);
}

void StrategyAgent::HearCall(int32_t x, int32_t y, int32_t v, int32_t r, int32_t mask, bool contested) {
  // the low bits of the round: the latest round at or before now with those low bits
  r = round_ - ((round_ - r) & mask);
  if (x >= board_.width() || y >= board_.height() || r < 0 || v <= 0) return;
  Call& c = calls_[board_.Tile(x, y)];
  if (r > c.round || (r == c.round && v > c.value)) c = {v, r, contested || (r == c.round && c.contested)};
  else if (r == c.round) c.contested |= contested;
}

void StrategyAgent::UpdateCall(const rl::Observation& o) {
  own_call_ = -1;
  // what we heard (Decode only sees the word, so the inbox is read here)
  for (const uint64_t w : o.inbox) {
    if ((w >> 56) == kCallMagic && int32_t((w >> 55) & 1) == (spawn_.team & 1))
      HearCall(int32_t((w >> 37) & 63), int32_t((w >> 31) & 63), int32_t((w >> 26) & 31), int32_t((w >> 10) & 511), 511, (w >> 9) & 1);
    else if ((w >> 56) == 0xBA && int32_t((w >> 25) & 1) == (spawn_.team & 1) && ((w >> 48) & 7))
      HearCall(int32_t((w >> 36) & 63), int32_t((w >> 42) & 63), int32_t((w >> 48) & 7), int32_t((w >> 52) & 15), 15, (w >> 51) & 1);
  }
  for (auto it = calls_.begin(); it != calls_.end();)
    it = round_ - it->second.round > kCallFresh ? calls_.erase(it) : std::next(it);
  if (round_ >= kCallUntil) return;
  // our own find: the densest cluster centred in view
  int32_t best = -1;
  double best_v = 0;
  for (int k = 0; k < rl::kWindowTiles; ++k) {
    const int32_t t = board_.Tile(o.tile_x[k], o.tile_y[k]);
    const double v = ClusterScore(t);
    if (v > best_v) best_v = v, best = t;
  }
  if (best < 0 || best_v < kCallMin - 1e-9) return;
  int32_t ours = 0, theirs = 0;
  HeadsNear(best, kCallNear, &ours, &theirs);
  if (Torus(head_, best) > kCallNear) --ours;   // HeadsNear counts us whether or not we are near
  int32_t surplus = static_cast<int32_t>(best_v + 1e-9) - ours;
  // g53-18: enemy heads at the cluster make it a rally: enough answerers to outnumber them
  const bool contested = theirs > 0;
  if (contested) surplus = std::max(surplus, theirs + 1);
  if (surplus < kCallSurplus && !contested) return;
  own_call_ = best;
  calls_[best] = {surplus, round_, contested};
#ifdef CALL_DBG
  std::fprintf(stderr, "CALLFIND r%d team%d id%d at(%d,%d) head(%d,%d) V%.0f ours%d theirs%d surplus%d\n", round_, spawn_.team, spawn_.id,
               board_.X(best), board_.Y(best), board_.X(head_), board_.Y(head_), best_v, ours, theirs, surplus);
#endif
}

bool StrategyAgent::CallToSend(int32_t* tile, Call* c) const {
  if (own_call_ >= 0) {
    *tile = own_call_;
    *c = calls_.at(own_call_);
    return true;
  }
  int32_t best = -1;
  for (const auto& [t, k] : calls_) {
    if (round_ - k.round > kCallFresh) continue;
    if (best < 0 || k.value > calls_.at(best).value || (k.value == calls_.at(best).value && k.round > calls_.at(best).round))
      best = t;
  }
  if (best < 0) return false;
  *tile = best;
  *c = calls_.at(best);
  return true;
}

// g42-9: the answerers per call are capped by its surplus: one answerer per spare bed (a bed
// regrows, so each spare bed keeps one worker eating; the surplus counts only our heads at the
// find, not enemies who arrive later). A cap of surplus / kCallSurplus (2 beds per worker, the
// pearls one 2-long worker needs to split) was tried first: it lost the contested-column map
// prisoners_dilemma 0-12 vs ring-sched-27 (the capped workers stayed in the contact zone).
// Every dragon ranks the workers it knows of (in view now, or reported within kCallFresh; never
// the queens, ids 0/1) that are not yet at the cluster (beyond kCallNear: the ones within were
// counted against the surplus at the find) by (torus distance to the cluster, id), and answers
// only when it ranks within the cap. Allies we hold no fresh report of are not counted, so the
// cap errs towards more answerers.
bool StrategyAgent::CallMine(int32_t c, const Call& k) const {
  const int32_t cap = std::max(1, k.value), mine = Torus(head_, c);
  int32_t ahead = 0;
  auto count = [&](int32_t id, int32_t tile) {
    if (id <= 1 || id == spawn_.id || tile < 0) return;
    const int32_t d = Torus(tile, c);
    if (d <= kCallNear) return;
    if (d < mine || (d == mine && id < spawn_.id)) ++ahead;
  };
  for (const auto& [id, w] : seen_)
    if (w.team == spawn_.team && w.round == round_) count(id, w.head);
  for (const auto& [id, r] : reports_) {
    if (r.length <= 0 || round_ - r.round > kCallFresh) continue;
    if (const auto it = seen_.find(id); it != seen_.end() && it->second.round == round_) continue;
    count(id, r.tile);
  }
#ifdef CALL_DBG
  if (ahead >= cap && round_ < 30)
    std::fprintf(stderr, "CAPPED r%d team%d id%d head(%d,%d) call(%d,%d) v%d cap%d ahead%d\n", round_, spawn_.team, spawn_.id, board_.X(head_), board_.Y(head_),
                 board_.X(c), board_.Y(c), k.value, cap, ahead);
#endif
  return ahead < cap;
}

// the call this worker answers (-1: none), and its value as ChooseGoal values a known bed in a
// field (1 + cluster_bonus per other bed within cluster_radius), counting only the surplus beds
int32_t StrategyAgent::CallAnswered(double* value, bool* rally) const {
  if (rally) *rally = false;
  if (IsQueen() || rush_state_ == 1 || calls_.empty() || round_ >= kCallUntil + kCallFresh) return -1;
  // g53-18: a rally within kRallyReach comes first, whatever our own goal (food or a race): the
  // nearest one we rank within the cap for. As for any call, a dragon that has seen the cluster
  // since the find (the finder itself, or one passing by) is not an answerer: it is one of the
  // heads already counted there, and its own scoring of what it sees decides (iteration 1: with
  // the finder answering its own rally, Trophy 265787's lone walker id3 was drawn r13-25 to the
  // cross three orange heads held: one head arriving alone, the failure the rally is for).
  {
    int32_t best = -1, best_d = 1 << 20;
#ifdef RALLY_DBG
    if (round_ <= 40) {
      std::fprintf(stderr, "RALLYK r%d team%d id%d head(%d,%d) cg%d:", round_, spawn_.team, spawn_.id, board_.X(head_), board_.Y(head_), contest_goal_);
      for (const auto& [c, k] : calls_)
        std::fprintf(stderr, " (%d,%d)v%d r%d c%d d%d mine%d", board_.X(c), board_.Y(c), k.value, k.round, int(k.contested), Torus(head_, c), int(CallMine(c, k)));
      std::fprintf(stderr, "\n");
    }
#endif
    for (const auto& [c, k] : calls_) {
      if (!k.contested || round_ - k.round > kCallFresh) continue;
      const int32_t here = Torus(head_, c);
      if (here <= kCallReached || here > kRallyReach || here >= best_d || board_.at(c).seen_round >= k.round || !CallMine(c, k)) continue;
      best = c, best_d = here;
      *value = 1.0 + p_.cluster_bonus * (k.value - 1);
    }
    if (best >= 0) {
      if (rally) *rally = true;
      return best;
    }
  }
  // g42-m: a worker racing for food (last turn's contest goal, see Decide) answers no call
  if (contest_goal_ >= 0) return -1;
  int32_t best = -1;
  double top = 0;
  for (const auto& [c, k] : calls_) {
    if (round_ - k.round > kCallFresh) continue;
    const int32_t here = Torus(head_, c);
    if (here <= kCallReached) continue;
    // we have seen the cluster ourselves since the call (our own find, or we passed by): our own
    // valuation of its beds in ChooseGoal is as fresh as the call
    if (board_.at(c).seen_round >= k.round) continue;
    if (!CallMine(c, k)) continue;
    const double v = 1.0 + p_.cluster_bonus * (k.value - 1);
    if (v / (here + 1.0) > top) top = v / (here + 1.0), best = c, *value = v;
  }
  return best;
}

// the first tile of our way to the call we answer, if worth more than `beat` (-1: none). Since
// g42-9 only used when our chosen goal is not food (exploring, patrol, none): food goals carry
// the call as a detour bonus in ChooseGoal instead, so an answerer keeps eating on the way.
int32_t StrategyAgent::CallGoal(const Paths& paths, double beat) const {
  double v = 0;
  bool rally = false;
  const int32_t c = CallAnswered(&v, &rally);
  if (rally) beat = 0.0;   // g53-18: a rally is answered over any goal
  if (c < 0) return -1;
  const int32_t here = Torus(head_, c);
  // the known tile from which the cluster is nearest (known path, then torus distance)
  int32_t via = -1, eta = 1 << 20;
  for (int32_t t = 0; t < board_.size(); ++t) {
    const int32_t d = paths.dist[t];
    if (d <= 0 || Occupied(t) || Banned(t)) continue;
    const int32_t rest = Torus(t, c), e = d + rest;
    if (e < eta || (e == eta && rest < Torus(via, c))) eta = e, via = t;
  }
  if (via < 0 || Torus(via, c) >= here) return -1;   // no known way brings us nearer
  // g53-18 iteration 2: a rally is answered only when the way there (known walk, then straight
  // line) is within kRallyReach, not the straight line alone (duel-86b46c73 maze game 48: id11 at
  // (1,18) answered a rally 12 tiles away across the wrap, a far longer walk through the maze, and
  // left its farm; arrivals that late come one by one, the failure the rally is for)
  if (rally && eta > kRallyReach) return -1;
  const double s = v / (eta + 1.0);
#ifdef CALL_DBG
  std::fprintf(stderr, "CALLGO? r%d team%d id%d len%d to(%d,%d) here%d eta%d s%.3f beat%.3f %s\n", round_, spawn_.team, spawn_.id, length_,
               board_.X(c), board_.Y(c), here, eta, s, beat, s > beat ? "GO" : "no");
#endif
  return s > beat ? via : -1;
}

}  // namespace gm2
