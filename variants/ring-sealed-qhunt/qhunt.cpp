// g53-4 (ring-sealed-qhunt): hunt the enemy queen when she is winning the queen race
// (strategist B g53, proposal 1).
//
// Observed cause: arena-r27 has 134 "queen race lost, both queens alive" and 134 "queen killed in
// a live race" losses; server-r26 has 47 losses on the longer queen with both alive (Trauma's
// enemy queen grows 3 -> 32 unhunted). Short stubs kill big queens in this game (ours die to 3-12
// long stubs 61 times in S1a losses), yet we attack hers only when she happens to be in view
// (TryQueenAttack). Step 0 (NOTES.md): on local race games the trigger below holds in a
// sizeable share of the queen-decided losses.
//
//  - Trigger: a worker sees the enemy queen at r >= kHuntFrom with length >= max(kHuntMinLen, our
//    queen's known length + kHuntMargin), or sees her while our queen is known dead (an obituary).
//    kHuntFrom 380: the late window the race evidence covers (the queen grows from her ring exit at
//    430; 380 gives a pursuer far away time to arrive). kHuntMargin 3: within 3 the conveyor's
//    feeding of our queen can still close the gap (it keeps priority there). kHuntMinLen 8: a
//    queen shorter than that is no race lead worth a worker's food.
//  - Relay: the sighter sends her tile, round and length in a 0xBC word, in place of one of its four
//    pings, for kHuntSend rounds; a dragon that hears a newer word younger than kHuntSend re-pings it
//    once.
//  - Pursuit: a worker (never the queen) of length >= kHuntWorkerLen holding a word at most
//    kHuntFresh rounds old walks to her last tile if it lies within (500 - r) / 2 path steps (the
//    conveyor's reach rule), unless two ally heads in its view are nearer to it (at most 2 pursuers
//    per group). The step is marked as walking to an ally, so plugin.cpp keeps it; once she is in
//    view the existing TryQueenAttack takes the kill.
//  - Off: while our queen is known to be the longer one (re-checked by every pursuer), for the
//    queen herself, and when our queen is sealed (the sealed case is unchanged).
#include "strategy.hpp"

#include <algorithm>
#include <string>
#ifdef QHUNT_DBG
#include <cstdio>
#endif

