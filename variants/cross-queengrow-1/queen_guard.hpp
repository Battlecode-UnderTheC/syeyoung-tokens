// gst-queen-guard-1's queen guard (giant team). Under unswbc 1.2.3 the longer queen wins at r500
// and a dead queen counts 0, so our queen (dragon id 0 or 1) must live. gst-queen-1 decides as
// usual; for the queen, this guard reviews its action every turn:
//   * forbidden: resign, feed, give up a slot, lunges and head trades, and any split that would
//     leave the queen shorter than 4 (unless it is trapped);
//   * candidates: every free single step, gst's own allowed action, and an economy SPLIT 2 that
//     keeps the queen at 4+ with room;
//   * each candidate's value is the rollout outcome on the forward model (arena-giant-r7 /
//     gst-giant-2b's judge, here with 1.2.3 sprint rules, enemies that trade any dragon into a
//     queen, and a queen that never trades), with a heavy loss for a dead queen; minus the head
//     risk of the tile (an enemy head that can reach it this turn under 1.2.3: ceil(L/4)+L-2
//     steps), a crowd term (enemy heads near the tile) and a room shortfall (dead ends);
//   * a queen that is doomed at spawn (its room smaller than its length) is left to plain gst.
#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <deque>
#include <functional>
#include <random>
#include <vector>

#include "agent.hpp"
#include "board_memory.hpp"

namespace qg {


constexpr int kInf = 1 << 20;
constexpr int kHorizon = 8;          // rounds per rollout
constexpr int kRollouts = 16;        // rollouts per candidate action
constexpr double kEps = 0.15;        // random safe step in the policy
constexpr double kTrade = 0.5;       // probability a head trades into an adjacent opposing head
constexpr bool kLungePolicy = true;  // rollout dragons also lunge (2-4 steps) onto opposing heads
constexpr double kLunge = 0.5;       // ... with this probability when one is in reach
constexpr int kModelRadius = 14;     // pearls and beds within this distance go into the model
constexpr int kSplitUntil = 360;     // our dragons split in rollouts until this round

// ---------------------------------------------------------------- the forward model
constexpr int kMemoryRounds = 4;     // enemy heads seen this recently still count as threats
constexpr double kRoomOk = 0.01;     // a room shortfall below this counts as enough room
constexpr int kMaxSprint = 3;        // the queen's sprint candidates go this many free steps at most
constexpr int kQueenMinLen = 3;      // an economy split must leave the queen at least this long
// (qg9-1) ... but only from this round: before it, splits may leave the queen at 2 (as in
// gst-queen-guard-2). The early splits compound into the team's economy, and early queens are safe
// (alive at r100 in 86% of games either way); the queen's deaths that minimum 3 prevents come later.
constexpr int kQueenMinLenFrom = 100;
constexpr int kQueenMinLenEarly = 2;
constexpr int kGrowFrom = 430;       // (cross-queengrow-1) the queen stops economy splits from here
constexpr double kQueenDeath = 30.0; // a dead queen in a rollout costs this much length
constexpr double kRiskW = 4.0;       // per point of head risk on the tile (adjacent enemy head: 3)
constexpr double kCrowdW = 0.5;      // per unit of enemy-head closeness: (kCrowdRadius - distance)^2
constexpr int kCrowdRadius = 7;
constexpr double kRoomW = 6.0;       // times the room shortfall (0..1)
constexpr int kRoomCap = 24;         // the room the queen wants: min(L + 2, this)
constexpr double kPrefer = 0.5;      // gst's own allowed choice keeps ties

struct SimDragon {
  std::vector<int> body;   // head first
  int team = 0;
  bool alive = true, moves = true;
  bool queen = false;      // ours: never trades or lunges in the rollouts
};

struct Bed {
  int tile, due;   // due: sim round the next pearl appears (hot beds: every round from due)
  bool hot;
};

class Sim {
 public:
  const std::vector<int>* nb = nullptr;   // [tile*4+dir] -> tile, -1 kelp
  int W = 0, H = 0;
  int my_team = 0;
  std::vector<int16_t> occ;
  std::vector<uint8_t> pearl;
  std::vector<int> pearls;                // tiles that ever held a pearl in this model
  std::vector<SimDragon> d;
  std::vector<Bed> beds;

