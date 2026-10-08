// gst-search-6 = gst-search-4 (gst-merge-2 + arena-search-7's lookahead steering plain late moves)
// with gst-giant-2's rollout judge (rollout_giant2.hpp, tag variant/gst-giant-2; credit: the giant
// team) in place of gst-merge-2's older one: rollouts where the other side also lunges, a review
// of trapped actions against split escapes, a veto of steps into our own body, and a review of
// contested pearls (another head and a pearl in view).
#include <cstdio>
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
constexpr bool kReviewEscape = true;    // gst's trapped actions are reviewed with split candidates
constexpr bool kReviewContest = true;   // moves with another head and a pearl in view are reviewed
// (gst-search-8) ... but not once the team consolidates: feeders and the champion keep gst's moves
constexpr int kContestStopBefore = 0;   // rounds before gst's feed round the contest review stops
}  // namespace gstsearch8

#ifdef LUNGE_DBG
#include <atomic>
#include <cstdio>
#include <map>
#include <mutex>
#include <string>
namespace gm2 { extern std::atomic<long> g_lunge_goal_veto, g_lf_enemy, g_lf_worth, g_lf_partial, g_lf_middle; }
static std::mutex g_ld_mu;
static std::map<std::string, long> g_ld_final;   // final step in lunge reach (with a clear alternative), by reason
static std::atomic<long> g_ld_turns{0}, g_ld_avoidable{0};
static struct LungeReport {
  ~LungeReport() {
    std::fprintf(stderr, "LUNGE 6+ worker turns %ld, safe-step goal vetoes %ld, final steps in avoidable reach %ld\n",
                 g_ld_turns.load(), gm2::g_lunge_goal_veto.load(), g_ld_avoidable.load());
    std::fprintf(stderr, "LUNGEFIELD enemies %ld worth %ld (middle %ld) partial %ld\n", gm2::g_lf_enemy.load(), gm2::g_lf_worth.load(), gm2::g_lf_middle.load(), gm2::g_lf_partial.load());
    for (const auto& [k, v] : g_ld_final) std::fprintf(stderr, "LUNGE   final in reach [%s] %ld\n", k.c_str(), v);
  }
} g_lunge_report;
#endif

