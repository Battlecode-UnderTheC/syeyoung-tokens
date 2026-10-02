// gst-cross-1 = gst-endgame-9 (arena gst-search-7) + queen-guard-7's queen guard and its plugin hooks.
// gst-search-6 = gst-search-4 (gst-merge-2 + arena-search-7's lookahead steering plain late moves)
// with gst-giant-2's rollout judge (rollout_giant2.hpp, tag variant/gst-giant-2; credit: the giant
// team) in place of gst-merge-2's older one: rollouts where the other side also lunges, a review
// of trapped actions against split escapes, a veto of steps into our own body, and a review of
// contested pearls (another head and a pearl in view).
#include <cstring>
#include <memory>

#include "plugin.hpp"
#include "rollout_giant2.hpp"
#include "strategy.hpp"
#include "queen_guard.hpp"
#include "window.hpp"

namespace gstsearch8 {
bot::Agent* MakeSearch7(const rl::Spawn& s);
// from this round, gst's plain one-step moves take their direction from arena-search-7's
// per-dragon lookahead (search7.cpp), unless the judge overrode the move, the dragon is trapped,
// walks to an ally to feed it, or steps into a dead end it chose to farm
constexpr int kLateRound = 250;
constexpr bool kReviewEscape = true;    // gst's trapped actions are reviewed with split candidates
constexpr bool kReviewContest = true;   // moves with another head and a pearl in view are reviewed
// (gst-search-8) ... but not once the team consolidates: feeders and the champion keep gst's moves
constexpr int kContestStopBefore = 0;   // rounds before gst's feed round the contest review stops
}  // namespace gstsearch8