  int Dist(int a, int b) const {
    int dx = std::abs(a % W - b % W), dy = std::abs(a / W - b / W);
    return std::min(dx, W - dx) + std::min(dy, H - dy);
  }
  void Kill(int i) {
    SimDragon& g = d[i];
    for (size_t s = 0; s < g.body.size(); ++s) {
      if (s % 2 == 0) AddPearl(g.body[s]);
      occ[g.body[s]] = -1;
    }
    g.alive = false;
    g.body.clear();
  }
  void AddPearl(int t) {
    if (!pearl[t]) {
      pearl[t] = 1;
      pearls.push_back(t);
    }
  }
  // one step of dragon i (engine: Game::Step); pay = a later step of a multi-step move
  bool Step(int i, int dir, bool pay) {
    SimDragon& g = d[i];
    if (pay && g.body.size() <= 2) { Kill(i); return false; }
    const int dest = (*nb)[g.body[0] * 4 + dir];
    if (dest < 0) { Kill(i); return false; }
    const int o = occ[dest];
    if (o == i) { Kill(i); return false; }
    if (o >= 0) {
      if (d[o].body[0] == dest) Kill(o);
      Kill(i);
      return false;
    }
    g.body.insert(g.body.begin(), dest);
    occ[dest] = int16_t(i);
    if (pearl[dest]) pearl[dest] = 0;
    else { occ[g.body.back()] = -1; g.body.pop_back(); }
    if (pay) { occ[g.body.back()] = -1; g.body.pop_back(); }
    return true;
  }
  int Split(int i, int n) {
    SimDragon& g = d[i];
    const int L = int(g.body.size());
    n = std::min(n, L - 2);
    if (n < 2) return -1;   // the model's body is shorter than the real one
    SimDragon c;
    c.team = g.team;
    for (int k = 0; k < n; ++k) c.body.push_back(g.body[L - 1 - k]);
    g.body.resize(L - n);
    d.push_back(c);
    const int ci = int(d.size()) - 1;
    for (int t : d[ci].body) occ[t] = int16_t(ci);
    return ci;
  }
  void Tick(int t) {   // beds at the start of sim round t
    for (Bed& b : beds)
      if ((b.hot && t >= b.due) || t == b.due)
        if (!pearl[b.tile] && occ[b.tile] < 0) AddPearl(b.tile);
  }
  // the rollout policy for dragon i
  void Policy(int i, std::mt19937& rng, bool may_split) {
    SimDragon& g = d[i];
    if (!g.alive || !g.moves) return;
    const int L = int(g.body.size());
    std::uniform_real_distribution<double> U(0, 1);
    if (may_split && L >= 4 && U(rng) < 0.8) { Split(i, 2); return; }
    const int head = g.body[0];
    int safe[4], ns = 0, trade = -1;
    for (int k = 0; k < 4; ++k) {
      const int n = (*nb)[head * 4 + k];
      if (n < 0) continue;
      const int o = occ[n];
      if (o < 0) safe[ns++] = k;
      else if (d[o].team != g.team && d[o].body[0] == n) trade = k;
    }
    if (trade >= 0 && !g.queen && U(rng) < kTrade) { Step(i, trade, false); return; }
    if (kLungePolicy && !g.queen && L >= 3 && U(rng) < kLunge) {   // a multi-step lunge onto an opposing head at least as long
      int steps[8];
      const int n = LungePath(i, steps);
      if (n > 0) {
        const int free = (L + 3) / 4;
        for (int k = 0; k < n; ++k)
          if (!Step(i, steps[k], k >= free)) break;
        return;
      }
    }
    if (ns == 0) { Kill(i); return; }
    if (U(rng) < kEps) { Step(i, safe[rng() % ns], false); return; }
    // toward the nearest pearl (torus manhattan), else keep going
    int best = -1, bd = kInf;
    for (int p : pearls)
      if (pearl[p]) {
        const int dd = Dist(head, p);
        if (dd < bd) { bd = dd; best = p; }
      }
    int pick = safe[rng() % ns], pd = kInf;
    if (best >= 0)
      for (int s = 0; s < ns; ++s) {
        const int n = (*nb)[head * 4 + safe[s]];
        const int dd = Dist(n, best) * 4 + int(rng() % 3);
        if (dd < pd) { pd = dd; pick = safe[s]; }
      }
    Step(i, pick, false);
  }
  // shortest free path (2..min(L-1, 4) steps) from dragon i's head onto an opposing head at least
  // as long; returns its length in steps (0: none)
  int LungePath(int i, int* out) const {
    const SimDragon& g = d[i];
    const int L = int(g.body.size()), reach = std::min((L + 3) / 4 + L - 2, 5);   // 1.2.3 reach
    struct Node { int tile, parent, dir, depth; };
    Node q[96];
    int qn = 0, qi = 0;
    q[qn++] = {g.body[0], -1, -1, 0};
    while (qi < qn) {
      const Node cur = q[qi++];
      if (cur.depth >= reach) continue;
      for (int k = 0; k < 4; ++k) {
        const int n = (*nb)[cur.tile * 4 + k];
        if (n < 0) continue;
        const int o = occ[n];
        if (o >= 0) {
          if (cur.depth >= 1 && d[o].team != g.team && d[o].body[0] == n && (d[o].queen || d[o].body.size() >= g.body.size())) {   // a queen is worth any trade
            int len = cur.depth + 1, at = qi - 1;
            out[len - 1] = k;
            for (int j = len - 2; j >= 0; --j) { out[j] = q[at].dir; at = q[at].parent; }
            return len;
          }
          continue;
        }
        bool seen = false;
        for (int j = 0; j < qn; ++j) if (q[j].tile == n) { seen = true; break; }
        if (seen || qn >= 96) continue;
        q[qn++] = {n, qi - 1, k, cur.depth + 1};
      }
    }
    return 0;
  }
  double Material(int me, double w_me) const {
    double m = 0;
    for (size_t i = 0; i < d.size(); ++i) {
      if (!d[i].alive) continue;
      const double l = double(d[i].body.size()) * (int(i) == me ? w_me : 1.0);
      m += d[i].team == my_team ? l : -l;
    }
    return m;
  }
};



class QueenGuard {
 public:
  explicit QueenGuard(const rl::Spawn& s)
      : sp_(s), mem_(s.width, s.height), rng_(uint32_t(s.id * 7919 + s.team * 104729 + 3)) {}

