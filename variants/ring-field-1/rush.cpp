// g40-5 (ring-frontier-1): explore the frontier instead of rushing the board's centre.
//
// g39 evidence: the geometric-centre rush (ring-sched-m2) won on known maps but lost 64-96 to the
// same code without it on 40 fresh anti-centre maps: the rushers walked ~20-30 rounds without
// eating or splitting, into centre traps, and stalled on walls. A fixed point is a guess about the
// map; what a dragon can observe is where it has not looked yet.
//
// 1. Explorers: the two lowest-id non-queen starters (map dragons alternate teams, so ours are
//    q, q+2, q+4, ...). Each turn an explorer heads for the reachable frontier tile (known free,
//    beside an edge never seen) with the most never-seen tiles in the 7x7 view it would have
//    there, per step of known path to it. Frontier tiles an ally head is nearer to (within
//    kSpread) are left to that ally, so the explorers fan out.
// 2. The opening is kept: food within kFoodDetour steps is taken first, and an explorer may make
//    one growth split (strategy.cpp step 4).
// 3. Exploring stops when a worthwhile cluster (kClusterMin live or soon-due beds within 2) is
//    seen: the scheduler takes over; or a bed in reach of an enemy head: the explorer holds it to
//    r60 (the contest); or when 6 rounds pass with no new tile seen; or at r30.
// 4. Contest (kept from g39-4, its zone now observed): before r60 a worker within kContestRadius
//    of a dense cluster counts heads within 4 for the parity and trade rules.
// Observation only: no board-size gates, no fixed board points, no map keys.
#include <algorithm>
#include <cstring>

#include "strategy.hpp"