namespace {

// (gst-cross-1) queen-guard-7's queen guard (queen_guard.hpp, from qg-board-1) and its plugin hooks
constexpr bool kYieldToQueen = true;   // non-queen dragons don't step beside our queen's head
constexpr bool kQueenGuard = true;     // queen_guard.hpp reviews every queen action

class JudgedAgent : public bot::Agent {
 public:
  explicit JudgedAgent(const rl::Spawn& s) : gst_(s), judge_(s), search_(gstsearch8::MakeSearch7(s)) {
    if (kQueenGuard && s.id <= 1) guard_ = std::make_unique<qg::QueenGuard>(s);   // our queen
  }
  bot::Action Turn(const rl::Observation& o) override {
    judge_.Observe(o);
    if (guard_) guard_->Observe(o);
    bot::Action a = gst_.Turn(o);
    why_ = gst_.reason();
    const bool move = a.kind == bot::Action::kMove && !a.resign && !a.steps.empty();
    const bool feeding = std::strstr(why_, "feed") != nullptr || std::strstr(why_, "champion") != nullptr;
    const bool trapped = std::strstr(why_, "trapped") != nullptr;
    // gm2: a farmer stepping into a dead end it chose (the tip split is the plan) is not reviewed
    const bool farming = gm2::kJudgeSkipsDeadEnds && std::strstr(why_, "dead end") != nullptr;
    bool over = false;
    const bot::Action mine = a;
    if (gm2::kJudge) {
      if (judge_.IntoOwnBody(a)) {   // e.g. a portal exit our own body now covers
        a = judge_.Review(o, a, &over, false, false, true);
        if (over) why_ = "rollout: own-body veto";
      } else if (gstsearch8::kReviewEscape && trapped) {
        a = judge_.Review(o, a, &over, true);
        if (over) why_ = a.kind == bot::Action::kSplit ? "rollout: escape split" : "rollout: escape move";
      } else if (move && !feeding && !farming && judge_.EnemyHeadInView(o)) {
        a = judge_.Review(o, a, &over);
        if (over) why_ = a.steps.size() > 1 ? "rollout: lunge" : "rollout: override";
      } else if (gstsearch8::kReviewContest && move && !feeding && !farming &&
                 o.round < gst_.FeedFrom() - gstsearch8::kContestStopBefore && judge_.OtherHeadAndPearlInView(o)) {
        a = judge_.Review(o, a, &over, false, true);
        if (over) why_ = "rollout: contest";
      }
      // never take a move override whose path our own memory says is not free
      if (over && a.kind == bot::Action::kMove && !a.resign && !a.steps.empty() &&
          (!gst_.FreeStep(a.steps[0]) || (a.steps.size() > 1 && !gst_.PathFree(a.steps)))) {
        a = mine;
        over = false;
        why_ = gst_.reason();
      }
    }
    const bot::Action b = search_->Turn(o);   // sees every observation from turn 1
    if (o.round >= gstsearch8::kLateRound && !over && !feeding && !trapped && !farming && !gst_.WalkingToAlly() &&
        a.kind == bot::Action::kMove && !a.resign && a.steps.size() == 1 && b.kind == bot::Action::kMove &&
        !b.resign && b.steps.size() == 1) {
      a.steps = b.steps;
      why_ = "late: lookahead step";
    }
    // (gst-endgame-3) a big dragon the lookahead runs out of a shorter enemy's lunge reach takes
    // the run (free steps only), unless it is feeding, trapped, farming or walking to an ally
    if (!guard_ && o.round >= gstsearch8::kLateRound && !feeding && !trapped && !farming && !gst_.WalkingToAlly() &&
        a.kind == bot::Action::kMove && !a.resign && a.steps.size() == 1 && b.kind == bot::Action::kMove &&
        !b.resign && b.steps.size() > 1 && (std::strcmp(search_->reason(), "escape sprint") == 0 || std::strcmp(search_->reason(), "harvest sprint") == 0) &&
        gst_.FreeStep(b.steps[0]) && gst_.PathFree(b.steps)) {
      a.steps = b.steps;
      why_ = std::strcmp(search_->reason(), "escape sprint") == 0 ? "late: escape sprint" : "late: harvest sprint";
    }
    // (gst-queen-guard-1) allies yield to the queen: a non-queen step that ends beside our queen's
    // head (it may step there too: a head-to-head with our own queen) takes another free step
    if (kYieldToQueen && !guard_ && a.kind == bot::Action::kMove && !a.resign && !a.steps.empty()) {
      int qx = -1, qy = -1;
      for (const rl::Segment& g : o.segments)
        if (g.head && g.team == o.spawn.team && g.id <= 1) { qx = g.x; qy = g.y; }
      if (qx >= 0) {
        auto beside = [&](int d) {
          const int n = win::Step(o, win::kCenter, d);
          if (n < 0) return false;
          const int dx = std::abs(o.tile_x[n] - qx), dy = std::abs(o.tile_y[n] - qy);
          return std::min(dx, o.spawn.width - dx) + std::min(dy, o.spawn.height - dy) <= 1;
        };
        if (beside(a.steps[0])) {
          for (int d = 0; d < 4; ++d) {
            const int n = win::Step(o, win::kCenter, d);
            if (d == a.steps[0] || n < 0 || win::Occupied(o, n) || !gst_.FreeStep(d) || beside(d)) continue;
            bot::Action y = bot::Action::Move(d);
            y.sonar = a.sonar;
            a = y;
            why_ = "yield to queen";
            break;
          }
        }
      }
    }
    // (gst-queen-guard-1) the queen's last word: forbidden actions replaced, rollouts against risk
    if (guard_) {
      bool changed = false;
      a = guard_->Guard(o, a, &changed);
      if (changed) why_ = a.kind == bot::Action::kSplit ? "queen guard: split" : "queen guard: step";
      guard_->Commit(a);
    }
    judge_.Commit(a);
    return a;
  }
  const char* reason() const override { return why_; }

 private:
  gm2::StrategyAgent gst_;
  gstg1r::Judge judge_;
  std::unique_ptr<bot::Agent> search_;
  std::unique_ptr<qg::QueenGuard> guard_;
  const char* why_ = "";
};

}  // namespace

LEAGUE_VARIANT(JudgedAgent)