  bool doomed() const { return doomed_; }

  void Observe(const rl::Observation& o) {
    mem_.Update(o);
    const int N = mem_.size();
    W_ = mem_.width();
    H_ = mem_.height();
    round_ = o.round;
    head_ = mem_.Tile(o.head_x(), o.head_y());
    L_ = o.length;
    if (hot_.empty()) { hot_.assign(N, 0); low_seen_.assign(N, -100); nb_.assign(N * 4, -1); }
    for (int k = 0; k < rl::kWindowTiles; ++k)
      if (o.countdown[k] >= 0 && o.countdown[k] <= 1) {
        const int t = mem_.Tile(o.tile_x[k], o.tile_y[k]);
        if (o.round - low_seen_[t] >= 1 && o.round - low_seen_[t] <= 10) hot_[t] = 1;
        low_seen_[t] = o.round;
      }
    TrackBody(o);
    // enemy heads seen lately (they keep threatening from outside the window)
    {
      std::vector<Sighting> keep;
      for (const Sighting& g : sightings_)
        if (round_ - g.round <= kMemoryRounds) keep.push_back(g);
      for (const rl::Segment& s : o.segments)
        if (s.head && s.team != sp_.team) {
          int len = 0;
          for (const rl::Segment& q : o.segments) len += q.id == s.id;
          bool found = false;
          for (Sighting& g : keep)
            if (g.id == s.id) { g = {s.id, mem_.Tile(s.x, s.y), round_, len}; found = true; }
          if (!found) keep.push_back({s.id, mem_.Tile(s.x, s.y), round_, len});
        }
      sightings_.swap(keep);
    }
    if (first_) {   // doomed at spawn: the room the head can reach is smaller than the body
      first_ = false;
      doomed_ = SpawnRoom(o) < L_;
    }
  }

