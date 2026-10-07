// g46-6 (spar-field-1): a SPARRING bot for the arena pool that opens like the top server swarm
// teams (docs/analysis-2026-10-07/STYLE.md, aAah; strategist-g46-A S3). The arena pool is all our
// own home-bound lineage, so opening changes can't be judged against an opponent that rushes.
//
// Until a dragon "arrives" (or kSparUntil), this module owns its turn:
//  1. Atomise: a dragon of length 4+ splits off a 2-long child (STYLE.md: aAah is "n6 L2" at r6).
//  2. Committed heading, one dragon per lane: each dragon takes a heading on its first turn, away
//     from the ally heads it knows of (else its facing), and keeps it; ally heads within kLane push
//     it sideways (an even carpet with gaps, not a blob). It walks to the reachable frontier most
//     along the heading per step (portals included: the search crosses known portals, so a tile past one
//     counts by where it really is); frontier nearer another ally head is its lane. The heading
//     slowly follows the way it actually goes (a room door, a corridor).
//  3. Food on the way: a pearl within kFoodDetour steps that is not behind us (the queen: kQueenFood,
//     any direction). Seen beds are travel targets too (kFieldPull each within 2).
//  4. Arrival: a bed cluster in view (kArrive live or soon-due beds within 2 of a tile within 3 of
//     the head) with no more of our heads around it than beds: the dragon stays and the
//     base farms it (it splits at 4, so its children carpet the field; a child born beside a
//     crowded field walks on).
//  5. On a 3+-dragon start the queen splits to 2 as well and walks towards the middle of the ally heads she knows
//     (inside the swarm, behind its tips), taking food within reach; with no ally known she walks
//     as a worker would. From kSparUntil she is the base's queen, her home where she stands.
//  An enemy head whose lunge could reach our next step (at least kFight; the queen kQueenMargin
//  more) hands that turn to the base (its trade, contest, lunge and queen rules).
// Observation only: no map keys, no board-size gates, no fixed board points.
#include <algorithm>
#include <cmath>

#include "strategy.hpp"

