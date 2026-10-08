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
#include <cstdlib>
#include <vector>

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
// g50-3: a find counts the pearls seen plus beds due within kCallDue rounds. eatrate-r24: with
// rush.cpp's 30-round horizon, spawn views whose beds refill every 13-46 rounds were "finds" at r0
// (224396) and answerers walked 16 tiles to beds that were still counting down (224415); call
// travel was 3.2% of turns and ate nothing. 8 rounds is about the walk an answerer makes (calls are
// answered from kCallReached..~10 tiles), so a bed due within 8 is ripe when the answerer arrives.
constexpr int32_t kCallDue = 8;
// g50-3: a worker seeing this many pearls in its own view that no other head in view is nearer to
// has food now and doesn't leave it for a call (224415: worker 17 was called 16 tiles out of its
// field at r13). 2: one pearl is a single bite (the cluster's surplus is worth more); two are its
// growth split (kCallSurplus' reasoning).
constexpr int32_t kStayPearls = 2;
constexpr int32_t kCallView = 3;       // the 7x7 window's half-width (docs/game.md)
}  // namespace

uint64_t StrategyAgent::CallWord(int32_t tile, const Call& c) const {
  return (kCallMagic << 56) | (uint64_t(spawn_.team & 1) << 55) | (uint64_t(board_.X(tile) & 63) << 37) |
         (uint64_t(board_.Y(tile) & 63) << 31) | (uint64_t(std::min(c.value, 31)) << 26) | (uint64_t(c.round & 511) << 10);
}

// the ring word (ring.cpp: magic 0xBA, bits 0-25) with the call in bits 36-55.
// g42-m: bits 26-35 carry ring-qgate-1's last enemy-queen sighting (round 26-34, valid 35), so the
// call moved up and was packed to the 20 free bits: x (6) | y (6) | surplus (4, capped at 15; 0 = no
// call) | round found mod 16 (4; a call older than kCallFresh = 8 is never relayed, so mod 16 is exact)
uint64_t StrategyAgent::CallInRing(uint64_t ring_word, int32_t tile, const Call& c) const {
  ring_word &= ~(((uint64_t(1) << 20) - 1) << 36);
  return ring_word | (uint64_t(board_.X(tile) & 63) << 36) | (uint64_t(board_.Y(tile) & 63) << 42) |
         (uint64_t(std::min(c.value, 15)) << 48) | (uint64_t(c.round & 15) << 52);
}

void StrategyAgent::HearCall(int32_t x, int32_t y, int32_t v, int32_t r, int32_t mask) {
  // the low bits of the round: the latest round at or before now with those low bits
  r = round_ - ((round_ - r) & mask);
  if (x >= board_.width() || y >= board_.height() || r < 0 || v <= 0) return;
  Call& c = calls_[board_.Tile(x, y)];
  if (r > c.round || (r == c.round && v > c.value)) c = {v, r};
}

double StrategyAgent::CallBedWeight(int32_t t) const {
  const TileMemory& m = board_.at(t);
  if (m.seen_round < 0) return 0.0;
  if (m.pearl) return 1.0;
  if (m.countdown < 0) return 0.0;
  return m.countdown - (round_ - m.seen_round) <= kCallDue ? 1.0 : 0.0;
}

double StrategyAgent::CallClusterScore(int32_t t) const {
  double s = 0;
  const int32_t x = board_.X(t), y = board_.Y(t);
  for (int32_t ey = -2; ey <= 2; ++ey)
    for (int32_t ex = -2 + std::abs(ey); ex <= 2 - std::abs(ey); ++ex) s += CallBedWeight(board_.Tile(x + ex, y + ey));
  return s;
}

int32_t StrategyAgent::UnmatchedPearlsInView() const {
  int32_t n = 0;
  for (int32_t ey = -kCallView; ey <= kCallView; ++ey)
    for (int32_t ex = -kCallView; ex <= kCallView; ++ex) {
      const int32_t t = board_.Tile(board_.X(head_) + ex, board_.Y(head_) + ey);
      if (!board_.Current(t) || !board_.at(t).pearl) continue;
      const int32_t mine = Torus(head_, t);
      bool other = false;   // another head in view (ally or enemy) nearer: that pearl is its
      for (const auto& [id, w] : seen_)
        if (id != spawn_.id && w.round == round_ && w.head >= 0 && Torus(w.head, t) < mine) other = true;
      n += !other;
    }
  return n;
}