  // the queen's action: gst's, unless it is forbidden or a candidate's rollouts beat it
  int MinLen() const { return round_ < kQueenMinLenFrom ? kQueenMinLenEarly : kQueenMinLen; }
  bot::Action Guard(const rl::Observation& o, const bot::Action& gst, bool* changed) {
    *changed = false;
    if (doomed_) return gst;
    const int N = mem_.size();
    for (int t = 0; t < N; ++t)
      for (int k = 0; k < 4; ++k) {
        int n = mem_.Step(t, k);
        if (n == strat::kUnknown) {
          const int g = mem_.Tile(mem_.X(t) + dragons::kDx[k], mem_.Y(t) + dragons::kDy[k]);
          n = mem_.at(t).seen_round >= 0 && mem_.at(g).seen_round >= 0 ? -1 : g;
        }
        nb_[t * 4 + k] = n;
      }
    BuildModel(o);
    base_.d[0].queen = true;
    std::vector<QCand> cands;
    const bool can_split = o.unit_count < sp_.unit_limit;
    int free_steps = 0;
    for (int d = 0; d < 4; ++d) {
      const int n = mem_.Step(head_, d);   // only seen edges: a blind step is not worth the queen
      if (n < 0 || base_.occ[n] >= 0) continue;
      ++free_steps;
      cands.push_back({bot::Action::Move(d), n, 0, gst.kind == bot::Action::kMove && !gst.resign && gst.steps.size() == 1 && gst.steps[0] == d, 0});
    }
    const bool trapped = free_steps == 0;
    // sprints: under 1.2.3 the first ceil(L/4) steps of a move are free, so a queen of length 5+
    // outruns every enemy of length 4 or less. Every simple path of 2..min(free, kMaxSprint) free,
    // seen tiles is a candidate.
    const int sprint = std::min((L_ + 3) / 4, kMaxSprint);
    if (sprint >= 2) {
      std::vector<int> path, tiles{head_};
      std::function<void(int)> grow = [&](int depth) {
        if (depth >= 2) {
          bot::Action a = bot::Action::Move(path[0]);
          a.steps = path;
          const bool mine = gst.kind == bot::Action::kMove && !gst.resign && gst.steps == path;
          cands.push_back({a, tiles.back(), 0, mine, 0});
        }
        if (depth == sprint) return;
        for (int d = 0; d < 4; ++d) {
          const int n = mem_.Step(tiles.back(), d);
          if (n < 0 || base_.occ[n] >= 0 || std::find(tiles.begin(), tiles.end(), n) != tiles.end()) continue;
          path.push_back(d);
          tiles.push_back(n);
          grow(depth + 1);
          path.pop_back();
          tiles.pop_back();
        }
      };
      grow(0);
    }
    // (cross-queengrow-1) from kGrowFrom the queen grows: no economy splits, only trapped ones
    const bool grow = round_ >= kGrowFrom;
    if (!grow && can_split && L_ - 2 >= MinLen() && static_cast<int>(body_.size()) == L_) {
      const bool mine = gst.kind == bot::Action::kSplit && gst.split == 2;
      cands.push_back({bot::Action::Split(2), -1, 0, mine, 0});
    }
    if (gst.kind == bot::Action::kSplit && gst.split != 2 && can_split && ((!grow && L_ - gst.split >= MinLen()) || trapped))
      cands.push_back({gst, -1, 0, true, 0});
    if (cands.empty()) return gst;   // trapped: nothing saves the head (a split keeps the queen as the stub)
    const uint32_t seed = rng_();
    double best_v = -1e18;
    size_t best = 0;
    for (size_t i = 0; i < cands.size(); ++i) {
      QCand& c = cands[i];
      rng_.seed(seed);
      double sum = 0;
      for (int r = 0; r < kRollouts; ++r) sum += Rollout(c.act);
      c.value = sum / kRollouts;
      c.risk = c.tile >= 0 ? HeadRisk(o, c.tile) + MemoryRisk(c.tile) : HeadRisk(o, head_) + MemoryRisk(head_);
      c.room = c.tile >= 0 ? RoomShort(c.tile) : 0.0;
      if (c.tile >= 0) c.value -= kRiskW * c.risk + kCrowdW * Crowd(o, c.tile) + kRoomW * c.room;
      if (c.gst) c.value += kPrefer;
    }
    // risk first: a candidate no enemy head can reach beats any that one can
    for (size_t i = 0; i < cands.size(); ++i) {
      const QCand& c = cands[i];
      const QCand& b = cands[best];
      // then room: a dead end traps the queen for good (a split leaves the queen as the stub)
      const bool c_ok = c.room < kRoomOk, b_ok = b.room < kRoomOk;
      if (i == 0 || c.risk < b.risk || (c.risk == b.risk && (c_ok && !b_ok)) ||
          (c.risk == b.risk && c_ok == b_ok && c.value > b.value)) { best = i; best_v = c.value; }
    }
    rng_.seed(seed + 1);
    if (cands[best].gst) return gst;
    *changed = true;
    bot::Action a = cands[best].act;
    a.sonar = gst.sonar;
    return a;
  }

