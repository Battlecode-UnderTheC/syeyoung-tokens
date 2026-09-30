// gst-giant-2: gather-split-trade-131 with a rollout judge (rollout.hpp). gst decides as before;
// when an enemy head is in view and gst's action is a move, the judge plays Monte Carlo rollouts
// on a forward model for gst's move and the alternatives, and overrides gst only when an
// alternative's rollout outcome is clearly better.
#include <cstring>

#include "plugin.hpp"
#include "rollout.hpp"
#include "strategy.hpp"

namespace {

constexpr bool kReviewEscape = true;    // gst's trapped actions are reviewed with split candidates
constexpr bool kReviewContest = true;   // moves with another head and a pearl in view are reviewed

class GiantJudgedAgent : public bot::Agent {
 public:
  explicit GiantJudgedAgent(const rl::Spawn& s) : gst_(s), judge_(s) {}
  bot::Action Turn(const rl::Observation& o) override {
    judge_.Observe(o);
    bot::Action a = gst_.Turn(o);
    why_ = gst_.reason();
    const bool move = a.kind == bot::Action::kMove && !a.resign && !a.steps.empty();
    const bool feeding = std::strstr(why_, "feed") != nullptr || std::strstr(why_, "champion") != nullptr;
    const bool trapped = std::strstr(why_, "trapped") != nullptr;
    bool over = false;
    if (judge_.IntoOwnBody(a)) {   // e.g. a portal exit our own body now covers
      a = judge_.Review(o, a, &over, false, false, true);
      if (over) why_ = "rollout: own-body veto";
    } else if (kReviewEscape && trapped) {
      a = judge_.Review(o, a, &over, true);   // escapes: moves and splits against gst's trapped action
      if (over) why_ = a.kind == bot::Action::kSplit ? "rollout: escape split" : "rollout: escape move";
    } else if (move && !feeding && judge_.EnemyHeadInView(o)) {
      a = judge_.Review(o, a, &over);
      if (over) why_ = a.steps.size() > 1 ? "rollout: lunge" : "rollout: override";
    } else if (kReviewContest && move && !feeding && judge_.OtherHeadAndPearlInView(o)) {
      a = judge_.Review(o, a, &over, false, true);   // contested pearls: another head and a pearl in view
      if (over) why_ = "rollout: contest";
    }
    judge_.Commit(a);
    return a;
  }
  const char* reason() const override { return why_; }

 private:
  gstg1::StrategyAgent gst_;
  gstg1r::Judge judge_;
  const char* why_ = "";
};

}  // namespace

LEAGUE_VARIANT(GiantJudgedAgent)