namespace gm2 {
using namespace strat;

namespace {
constexpr int32_t kSparUntil = 40;    // STYLE.md: aAah's swarm is in the cup and fields by r18-r24
constexpr int32_t kSearch = 40;       // path-search depth for the travel target
constexpr int32_t kFight = 3;         // an enemy head this near: the base's fight rules
constexpr int32_t kPartialLen = 6;    // an enemy whose body leaves the view: at least this long
constexpr int32_t kSmallStart = 2;    // the queen's r0 unit count at or below which she keeps the base's forage
constexpr int32_t kQueenMargin = 2;   // the queen's extra distance from any lunge reach
constexpr int32_t kLane = 4;          // ally heads this near push the heading sideways
constexpr double kPush = 0.4;         // how hard (per 1/d^2), sideways only
constexpr double kAlign = 3.0;        // a frontier straight ahead v one straight behind: 6 steps
constexpr double kStep = 0.5;         // per step of path
constexpr double kTheirs = 2.0;       // frontier nearer to another ally head
constexpr double kFollow = 0.25;      // the heading turns this much a turn towards the way we go
constexpr int32_t kFoodDetour = 2;    // food within this many steps, not behind us
constexpr int32_t kQueenFood = 3;     // the queen's food reach, any direction: on 1-2-starter maps she is a
                                      // third to half of the opening workforce (strategist-g46-A)
constexpr double kArrive = 3.0;       // live or soon-due beds within 2 of a tile: a field
constexpr double kFieldPull = 1.0;    // a seen field ahead pulls the travel target
constexpr int32_t kQueenSplitMin = 4; // the queen keeps 2 (STYLE.md: 2-4 long all game)
}  // namespace

bool StrategyAgent::SparTurn(const rl::Observation& o, bot::Action* out) {
  if (spar_done_) return false;
  if (round_ >= kSparUntil) {
    spar_done_ = true;
    return false;
  }
  // a queen that starts with at most one other dragon keeps the base's opening forage (she eats and
  // splits at 4 at home: ring-plus-1's forage, real on 2-starter server maps; here, fresh off-axis
  // 1-2-starter maps went 2-14 when she walked out to the swarm instead)
  if (IsQueen() && round_ == 0 && units_ <= kSmallStart) spar_done_ = true;
  if (spar_done_) return false;
  const int32_t W = board_.width(), H = board_.height();
  auto disp = [&](int32_t from, int32_t to, double* dx, double* dy) {
    int32_t x = board_.X(to) - board_.X(from), y = board_.Y(to) - board_.Y(from);
    if (x > W / 2) x -= W;
    if (x < -W / 2) x += W;
    if (y > H / 2) y -= H;
    if (y < -H / 2) y += H;
    *dx = x;
    *dy = y;
  };
  // an enemy head in view: the base's fight rules decide this turn
  // (by path: a head across a kelp wall is no fight)
  const Paths paths = Search(board_, head_, [&](int32_t t) { return Classify(t) == Cell::kFree; }, kSearch);
  // An enemy of length Le puts its head on any tile within ceil(Le/4)+Le-2 steps in one lunge (the
  // rules' sprint cost, as the base's lunge field); a body leaving the view counts as long. Within
  // that reach (at least kFight) of our next step the base's fight rules decide; the queen keeps
  // kQueenMargin more (sim, fresh sparse maps: walking past lunge reach, our 2-long queen died in
  // 10 of 16 games and our L2 workers were lunged one by one).
  for (const auto& [id, w] : seen_) {
    if (w.team == spawn_.team || w.round != round_ || w.head < 0) continue;
    const int32_t le = w.partial ? std::max<int32_t>(w.length, kPartialLen) : w.length;
    const int32_t reach = std::max(kFight - 1, (le + 3) / 4 + le - 2) + (IsQueen() ? kQueenMargin : 0);
    if (Torus(w.head, head_) > reach + 2) continue;
    for (int k = 0; k < 4; ++k) {
      const int32_t n = board_.Step(w.head, k);
      if (n >= 0 && (n == head_ || (paths.dist[n] >= 0 && paths.dist[n] <= reach))) return false;
    }
  }

  const std::vector<int32_t> allies = AllyHeads(false);
  // the heading: on the first turn away from the allies we know of (else our facing)
  static constexpr double fx[4] = {0, 1, 0, -1}, fy[4] = {-1, 0, 1, 0};
  auto push = [&](int32_t radius, double* rx, double* ry) {
    *rx = *ry = 0;
    for (const int32_t a : allies) {
      if (a == head_ || Torus(a, head_) > radius) continue;
      double dx, dy;
      disp(a, head_, &dx, &dy);
      const double d2 = dx * dx + dy * dy;
      if (d2 > 0) {
        *rx += dx / d2;
        *ry += dy / d2;
      }
    }
  };
  if (!spar_init_) {
    spar_init_ = true;
    double hx = fx[o.facing & 3], hy = fy[o.facing & 3], rx, ry;
    push(6, &rx, &ry);
    const double rn = std::hypot(rx, ry);
    if (rn > 1e-9) {
      hx = 0.5 * hx + rx / rn;
      hy = 0.5 * hy + ry / rn;
    }
    const double n = std::hypot(hx, hy);
    if (n < 1e-9) {
      hx = fx[o.facing & 3];
      hy = fy[o.facing & 3];
    } else {
      hx /= n;
      hy /= n;
    }
    // a split child (its tail touches its parent's new tail on its first turn, docs/game.md SPLIT)
    // faces back the way its parent came; it takes its parent's way instead, turned 45 degrees to
    // one side (by id parity), so a line of splits fans out rather than doubling back home
    if (round_ == born_ && !body_.empty()) {
      const int32_t tail = body_.back();
      int32_t parent = -1;
      for (const rl::Segment& sg : o.segments)
        if (sg.team == spawn_.team && sg.id != spawn_.id)
          for (int k = 0; k < 4 && parent < 0; ++k)
            if (board_.Step(tail, k) == board_.Tile(sg.x, sg.y)) parent = sg.id;
      int32_t phead = -1;
      if (parent >= 0)
        for (const rl::Segment& sg : o.segments)
          if (sg.id == parent && sg.head) phead = board_.Tile(sg.x, sg.y);
      double dx, dy;
      if (phead >= 0 && phead != head_) {
        disp(head_, phead, &dx, &dy);
        const double r = std::hypot(dx, dy), c = std::sqrt(0.5), sgn = (spawn_.id & 2) ? 1.0 : -1.0;
        const double ux = dx / r, uy = dy / r;
        hx = c * (ux - sgn * uy);
        hy = c * (uy + sgn * ux);
      }
    }
    spar_hx_ = hx;
    spar_hy_ = hy;
  } else {
    double rx, ry;
    push(kLane, &rx, &ry);
    const double along = rx * spar_hx_ + ry * spar_hy_;   // only the sideways part: lanes, not reversals
    rx -= along * spar_hx_;
    ry -= along * spar_hy_;
    const double hx = spar_hx_ + kPush * rx, hy = spar_hy_ + kPush * ry, n = std::hypot(hx, hy);
    if (n > 1e-9) {
      spar_hx_ = hx / n;
      spar_hy_ = hy / n;
    }
  }

  // arrival: a field in view that our heads don't already crowd
  if (!IsQueen()) {
    const int32_t hx = board_.X(head_), hy = board_.Y(head_);
    for (int32_t ey = -3; ey <= 3; ++ey)
      for (int32_t ex = -3; ex <= 3; ++ex) {
        const int32_t t = board_.Tile(hx + ex, hy + ey);
        const double c = ClusterScore(t);
        if (c < kArrive - 1e-9) continue;
        int32_t ours = 1;
        for (const int32_t a : allies) ours += Torus(a, t) <= 3;
        if (ours <= c + 1e-9) {   // one head per live bed: room for us (STYLE.md: singles, evenly spaced)
          spar_done_ = true;
          return false;
        }
      }
  }

  // atomise
  const int32_t split_min = IsQueen() ? kQueenSplitMin : 4;
  if (length_ >= split_min && FreeSlots() > 1 && SplitSafe(2)) {
    *out = Because(IsQueen() ? "spar: queen atomise" : "spar: atomise", bot::Action::Split(2));
    return true;
  }

  avoid_tile_ = -1;
  auto step_to = [&](int32_t target, const char* why) {
    cur_goal_ = target;
    bot::Action a = SafeStep(paths.first[target], false);
    if (a.kind == bot::Action::kMove && !a.resign && a.steps.size() == 1 && a.steps[0] == paths.first[target]) reason_ = why;
    *out = a;
    return true;
  };
  // food on the way
  {
    int32_t food = -1;
    const int32_t reach = IsQueen() ? kQueenFood : kFoodDetour;
    for (int32_t dy = -reach; dy <= reach; ++dy)
      for (int32_t dx = -reach + std::abs(dy); dx <= reach - std::abs(dy); ++dx) {
        const int32_t t = board_.Tile(board_.X(head_) + dx, board_.Y(head_) + dy);
        const int32_t d = paths.dist[t];
        if (d <= 0 || d > reach || Exposed(t)) continue;
        int32_t ready;
        if (TileValue(t, d, &ready) <= 0 || ready > d) continue;
        double ddx, ddy;
        disp(head_, t, &ddx, &ddy);
        if (!IsQueen() && d > kFoodDetour && ddx * spar_hx_ + ddy * spar_hy_ < 0) continue;   // behind us beyond the detour: not on the way
        if (food < 0 || d < paths.dist[food]) food = t;
      }
    if (food >= 0) return step_to(food, "spar: food on the way");
  }

  // the queen: towards the middle of the ally heads she knows, inside the swarm
  if (IsQueen() && !allies.empty()) {
    double cx = 0, cy = 0;
    for (const int32_t a : allies) {
      double dx, dy;
      disp(head_, a, &dx, &dy);
      cx += dx;
      cy += dy;
    }
    cx /= allies.size();
    cy /= allies.size();
    int32_t best = -1;
    double best_s = std::hypot(cx, cy);   // staying put scores the current distance
    for (int32_t t = 0; t < board_.size(); ++t) {
      const int32_t d = paths.dist[t];
      if (d <= 0 || d > 6) continue;
      double dx, dy;
      disp(head_, t, &dx, &dy);
      const double s = std::hypot(cx - dx, cy - dy) + 0.05 * d;
      if (s < best_s - 0.5) {
        best_s = s;
        best = t;
      }
    }
    if (best >= 0) return step_to(best, "spar: queen with the swarm");
    *out = Because("spar: queen in the swarm", SafeStep(-1, false));
    return true;
  }

  // travel: the reachable frontier (a known free tile beside an edge never seen) or seen field
  // that lies most along the heading, per step of path to it; frontier an ally head is nearer
  // to is that ally's lane. In a closed room the only frontier is its door, whatever the heading.
  int32_t target = -1;
  double best_s = -1e9;
  for (int32_t t = 0; t < board_.size(); ++t) {
    const int32_t d = paths.dist[t];
    if (d <= 0) continue;
    bool frontier = false;
    for (int k = 0; k < 4 && !frontier; ++k) frontier = board_.Step(t, k) == kUnknown;
    const double field = ClusterScore(t);
    if (!frontier && field < 1.0 - 1e-9) continue;   // the frontier, or a live or soon-due bed seen
    double dx, dy;
    disp(head_, t, &dx, &dy);
    const double r = std::hypot(dx, dy);
    const double align = r > 1e-9 ? (dx * spar_hx_ + dy * spar_hy_) / r : 0.0;
    bool theirs = false;
    if (!IsQueen())
      for (const int32_t a : allies)
        if (a != head_ && Torus(a, t) < d && Torus(a, t) <= kLane + 2) theirs = true;
    const double s = kAlign * align - kStep * d + kFieldPull * field - (theirs ? kTheirs : 0.0);
    if (s > best_s) {
      best_s = s;
      target = t;
    }
  }
  if (target < 0) {
    spar_done_ = true;   // nowhere new to go: the base takes over
    return false;
  }
  // the heading follows the way we actually go, slowly (a corridor bends it)
  {
    double dx, dy;
    disp(head_, target, &dx, &dy);
    const double r = std::hypot(dx, dy);
    if (r > 1e-9) {
      const double hx = spar_hx_ + kFollow * dx / r, hy = spar_hy_ + kFollow * dy / r, n = std::hypot(hx, hy);
      if (n > 1e-9) {
        spar_hx_ = hx / n;
        spar_hy_ = hy / n;
      }
    }
  }
  return step_to(target, IsQueen() ? "spar: queen travel" : "spar: travel");
}

}  // namespace gm2