namespace gm2 {
#ifdef QHUNT_DBG
#define QHR(n) std::fprintf(stderr, "QHREJ %d r%d id%d\n", n, round_, spawn_.id)
#else
#define QHR(n)
#endif

namespace {
constexpr uint64_t kHuntMagic = 0xBC;
constexpr int32_t kHuntFrom = 380, kHuntMinLen = 8, kHuntMargin = 3, kHuntSend = 3, kHuntFresh = 10, kHuntWorkerLen = 4;
}  // namespace

void StrategyAgent::OurQueenKnown(int32_t* len, bool* dead) const {
  *len = -1;
  *dead = false;
  if (queen_id_ < 0) return;
  if (const auto w = seen_.find(queen_id_); w != seen_.end() && w->second.team == spawn_.team) *len = std::max(*len, w->second.length);
  if (const auto r = reports_.find(queen_id_); r != reports_.end() && r->second.id >= 0) {
    if (r->second.length == 0) *dead = true;
    else *len = std::max(*len, r->second.length);
  }
}

uint64_t StrategyAgent::HuntWord() const {
  return (kHuntMagic << 56) | (uint64_t(spawn_.team & 1) << 55) | uint64_t(board_.X(hunt_tile_) & 63) |
         (uint64_t(board_.Y(hunt_tile_) & 63) << 6) | (uint64_t(hunt_round_ & 511) << 12) |
         (uint64_t(std::min(hunt_len_, 127)) << 21);
}

void StrategyAgent::UpdateHunt(const rl::Observation& o) {
  hunt_relay_ = false;
  for (const uint64_t w : o.inbox) {
    if ((w >> 56) != kHuntMagic || int32_t((w >> 55) & 1) != (spawn_.team & 1)) continue;
    const int32_t x = int32_t(w & 63), y = int32_t((w >> 6) & 63), r = int32_t((w >> 12) & 511), len = int32_t((w >> 21) & 127);
    if (x >= board_.width() || y >= board_.height() || r > round_ || r <= hunt_round_) continue;
    hunt_tile_ = board_.Tile(x, y);
    hunt_round_ = r;
    hunt_len_ = len;
    hunt_relay_ = round_ - r < kHuntSend;   // re-ping a young word once (we adopt each word once)
#ifdef QHUNT_DBG
    std::fprintf(stderr, "QHHEAR r%d team%d id%d len%d age%d\n", round_, spawn_.team, spawn_.id, length_, round_ - r);
#endif
  }
  if (round_ < kHuntFrom) return;
  for (int q = 0; q < 2; ++q) {
    const auto s = seen_.find(q);
    if (s == seen_.end() || s->second.team == spawn_.team || s->second.round != round_ || s->second.length <= 0 || s->second.head < 0) continue;
    int32_t ql;
    bool dead;
    OurQueenKnown(&ql, &dead);
    if (!dead && s->second.length < std::max(kHuntMinLen, ql + kHuntMargin)) continue;
    hunt_tile_ = s->second.head;
    hunt_round_ = round_;
    hunt_len_ = s->second.length;
    hunt_send_until_ = round_ + kHuntSend - 1;
#ifdef QHUNT_DBG
    std::fprintf(stderr, "QHTRIG r%d team%d id%d elen%d ql%d dead%d\n", round_, spawn_.team, spawn_.id, hunt_len_, ql, int(dead));
#endif
  }
}

// iteration 2: which of the four pings carries the word. Iteration 1 used round & 3, and in 24 local race
// games the 18 sightings' words reached no ally at all (a ping stops at the first dragon or kelp it
// meets, and next to the enemy queen that is mostly her team's bodies). So the word goes on a direction
// whose ray, as far as we see this turn, first meets an ally (the nearest such); else round & 3.
// A ping opposite the facing starts at the tail and runs the tail's way, so that direction is skipped.
int StrategyAgent::HuntDir(const bot::Action& a) const {
  int32_t start = head_;
  int facing = -1;
  if (body_.size() >= 2)
    for (int d = 0; d < 4; ++d)
      if (board_.Step(body_[1], d) == body_[0]) facing = d;
  if (a.kind == bot::Action::kMove && !a.resign)
    for (const int d : a.steps) {
      const int32_t n = board_.Step(start, d);
      if (n < 0) break;
      start = n;
      facing = d;
    }
  int best = round_ & 3, best_dist = 1 << 30;
  for (int d = 0; d < 4; ++d) {
    if (facing >= 0 && d == (facing + 2) % 4) continue;
    int32_t t = start;
    for (int k = 1; k <= board_.width() + board_.height(); ++k) {
      t = board_.Step(t, d);
      if (t < 0 || !board_.Current(t)) break;   // kelp, an unknown edge, or out of view: we can't tell
      const int32_t occ = board_.at(t).occupant;
      if (occ < 0) continue;
      if (occ != spawn_.id && board_.at(t).occupant_team == (spawn_.team & 1) && k < best_dist) best = d, best_dist = k;
      break;
    }
  }
  return best;
}

bool StrategyAgent::HuntPing() const {
  return hunt_round_ >= 0 && (hunt_send_until_ >= round_ || hunt_relay_);
}

bot::Action StrategyAgent::HuntMove() {
#ifdef QHUNT_DBG
  if (hunt_round_ >= kHuntFrom && round_ - hunt_round_ <= kHuntFresh) std::fprintf(stderr, "QHM r%d team%d id%d len%d sealed%d hage%d edead%d\n", round_, spawn_.team, spawn_.id, length_, int(queen_sealed_), round_ - hunt_round_, enemy_queen_dead_);
#endif
  if (IsQueen() || queen_sealed_ || round_ >= 499 || length_ < kHuntWorkerLen) return {};
  if (hunt_round_ < kHuntFrom || round_ - hunt_round_ > kHuntFresh || enemy_queen_dead_ >= hunt_round_) return {};
  int32_t ql;
  bool dead;
  OurQueenKnown(&ql, &dead);
  if (!dead && hunt_len_ < std::max(kHuntMinLen, ql + kHuntMargin)) { QHR(105); return {}; }   // ours is (now) the longer one
  int32_t target = hunt_tile_;
  bool in_view = false;
  for (int q = 0; q < 2; ++q)
    if (const auto s = seen_.find(q); s != seen_.end() && s->second.team != spawn_.team && s->second.round == round_ && s->second.head >= 0) {
      target = s->second.head;
      in_view = true;
    }
  // her last tile is in view and she isn't there: the word is stale and we don't know where she went
  if (!in_view && board_.Current(target)) {
    hunt_round_ = -1000;
    { QHR(116); return {}; }
  }
  const int32_t budget = (500 - round_) / 2;
  const int32_t mine = Torus(head_, target);
  if (budget < 1 || mine > budget + 1) { QHR(120); return {}; }
  // at most two pursuers per group: yield to two ally heads in view nearer to her
  int32_t nearer = 0;
  for (const auto& [id, w] : seen_)
    if (w.team == spawn_.team && w.round == round_ && w.head >= 0 && id > 1 && id != spawn_.id) {
      const int32_t d = Torus(w.head, target);
      if (d < mine || (d == mine && id < spawn_.id)) ++nearer;
    }
  if (nearer >= 2) { QHR(128); return {}; }
  const Paths paths = Search(board_, head_, [&](int32_t t) {
    return (RING5_MODE != 3 || !Ring5Reserved(t)) && !Occupied(t);
  }, budget + 1);
  const int32_t goal = paths.Nearest([&](int32_t t) { return t == target || Torus(t, target) <= 1; });
  if (goal < 0 || paths.dist[goal] <= 0 || paths.dist[goal] > budget) { QHR(133); return {}; }
  cur_goal_ = goal;
  bot::Action a = SafeStep(paths.first[goal], false);
  cur_goal_ = -1;
  if (std::string(reason_) != "toward goal") { QHR(137); return {}; }   // no safe step toward her: carry on as usual
  walking_to_ally_ = true;   // plugin.cpp keeps this step (not the late lookahead's)
  reason_ = "qhunt: pursue queen";
#ifdef QHUNT_DBG
  std::fprintf(stderr, "QHUNT r%d team%d id%d len%d dist%d hlen%d hage%d ql%d dead%d view%d\n", round_, spawn_.team, spawn_.id, length_,
               paths.dist[goal], hunt_len_, round_ - hunt_round_, ql, int(dead), int(in_view));
#endif
  return a;
}

}  // namespace gm2
