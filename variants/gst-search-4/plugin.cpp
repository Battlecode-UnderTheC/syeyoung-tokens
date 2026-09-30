// gst-search-4 = gst-merge-2 (a1a5672 on task/g26-2) + arena-search-7's lookahead steering plain late moves.
// gst-merge-2: gst-merge-1 (gm2::StrategyAgent) wrapped with the giant team's rollout judge
// (rollout.hpp, from gst-giant-1; credit: the giant team). gst decides as before; when an enemy
// head is in view and gst's action is a move (not feeding), the judge plays Monte Carlo rollouts
// on a forward model for gst's move and the alternatives, and overrides gst only when an
// alternative's outcome is clearly better. The wrapper is gst-giant-1's plugin, holding our agent.
#include <cstring>
#include <memory>

#include "plugin.hpp"
#include "rollout.hpp"
#include "strategy.hpp"

namespace gstsearch4 {
bot::Agent* MakeSearch7(const rl::Spawn& s);
// (gst-search-4) from this round, gst's plain one-step moves take their direction from
// arena-search-7's per-dragon lookahead (search7.cpp), unless the rollout judge overrode the
// move, the dragon walks to an ally to feed it, or it steps into a dead end it chose to farm
constexpr int kLateRound = 250;
}  // namespace gstsearch4

namespace {

class JudgedAgent : public bot::Agent {
 public:
  explicit JudgedAgent(const rl::Spawn& s) : gst_(s), judge_(s), search_(gstsearch4::MakeSearch7(s)) {}
  bot::Action Turn(const rl::Observation& o) override {
    judge_.Observe(o);
    bot::Action a = gst_.Turn(o);
    why_ = gst_.reason();
    const bool move = a.kind == bot::Action::kMove && !a.resign && !a.steps.empty();
    bool judged = false;
    const bool feeding = std::strstr(why_, "feed") != nullptr || std::strstr(why_, "champion") != nullptr;
    // gm2: a farmer stepping into a dead end it chose (the tip split is the plan) is not reviewed
    const bool farming = gm2::kJudgeSkipsDeadEnds && std::strstr(why_, "dead end") != nullptr;
    if (gm2::kJudge && move && !feeding && !farming && judge_.EnemyHeadInView(o)) {
      bool over = false;
      const bot::Action mine = a;
      a = judge_.Review(o, a, &over);
      // gm2: never take an override whose first step our own memory says is not free
      if (over && (!gst_.FreeStep(a.steps[0]) || (a.steps.size() > 1 && !gst_.PathFree(a.steps)))) {
        a = mine;
        over = false;
      }
      if (over) why_ = a.steps.size() > 1 ? "rollout: lunge" : "rollout: override";
      judged = over;
    }
    const bot::Action b = search_->Turn(o);   // sees every observation from turn 1
    if (o.round >= gstsearch4::kLateRound && !judged && !farming && !feeding && !gst_.WalkingToAlly() &&
        a.kind == bot::Action::kMove && !a.resign && a.steps.size() == 1 && b.kind == bot::Action::kMove &&
        !b.resign && b.steps.size() == 1) {
      a.steps = b.steps;
      why_ = "late: lookahead step";
    }
    return a;
  }
  const char* reason() const override { return why_; }

 private:
  gm2::StrategyAgent gst_;
  gm2r::Judge judge_;
  std::unique_ptr<bot::Agent> search_;
  const char* why_ = "";
};

}  // namespace

LEAGUE_VARIANT(JudgedAgent)
