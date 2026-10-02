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

namespace gstsearch8 {
bot::Agent* MakeSearch7(const rl::Spawn& s);
// from this round, gst's plain one-step moves take their direction from arena-search-7's
// per-dragon lookahead (search7.cpp), unless the judge overrode the move, the dragon is trapped,
// walks to an ally to feed it, or steps into a dead end it chose to farm
constexpr int kLateRound = 250;
constexpr bool kYieldToQueen = true;   // (gst-endgame-11)
constexpr bool kReviewEscape = true;    // gst's trapped actions are reviewed with split candidates
constexpr bool kReviewContest = true;   // moves with another head and a pearl in view are reviewed
// (gst-search-8) ... but not once the team consolidates: feeders and the champion keep gst's moves
constexpr int kContestStopBefore = 0;   // rounds before gst's feed round the contest review stops
}  // namespace gstsearch8

namespace {

class JudgedAgent : public bot::Agent {
 public:
  explicit JudgedAgent(const rl::Spawn& s) : gst_(s), judge_(s), search_(gstsearch8::MakeSearch7(s)) {}
  bot::Action Turn(const rl::Observation& o) override {
    judge_.Observe(o);
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
      } else if (move && !feeding && !farming && !gst_.Queen() && judge_.EnemyHeadInView(o)) {
        a = judge_.Review(o, a, &over);
        if (over) why_ = a.steps.size() > 1 ? "rollout: lunge" : "rollout: override";
      } else if (gstsearch8::kReviewContest && move && !feeding && !farming && !gst_.Queen() &&
                 o.round < gst_.FeedFrom() - gstsearch8::kContestStopBefore && judge_.OtherHeadAndPearlInView(o)) {
        a = judge_.Review(o, a, &over, false, true);
        if (over) why_ = "rollout: contest";
      }
      // (gst-endgame-11) the queen takes no judged lunge, and a judged step only if it is safe
      if (over && gst_.Queen() && a.kind == bot::Action::kMove && !a.resign &&
          (a.steps.size() != 1 || !gst_.QueenSafeStep(a.steps[0]))) {
        a = mine;
        over = false;
        why_ = gst_.reason();
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
        !b.resign && b.steps.size() == 1 && (!gst_.Queen() || gst_.QueenSafeStep(b.steps[0]))) {
      a.steps = b.steps;
      why_ = "late: lookahead step";
    }
    // (gst-endgame-3) a big dragon the lookahead runs out of a shorter enemy's lunge reach takes
    // the run (free steps only), unless it is feeding, trapped, farming or walking to an ally
    if (o.round >= gstsearch8::kLateRound && !feeding && !trapped && !farming && !gst_.WalkingToAlly() && !gst_.Queen() &&
        a.kind == bot::Action::kMove && !a.resign && a.steps.size() == 1 && b.kind == bot::Action::kMove &&
        !b.resign && b.steps.size() > 1 && (std::strcmp(search_->reason(), "escape sprint") == 0 || std::strcmp(search_->reason(), "harvest sprint") == 0) &&
        gst_.FreeStep(b.steps[0]) && gst_.PathFree(b.steps)) {
      a.steps = b.steps;
      why_ = std::strcmp(search_->reason(), "escape sprint") == 0 ? "late: escape sprint" : "late: harvest sprint";
    }
    // (gst-endgame-11) allies keep off our queen: a non-queen step that ends beside her head (or on
    // a tile she could step to) takes another free step, if there is one
    if (gstsearch8::kYieldToQueen && !gst_.Queen() && a.kind == bot::Action::kMove && !a.resign && !a.steps.empty()) {
      int qx = -1, qy = -1;
      for (const rl::Segment& g : o.segments)
        if (g.head && g.team == o.spawn.team && g.id <= 1 && g.id != o.spawn.id) { qx = g.x; qy = g.y; }
      if (qx >= 0) {
        auto near_queen = [&](int d) {
          int x = o.head_x(), y = o.head_y();
          x += dragons::kDx[d], y += dragons::kDy[d];
          int dx = std::abs(x - qx), dy = std::abs(y - qy);
          dx = std::min(dx, o.spawn.width - dx), dy = std::min(dy, o.spawn.height - dy);
          return dx + dy <= 2;
        };
        if (near_queen(a.steps[0]) && !std::strstr(why_, "trapped"))
          for (int d = 0; d < 4; ++d) {
            if (d == a.steps[0] || !gst_.FreeStep(d) || near_queen(d)) continue;
            bot::Action y = bot::Action::Move(d);
            y.sonar = a.sonar;
            a = y;
            why_ = "yield to queen";
            break;
          }
      }
    }
    judge_.Commit(a);
    return a;
  }
  const char* reason() const override { return why_; }

 private:
  gm2::StrategyAgent gst_;
  gstg1r::Judge judge_;
  std::unique_ptr<bot::Agent> search_;
  const char* why_ = "";
};

}  // namespace

LEAGUE_VARIANT(JudgedAgent)