  // the action finally sent, so the next turn's body includes its whole path
  void Commit(const bot::Action& a) {
    planned_.clear();
    if (a.kind != bot::Action::kMove || a.resign) return;
    int at = head_;
    for (int d : a.steps) {
      const int n = mem_.Step(at, d);
      if (n < 0) { planned_.clear(); return; }
      planned_.push_back(n);
      at = n;
    }
  }

 private:
  struct QCand {
    bot::Action act;
    int tile;        // the head's tile after a single step (-1: a split)
    double value;
    bool gst;        // gst's own choice
    int risk = 0;
    double room = 0;
  };
  struct Sighting {
    int id, tile, round, len;
  };

  // enemy heads seen in the last kMemoryRounds rounds but not in view now: Manhattan reach,
  // grown by the rounds since (they may have walked toward us)
  int MemoryRisk(int t) const {
    int risk = 0;
    for (const Sighting& g : sightings_) {
      if (g.round == round_) continue;   // in view: HeadRisk handles it
      const int reach = (g.len + 3 + 3) / 4 + g.len + 3 - 2 + (round_ - g.round);
      if (Dist(g.tile, t) <= std::min(reach, 10)) ++risk;
    }
    return risk;
  }

  int Dist(int a, int b) const {
    int dx = std::abs(mem_.X(a) - mem_.X(b)), dy = std::abs(mem_.Y(a) - mem_.Y(b));
    return std::min(dx, W_ - dx) + std::min(dy, H_ - dy);
  }

  void TrackBody(const rl::Observation& o) {
    std::vector<int> vis;
    for (const rl::Segment& s : o.segments)
      if (s.id == sp_.id) vis.push_back(mem_.Tile(s.x, s.y));
    if (int(vis.size()) == L_ || body_.empty()) {
      body_.assign(vis.begin(), vis.end());
      if (body_.empty() || body_.front() != head_) body_.push_front(head_);
    } else if (body_.front() != head_) {
      if (!planned_.empty() && planned_.back() == head_) {
        for (int t : planned_)
          if (body_.front() != t) body_.push_front(t);
      } else {
        body_.push_front(head_);
      }
    }
    planned_.clear();
    while (int(body_.size()) > L_) body_.pop_back();
  }