void StrategyAgent::UpdateCall(const rl::Observation& o) {
  own_call_ = -1;
  // g50-3: our spawn ground (the first turn of a dragon that starts in the opening)
  // (ablation g50-3c: as briefed, the dragon's own first-turn view in the opening only; iterations
  // 2-3 widened this to allies' views and every first turn, and the r200-300 eval fell .52 -> .45)
  if (round_ == born_ && round_ <= 2 && call_home_.empty()) {
    call_home_.assign(board_.size(), 0);
    for (int32_t t = 0; t < board_.size(); ++t) call_home_[t] = board_.at(t).seen_round >= 0;
  }
  // what we heard (Decode only sees the word, so the inbox is read here)
  for (const uint64_t w : o.inbox) {
    if ((w >> 56) == kCallMagic && int32_t((w >> 55) & 1) == (spawn_.team & 1))
      HearCall(int32_t((w >> 37) & 63), int32_t((w >> 31) & 63), int32_t((w >> 26) & 31), int32_t((w >> 10) & 511), 511);
    else if ((w >> 56) == 0xBA && int32_t((w >> 25) & 1) == (spawn_.team & 1) && ((w >> 48) & 15))
      HearCall(int32_t((w >> 36) & 63), int32_t((w >> 42) & 63), int32_t((w >> 48) & 15), int32_t((w >> 52) & 15), 15);
  }
  for (auto it = calls_.begin(); it != calls_.end();)
    it = round_ - it->second.round > kCallFresh ? calls_.erase(it) : std::next(it);
  if (round_ >= kCallUntil) return;
  // our own find: the densest cluster centred in view
  int32_t best = -1;
  double best_v = 0;
  for (int k = 0; k < rl::kWindowTiles; ++k) {
    const int32_t t = board_.Tile(o.tile_x[k], o.tile_y[k]);
    if (!call_home_.empty() && call_home_[t]) continue;   // g50-3: our own spawn ground
    const double v = CallClusterScore(t);
    if (v > best_v) best_v = v, best = t;
  }
  if (best < 0 || best_v < kCallMin - 1e-9) return;
  int32_t ours = 0, theirs = 0;
  HeadsNear(best, kCallNear, &ours, &theirs);
  if (Torus(head_, best) > kCallNear) --ours;   // HeadsNear counts us whether or not we are near
  const int32_t surplus = static_cast<int32_t>(best_v + 1e-9) - ours;
  if (surplus < kCallSurplus) return;
  own_call_ = best;
  calls_[best] = {surplus, round_};
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
int32_t StrategyAgent::CallAnswered(double* value) const {
  if (IsQueen() || rush_state_ == 1 || calls_.empty() || round_ >= kCallUntil + kCallFresh) return -1;
  // g42-m: a worker racing for food (last turn's contest goal, see Decide) answers no call
  if (contest_goal_ >= 0) return -1;
  // g50-3: a worker with food of its own in view stays with it
  if (UnmatchedPearlsInView() >= kStayPearls) return -1;
  // g50-3 (iteration 2): a worker that is itself calling a find this turn is in a field with spare
  // beds; it stays (wg39000/wg39001 repro: 84-310 GO turns per 6 games were by a worker answering
  // another call while its own view held a find)
  if (own_call_ >= 0) return -1;
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
  const int32_t c = CallAnswered(&v);
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
  const double s = v / (eta + 1.0);
#ifdef CALL_DBG
  std::fprintf(stderr, "CALLGO? r%d team%d id%d len%d to(%d,%d) here%d eta%d s%.3f beat%.3f %s\n", round_, spawn_.team, spawn_.id, length_,
               board_.X(c), board_.Y(c), here, eta, s, beat, s > beat ? "GO" : "no");
#endif
  return s > beat ? via : -1;
}

}  // namespace gm2