namespace gm2 {
using namespace strat;

namespace {
constexpr int32_t kRushUntil = 30;       // the opening: the scheduler owns the board after this
constexpr int32_t kRushStall = 6;        // rounds without seeing a new tile: the frontier is shut
constexpr double kClusterMin = 2.0;      // two live beds within 2: worth farming instead of exploring
constexpr int32_t kContestReach = 4;     // an enemy head this near a bed contests it (HeadsNear's radius)
constexpr int32_t kContestRadius = 4;    // the contest zone around a dense cluster (same radius)
constexpr int32_t kMiddleUntil = 60;     // the hold and the parity rule end here (as in g39-4)
constexpr int32_t kSpread = 6;           // a frontier this near a nearer ally head is that ally's
constexpr int32_t kFoodDetour = 2;       // food on or beside the path: at most a 1-tile detour
constexpr int32_t kView = 3;             // the 7x7 window's half-width (docs/game.md)
constexpr double kKeep = 1.25;           // the current frontier wins near-ties (no dithering)
}  // namespace

int32_t StrategyAgent::Torus(int32_t a, int32_t b) const {
  const int32_t dx = std::abs(board_.X(a) - board_.X(b)), dy = std::abs(board_.Y(a) - board_.Y(b));
  return std::min(dx, board_.width() - dx) + std::min(dy, board_.height() - dy);
}

double StrategyAgent::BedWeight(int32_t t) const {
  const TileMemory& m = board_.at(t);
  if (m.seen_round < 0) return 0.0;
  if (m.pearl) return 1.0;
  if (m.countdown < 0) return 0.0;
  const int32_t due = m.countdown - (round_ - m.seen_round);
  return due <= 30 ? 1.0 : 0.0;   // a slow bed (long countdown) is no cluster
}

double StrategyAgent::ClusterScore(int32_t t) const {
  double s = 0;
  const int32_t x = board_.X(t), y = board_.Y(t);
  for (int32_t ey = -2; ey <= 2; ++ey)
    for (int32_t ex = -2 + std::abs(ey); ex <= 2 - std::abs(ey); ++ex) s += BedWeight(board_.Tile(x + ex, y + ey));
  return s;
}

bool StrategyAgent::DenseFoodInView() const {
  for (int32_t ey = -kView; ey <= kView; ++ey)
    for (int32_t ex = -kView; ex <= kView; ++ex)
      if (ClusterScore(board_.Tile(board_.X(head_) + ex, board_.Y(head_) + ey)) >= kClusterMin - 1e-9) return true;
  return false;
}

// the same test as ChooseGoal's contest boost, for one tile
bool StrategyAgent::ContestWon(int32_t t, int32_t ready, int32_t* margin) const {
  const TileMemory& m = board_.at(t);
  const int32_t due = m.pearl ? 0 : std::max(0, m.countdown - (round_ - m.seen_round));
  int32_t theirs = 1 << 20;
  for (const auto& [id, w] : seen_)
    if (w.team != spawn_.team && w.head >= 0 && round_ - w.round <= kRivalMemory)
      theirs = std::min(theirs, BedReady(RivalDist(Torus(w.head, t), round_ - w.round), due, 0.0));
  if (margin) *margin = theirs - ready;
  return ready <= theirs && theirs <= kSchedHorizon;
}

void StrategyAgent::HeadsNear(int32_t at, int32_t r, int32_t* ours, int32_t* theirs) const {
  *ours = 1;   // ourselves
  *theirs = 0;
  for (const auto& [id, w] : seen_) {
    if (id == spawn_.id || w.round != round_ || w.head < 0 || Torus(w.head, at) > r) continue;
    if (w.team == spawn_.team) ++*ours;
    else ++*theirs;
  }
}

// ally heads we know now: in view this round, or reported in the last 2 rounds
std::vector<int32_t> StrategyAgent::AllyHeads(bool skip_queen) const {
  std::vector<int32_t> allies;
  for (const auto& [id, w] : seen_)
    if (id != spawn_.id && !(skip_queen && id <= 1) && w.team == spawn_.team && w.round == round_ && w.head >= 0) allies.push_back(w.head);
  for (const auto& [id, r] : reports_)
    if (id != spawn_.id && !(skip_queen && id <= 1) && r.tile >= 0 && round_ - r.round <= 2 && !(seen_.count(id) && seen_.at(id).round == round_))
      allies.push_back(r.tile);
  return allies;
}

void StrategyAgent::UpdateRush() {
  contest_ = false;
  if (IsQueen()) {
    rush_state_ = 3;
    return;
  }
  if (rush_state_ == 0) {
    rush_state_ = 3;
    const int32_t q = spawn_.id & 1;
    if (!FIELD_MODE && leader_ && round_ == 0 && (spawn_.id == q + 2 || spawn_.id == q + 4)) {
      rush_state_ = 1;
      rush_since_ = 0;
      // what the starters see at spawn is their own ground: the queen and the other starters
      // farm it, so a cluster in it is no find and doesn't end the exploration
      home_view_.assign(board_.size(), 0);
      for (int32_t t = 0; t < board_.size(); ++t) home_view_[t] = board_.at(t).seen_round >= 0;
    }
  }
  if (rush_state_ == 2 && round_ >= kMiddleUntil) rush_state_ = 3;
  if (round_ >= kMiddleUntil) return;
  // the contest: an explorer holding a contested find, while its head is within kContestRadius of it
  contest_ = rush_state_ == 2 && Torus(head_, rush_target_) <= kContestRadius;
  if (rush_state_ != 1) return;
  if (round_ >= kRushUntil) {
    rush_state_ = 3;
    return;
  }
  // a contested cluster, or a worthwhile one nobody of ours is at yet, seen: stop exploring
  const std::vector<int32_t> allies = AllyHeads();
  int32_t contested = -1, worthwhile = -1;
  for (int32_t t = 0; t < board_.size(); ++t) {
    if (BedWeight(t) <= 0) continue;
    for (const auto& [id, w] : seen_)
      if (w.team != spawn_.team && w.round == round_ && w.head >= 0 && Torus(w.head, t) <= kContestReach &&
          (contested < 0 || Torus(head_, t) < Torus(head_, contested)))
        contested = t;
    if (worthwhile >= 0 || home_view_[t] || ClusterScore(t) < kClusterMin - 1e-9) continue;
    // an ally head within kContestReach is already farming it
    bool farmed = false;
    for (const int32_t a : allies) farmed |= Torus(a, t) <= kContestReach;
    if (!farmed) worthwhile = t;
  }
  if (contested >= 0) {
    rush_state_ = 2;   // hold it to r60 (ChooseGoal halves goals far from it)
    rush_target_ = contested;
    return;
  }
  if (worthwhile >= 0) {
    rush_state_ = 3;   // the scheduler farms it
    return;
  }
  // no new tile seen for kRushStall rounds: the reachable frontier is used up
  int32_t seen = 0;
  for (int32_t t = 0; t < board_.size(); ++t) seen += board_.at(t).seen_round >= 0;
  if (seen > rush_seen_) {
    rush_seen_ = seen;
    rush_since_ = round_;
  } else if (round_ - rush_since_ >= kRushStall) {
    rush_state_ = 3;
  }
}

// food within kFoodDetour steps, else the frontier tile with the most unseen view per step
int32_t StrategyAgent::RushGoal(const Paths& paths) {
  int32_t food = -1;
  for (int32_t dy = -kFoodDetour; dy <= kFoodDetour; ++dy)
    for (int32_t dx = -kFoodDetour + std::abs(dy); dx <= kFoodDetour - std::abs(dy); ++dx) {
      const int32_t t = board_.Tile(board_.X(head_) + dx, board_.Y(head_) + dy);
      const int32_t d = paths.dist[t];
      if (d <= 0 || d > kFoodDetour || Banned(t)) continue;
      int32_t ready;
      if (TileValue(t, d, &ready) > 0 && ready <= d && (food < 0 || d < paths.dist[food])) food = t;
    }
  if (food >= 0) return food;
  // frontier nearer to an ally head is its share
  // g43-6: ... except our queen's: she never explores (ring or home), so for a founder, whose ally
  // is often only the queen beside it, her share left it no frontier (iteration 3, Colosseum seed
  // 11: the founder circled its corner for 18 rounds with RushGoal flipping between -1 and far tiles)
  const std::vector<int32_t> allies = AllyHeads(FOUNDER_MODE && founder_ == 1);
  int32_t best = -1;
  double best_score = 0;
  for (int32_t t = 0; t < board_.size(); ++t) {
    const int32_t d = paths.dist[t];
    if (d <= 0 || Banned(t) || Classify(t) != Cell::kFree) continue;
    bool frontier = false;
    for (int k = 0; k < 4 && !frontier; ++k) frontier = board_.Step(t, k) == kUnknown;
    if (!frontier) continue;
    bool theirs = false;
    for (const int32_t a : allies) {
      const int32_t da = Torus(a, t);
      if (da <= kSpread && da < d) {
        theirs = true;
        break;
      }
    }
    if (theirs) continue;
    int32_t gain = 0;
    const int32_t x = board_.X(t), y = board_.Y(t);
    for (int32_t ey = -kView; ey <= kView; ++ey)
      for (int32_t ex = -kView; ex <= kView; ++ex) gain += board_.at(board_.Tile(x + ex, y + ey)).seen_round < 0;
    if (gain == 0) continue;
    double score = gain / (d + 1.0);
    if (t == rush_target_) score *= kKeep;
    if (score > best_score) {
      best = t;
      best_score = score;
    }
  }
  rush_target_ = best;
  return best;
}

// a second free step along the shortest known path to the goal, when our length makes it free
// and the tile is in view, free, not exposed and roomy
bot::Action StrategyAgent::RushSprint(bot::Action a, int32_t goal) const {
  if (goal < 0 || (length_ + 3) / 4 < 2 || a.kind != bot::Action::kMove || a.resign || a.steps.size() != 1 ||
      std::strcmp(reason_, "toward goal") != 0)
    return a;
  const int32_t t1 = board_.Step(head_, a.steps[0]);
  if (t1 < 0 || t1 == goal) return a;
  // distances to the goal over free known tiles
  std::vector<int32_t> g(board_.size(), -1);
  std::vector<int32_t> queue{goal};
  g[goal] = 0;
  for (size_t qi = 0; qi < queue.size(); ++qi) {
    const int32_t t = queue[qi];
    if (t == t1 || g[t] >= 40) continue;
    for (int d = 0; d < 4; ++d) {
      const int32_t n = board_.Step(t, d);
      if (n < 0 || g[n] >= 0 || (n != t1 && Classify(n) != Cell::kFree)) continue;
      g[n] = g[t] + 1;
      queue.push_back(n);
    }
  }
  if (g[t1] < 2) return a;
  const int32_t need = RoomNeeded();
  auto open = [&](int32_t t) { return !Occupied(t) && !Exposed(t); };
  for (int d = 0; d < 4; ++d) {
    const int32_t t2 = board_.Step(t1, d);
    if (t2 < 0 || t2 == head_ || !board_.Current(t2) || Classify(t2) != Cell::kFree || g[t2] != g[t1] - 1) continue;
    if (Exposed(t2) || RoomAround(board_, t2, open, need) < need) continue;
    a.steps.push_back(d);
    return a;
  }
  return a;
}

// g43-5 (ring-sprint-1): a non-queen dragon of length 5+ walking to its goal takes the extra
// FREE steps its length gives (ceil(L/4): a second step from length 5, a third from 9), never a
// paid one. Each extra step must pass the single step's checks (SafeStep): a free tile we see
// now, not beside an enemy head we won't trade with, room >= RoomNeeded(), an escape (level >= 1)
// and 2 liberties for the body as it will be after the step; and it must stay on a shortest known
// path to the goal (the goal itself only when it is a free tile in view). Evidence: devtest1 (the
// server's #1) takes the free second step on 42-54% of eligible moves; ring-contest-1 on 2-5%.
bot::Action StrategyAgent::FreeSprint(bot::Action a, int32_t goal) const {
  const int free_steps = (length_ + 3) / 4;
  if (IsQueen() || goal < 0 || length_ < 5 || free_steps < 2 || a.kind != bot::Action::kMove || a.resign ||
      a.steps.size() != 1)
    return a;
  int32_t cur = board_.Step(head_, a.steps[0]);
  if (cur < 0 || cur == goal) return a;
  // v2: no sprint while an enemy head is in view this round. Sprints skipped the judge's late
  // one-step lookahead and walked long dragons into enemy reach faster: in v1 (fresh maps, 176
  // games) our 10+ dragons were lunged 9.8 times a match (160 length) v 6.8 (101) for the base.
  // Near an enemy the single-step logic (and its rollout review) decides as before.
  if (SPRINT_MODE >= 2)
    for (const auto& [id, w] : seen_)
      if (w.team != spawn_.team && w.round == round_ && w.head >= 0) return a;
  // distances to the goal over free known tiles (the first step's tile counts as passable)
  std::vector<int32_t> g(board_.size(), -1);
  std::vector<int32_t> queue{goal};
  g[goal] = 0;
  for (size_t qi = 0; qi < queue.size(); ++qi) {
    const int32_t t = queue[qi];
    if (t == cur || g[t] >= 40) continue;
    for (int d = 0; d < 4; ++d) {
      const int32_t n = board_.Step(t, d);
      if (n < 0 || g[n] >= 0 || (n != cur && n != head_ && Classify(n) != Cell::kFree)) continue;
      g[n] = g[t] + 1;
      queue.push_back(n);
    }
  }
  if (g[cur] < 1) return a;
  const bool goal_free = board_.Current(goal) && Classify(goal) == Cell::kFree;
  const int32_t need = RoomNeeded(), cap = EscapeCap(length_);
  auto open = [&](int32_t t) { return !Occupied(t) && !Exposed(t); };
  // the body after the first step (head first; a pearl eaten keeps the tail)
  std::vector<int32_t> body(body_.begin(), body_.end());
  auto advance = [&](int32_t to) {
    const bool pearl = board_.Current(to) && board_.at(to).pearl;
    body.insert(body.begin(), to);
    if (!pearl && !body.empty()) body.pop_back();
  };
  advance(cur);
  if (lunge_round_ != round_) BuildLungeField();
  for (int step = 1; step < free_steps && cur != goal; ++step) {
    int pick = -1;
    for (int d = 0; d < 4 && pick < 0; ++d) {
      const int32_t t = board_.Step(cur, d);
      if (t < 0 || !board_.Current(t) || g[t] < 0 || g[t] != g[cur] - 1) continue;
      if (t == goal && !goal_free) continue;
      if (Classify(t) != Cell::kFree || std::find(body.begin(), body.end(), t) != body.end()) continue;
      // g44-3: the lunge guard (g43-7) applies to each extra step as to the single step; g45-1 v2: each
      // extra step may be the move's last, so it is held to the end-tile reach (unknown lengths unbounded)
      if (Exposed(t) || LungeEndExposed(t) || RoomAround(board_, t, open, need) < need) continue;
      std::vector<int32_t> nb(body);
      nb.insert(nb.begin(), t);
      if (!(board_.Current(t) && board_.at(t).pearl)) nb.pop_back();
      if (Liberties(t, nb) < 2) continue;
      if (p_.escape_steps && EscapeLevel(nb, cap) < 1) continue;
      pick = d;
    }
    if (pick < 0) break;
    a.steps.push_back(pick);
    cur = board_.Step(cur, pick);
    advance(cur);
  }
  return a;
}

}  // namespace gm2