  // tiles reachable from the head at spawn through free, seen ground (an unseen edge: plenty)
  int SpawnRoom(const rl::Observation& o) const {
    std::vector<int> occ;
    for (const rl::Segment& s : o.segments) occ.push_back(mem_.Tile(s.x, s.y));
    std::vector<int> seen{head_}, q{head_};
    for (size_t qi = 0; qi < q.size() && static_cast<int>(seen.size()) <= L_; ++qi)
      for (int k = 0; k < 4; ++k) {
        const int n = mem_.Step(q[qi], k);
        if (n == strat::kUnknown) return kInf;
        if (n < 0 || std::find(occ.begin(), occ.end(), n) != occ.end() ||
            std::find(seen.begin(), seen.end(), n) != seen.end())
          continue;
        seen.push_back(n);
        q.push_back(n);
      }
    return static_cast<int>(seen.size()) - 1;
  }

  // enemy heads in view that could stand on tile t after their next move (1.2.3 reach), weighted
  int HeadRisk(const rl::Observation& o, int t) const {
    int risk = 0;
    const Sim& s = base_;
    for (size_t e = 1; e < s.d.size(); ++e) {
      const SimDragon& g = s.d[e];
      if (g.team == sp_.team || !g.moves || g.body.empty()) continue;
      int Le = static_cast<int>(g.body.size());
      bool edge = false;
      for (int b : g.body) {
        const int dx = std::abs(mem_.X(b) - o.head_x()), dy = std::abs(mem_.Y(b) - o.head_y());
        if (std::min(dx, W_ - dx) == 3 || std::min(dy, H_ - dy) == 3) edge = true;
      }
      if (edge) Le += 3;
      const int reach = std::min((Le + 3) / 4 + Le - 2, 8);
      // BFS from its head through free tiles (t itself is free)
      std::vector<std::pair<int, int>> q{{g.body[0], 0}};
      std::vector<int> seen{g.body[0]};
      int dist = -1;
      for (size_t qi = 0; qi < q.size() && dist < 0; ++qi) {
        const auto [at, dd] = q[qi];
        if (dd >= reach) continue;
        for (int k = 0; k < 4; ++k) {
          const int n = nb_[at * 4 + k];
          if (n < 0 || std::find(seen.begin(), seen.end(), n) != seen.end()) continue;
          if (n == t) { dist = dd + 1; break; }
          if (s.occ[n] >= 0) continue;
          seen.push_back(n);
          q.push_back({n, dd + 1});
        }
      }
      if (dist > 0) risk += dist == 1 ? 3 : 1;
    }
    return risk;
  }

  // enemy heads in view near tile t
  double Crowd(const rl::Observation& o, int t) const {
    (void)o;
    double c = 0;
    const Sim& s = base_;
    for (size_t e = 1; e < s.d.size(); ++e) {
      const SimDragon& g = s.d[e];
      if (g.team == sp_.team || !g.moves || g.body.empty()) continue;
      const int near = std::max(0, kCrowdRadius - Dist(g.body[0], t));
      c += near * near;
    }
    return c;
  }

  // how far the room after stepping onto t falls short of what the queen needs; our own segment i
  // (from the head) frees after L - i moves, so a body that can follow its tail has room
  double RoomShort(int t) const {
    const int need = std::min(L_ + 2, kRoomCap);
    std::vector<int> free_at(mem_.size(), 0);
    for (size_t i = 0; i < body_.size(); ++i) free_at[body_[i]] = L_ - static_cast<int>(i) + 1;
    const Sim& s = base_;
    std::vector<int> dist(mem_.size(), -1);
    std::vector<int> q{t};
    dist[t] = 1;
    int count = 1;
    for (size_t qi = 0; qi < q.size() && count < need; ++qi) {
      const int at = q[qi];
      for (int k = 0; k < 4; ++k) {
        const int n = nb_[at * 4 + k];
        if (n < 0 || dist[n] >= 0) continue;
        if (s.occ[n] > 0) continue;                                    // another dragon
        if (s.occ[n] == 0 && free_at[n] > dist[at] + 1) continue;     // our body, not yet freed
        dist[n] = dist[at] + 1;
        q.push_back(n);
        ++count;
      }
    }
    return static_cast<double>(std::max(0, need - count)) / need;
  }