namespace {

class JudgedAgent : public bot::Agent {
 public:
  explicit JudgedAgent(const rl::Spawn& s) : gst_(s), judge_(s), search_(gstsearch8::MakeSearch7(s)) {}
  bot::Action Turn(const rl::Observation& o) override {
#ifdef CONVEYOR_DBG
    bot::Action r = Turn2(o);
    if (o.round >= 420) std::fprintf(stderr, "CV r%d team%d id%d len%d qd%d kind%d resign%d steps%d why=%s\n", int(o.round), int(o.spawn.team), int(o.spawn.id), int(o.length), gst_.DbgQueenDist(), int(r.kind), int(r.resign), int(r.steps.size()), why_);
    return r;
  }
  bot::Action Turn2(const rl::Observation& o) {
#endif
#ifdef FIELD_DBG
    bot::Action r = Turn2(o);
    if (o.round < 45) std::fprintf(stderr, "FT r%d team%d id%d len%d units%d f%d head%d,%d steps%d why=%s\n", int(o.round), int(o.spawn.team), int(o.spawn.id), int(o.length), int(o.unit_count), gst_.Field(), int(o.head_x()), int(o.head_y()), int(r.steps.size()), why_);
    return r;
  }
  bot::Action Turn2(const rl::Observation& o) {
#endif
#ifdef FOUNDER_DBG
    bot::Action r = Turn2(o);
    if (o.round < 70) std::fprintf(stderr, "FD r%d team%d id%d len%d units%d f%d head%d,%d why=%s\n", int(o.round), int(o.spawn.team), int(o.spawn.id), int(o.length), int(o.unit_count), gst_.Founder(), int(o.head_x()), int(o.head_y()), why_);
    return r;
  }
  bot::Action Turn2(const rl::Observation& o) {
#endif
    judge_.Observe(o);
    bot::Action a = gst_.Turn(o);
    why_ = gst_.reason();
    if (gst_.IsQueen()) return a;
    if(RING12_ATTACK_MODE && !gst_.RescueCommitted()) {
      bot::Action attack;
      if(gst_.TryQueenAttack(o,&attack)) {
        attack.sonar=a.sonar;
        why_="queen12: direct attack";
        return attack; // A confirmed head collision kills this worker; no future history.
      }
    }
    if (gst_.RescueCommitted()) {
      a = gst_.ProtectQueenAction(a, a);
      why_ = gst_.reason();
      search_->Turn(o); // keep the secondary policy's observation history current
      judge_.Commit(a); // remember every actual sprint step before the next observation
      return a;
    }
    const bool move = a.kind == bot::Action::kMove && !a.resign && !a.steps.empty();
    const bool feeding = std::strstr(why_, "feed") != nullptr || std::strstr(why_, "champion") != nullptr;
    const bool trapped = std::strstr(why_, "trapped") != nullptr;
    // g40-6: a worker stepping aside from (v2: or around) a bed before its tick keeps that step (not the lookahead's)
    const bool bed_wait = std::strstr(why_, "bed: ") != nullptr;
    // gm2: a farmer stepping into a dead end it chose (the tip split is the plan) is not reviewed
    const bool farming = gm2::kJudgeSkipsDeadEnds && std::strstr(why_, "dead end") != nullptr;
    bool over = false;
    const bot::Action mine = a;
    const char* mine_why = why_;
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
    if (o.round >= gstsearch8::kLateRound && !over && !feeding && !trapped && !farming && !bed_wait && !gst_.WalkingToAlly() &&
        a.kind == bot::Action::kMove && !a.resign && a.steps.size() == 1 && b.kind == bot::Action::kMove &&
        !b.resign && b.steps.size() == 1) {
      a.steps = b.steps;
      why_ = "late: lookahead step";
    }
    // g43-7 (lungeguard): the lookahead (from r250) and the rollout judge replace gst's step without
    // the lunge field; a long worker keeps gst's own step when theirs ends in an untradeable enemy's
    // lunge reach and gst's does not (a multi-step lunge of ours is left alone: it is an attack)
    if (a.kind == bot::Action::kMove && !a.resign && a.steps.size() == 1 && gst_.LungeAvoid(a.steps[0]) &&
        mine.kind == bot::Action::kMove && !mine.resign && mine.steps.size() == 1 && gst_.LungeClear(mine.steps[0]) &&
        gst_.FreeStep(mine.steps[0])) {
      const bot::Action keep = mine;
      a.steps = keep.steps;
      why_ = "lungeguard: keep gst step";
    }
    // g43-7 v2: likewise gst's escape run is kept against a single step that stays in reach
    if (mine.kind == bot::Action::kMove && !mine.resign && mine.steps.size() > 1 && std::strcmp(mine_why, "lungeguard: escape run") == 0 &&
        a.kind == bot::Action::kMove && !a.resign && a.steps.size() == 1 && gst_.LungeFlagged(a.steps[0]) && gst_.PathFree(mine.steps)) {
      a.steps = mine.steps;
      why_ = "lungeguard: keep escape run";
    }
#ifdef LUNGE_DBG
    const char* pre = why_;
#endif
    a = gst_.ProtectQueenAction(a, mine);
    why_ = gst_.reason();
#ifdef LUNGE_DBG
    if (std::strcmp(mine_why, "lungeguard: escape run") == 0) {
      std::lock_guard<std::mutex> l(g_ld_mu);
      ++g_ld_final[a.steps == mine.steps ? "RUN taken" : "RUN replaced"];
    }
    if (gst_.Length() >= 6) {
      ++g_ld_turns;
      if (a.kind == bot::Action::kMove && !a.resign && !a.steps.empty() && gst_.LungeAvoid(a.steps[0])) {
        ++g_ld_avoidable;
        std::lock_guard<std::mutex> l(g_ld_mu);
        ++g_ld_final[pre];
      }
      if (a.kind == bot::Action::kMove && !a.resign && a.steps.size() == 1 && gst_.DbgExposedStep(a.steps[0])) {
        std::lock_guard<std::mutex> l(g_ld_mu);
        ++g_ld_final[std::string("ADJ ") + (gst_.DbgSafeAlt() ? "alt " : "noalt ") + pre];
      }
    }
#endif
    judge_.Commit(a);
#ifdef SPRINT_DBG
    if (a.kind == bot::Action::kMove && !a.resign && !a.steps.empty())
      std::fprintf(stderr, "SPR r%d len%d steps%d %s\n", o.round, gst_.Length(), int(a.steps.size()), why_);
#endif
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