  double Rollout(const bot::Action& act) {
    sim_ = base_;
    Sim& s = sim_;
    const double start = s.Material(0, 1.0);
    if (act.kind == bot::Action::kSplit) s.Split(0, act.split);
    else if (!act.resign) {
      const int free = (static_cast<int>(s.d[0].body.size()) + 3) / 4;
      for (size_t k = 0; k < act.steps.size(); ++k)
        if (!s.Step(0, act.steps[k], static_cast<int>(k) >= free)) break;
    }
    for (int t = 0; t < kHorizon; ++t) {
      if (t > 0) {
        s.Tick(t);
        s.Policy(0, rng_, false);
      }
      const size_t n = s.d.size();
      for (size_t i = 1; i < n; ++i) s.Policy(int(i), rng_, s.d[i].team != sp_.team || round_ + t < kSplitUntil);
    }
    return s.Material(0, 1.0) - start - (s.d[0].alive ? 0.0 : kQueenDeath);
  }

  void BuildModel(const rl::Observation& o) {
    const int N = mem_.size();
    Sim& s = base_;
    s.nb = &nb_;
    s.W = W_;
    s.H = H_;
    s.my_team = sp_.team;
    s.occ.assign(N, -1);
    s.pearl.assign(N, 0);
    s.pearls.clear();
    s.d.clear();
    s.beds.clear();
    SimDragon me;
    me.team = sp_.team;
    me.body.assign(body_.begin(), body_.end());
    s.d.push_back(me);
    // other dragons in view, bodies in listed order (head first when its head is in view)
    std::vector<int> ids;
    for (const rl::Segment& g : o.segments)
      if (g.id != sp_.id && std::find(ids.begin(), ids.end(), g.id) == ids.end()) ids.push_back(g.id);
    for (int id : ids) {
      SimDragon g;
      bool head = false;
      for (const rl::Segment& q : o.segments)
        if (q.id == id) {
          g.team = q.team;
          if (q.head) head = true;
          g.body.push_back(mem_.Tile(q.x, q.y));
        }
      g.moves = head;
      s.d.push_back(g);
    }
    for (size_t i = 0; i < s.d.size(); ++i)
      for (int t : s.d[i].body) s.occ[t] = int16_t(i);
    for (int t : s.d[0].body) s.occ[t] = 0;   // our own body wins any stale overlap
    for (const rl::Segment& g : o.segments)     // and every own segment in view, whatever the history says
      if (g.id == sp_.id) s.occ[mem_.Tile(g.x, g.y)] = 0;
    // pearls and beds near us
    const int hx = o.head_x(), hy = o.head_y();
    for (int dy = -kModelRadius; dy <= kModelRadius; ++dy)
      for (int dx = -kModelRadius + std::abs(dy); dx <= kModelRadius - std::abs(dy); ++dx) {
        const int t = mem_.Tile(hx + dx, hy + dy);
        const strat::TileMemory& m = mem_.at(t);
        if (m.seen_round < 0) continue;
        if (m.pearl && round_ - m.seen_round <= 25 && s.occ[t] < 0) s.AddPearl(t);
        if (m.countdown >= 0) {
          const int due = m.countdown - (round_ - m.seen_round);
          if (hot_[t]) s.beds.push_back({t, std::max(1, due), true});
          else if (due >= 1 && due <= kHorizon) s.beds.push_back({t, due, false});
        }
      }
  }


  rl::Spawn sp_;
  strat::BoardMemory mem_;
  std::mt19937 rng_;
  int W_ = 0, H_ = 0, round_ = 0, head_ = 0, L_ = 0;
  bool first_ = true, doomed_ = false;
  std::deque<int> body_;
  std::vector<int> planned_;
  std::vector<Sighting> sightings_;
  std::vector<int> nb_, low_seen_;
  std::vector<uint8_t> hot_;
  Sim base_, sim_;
};

}  // namespace qg
