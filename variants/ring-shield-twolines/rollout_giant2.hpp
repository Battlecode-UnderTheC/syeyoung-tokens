// gst-giant-1's rollout judge: the forward model and Monte Carlo rollouts from arena-giant-r7,
// used to review gather-split-trade-131's move when an enemy head is in view.
//
// Each reviewed turn the dragon builds a model of what it knows (board with portals and wrap, its
// own full body, every dragon in view, remembered pearls, beds with known countdowns, hot beds)
// and plays R rollouts of H rounds for each candidate: gst's own action, the four single steps,
// and a lunge onto an enemy head at least as long. In the rollouts every dragon moves by a
// sampled policy (trade into an adjacent opposing head with probability 1/2, otherwise toward the
// nearest pearl, epsilon-random safe steps; split at 4 while splitting). The value is the change
// in (our length in view - enemy length in view). gst's action stands unless another candidate's
// rollout value beats it by kMargin.
#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <deque>
#include <random>
#include <vector>

#include "agent.hpp"
#include "board_memory.hpp"

namespace gstg1r {

constexpr int kInf = 1 << 20;
// unswbc 1.2.3 MOVE: a dragon of length L (fixed when the move starts) takes its first ceil(L/4)
// steps free; every later step costs a tail segment and needs length > 2
inline int FreeSteps(int L) { return (L + 3) / 4; }
inline int MoveReach(int L) { return FreeSteps(L) + std::max(0, L - 2); }
constexpr int kHorizon = 8;          // rounds per rollout
constexpr int kRollouts = 16;        // rollouts per candidate action
constexpr double kEps = 0.15;        // random safe step in the policy
constexpr double kTrade = 0.5;       // probability a head trades into an adjacent opposing head
constexpr bool kLungePolicy = true;  // rollout dragons also lunge (2-4 steps) onto opposing heads
constexpr double kLunge = 0.5;       // ... with this probability when one is in reach
constexpr double kMargin = 1.0;      // a candidate must beat gst's action by this much (length)
constexpr int kModelRadius = 14;     // pearls and beds within this distance go into the model
constexpr int kSplitUntil = 360;     // our dragons split in rollouts until this round

// ---------------------------------------------------------------- the forward model
struct SimDragon {
  std::vector<int> body;   // head first
  int team = 0;
  bool alive = true, moves = true;
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
  // one step of dragon i (engine: Game::Step); pay = a step beyond the move's FreeSteps(L)
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
    if (trade >= 0 && U(rng) < kTrade) { Step(i, trade, false); return; }
    if (kLungePolicy && L >= 3 && U(rng) < kLunge) {   // a multi-step lunge onto an opposing head at least as long
      int steps[8];
      const int n = LungePath(i, steps);
      if (n > 0) {
        const int free = FreeSteps(L);
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
  // shortest free path (2..min(MoveReach(L), 4) steps) from dragon i's head onto an opposing head
  // at least as long; returns its length in steps (0: none)
  int LungePath(int i, int* out) const {
    const SimDragon& g = d[i];
    const int L = int(g.body.size()), reach = std::min(MoveReach(L), 4);
    struct Node { int tile, parent, dir, depth; };
    Node q[64];
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
          if (cur.depth >= 1 && d[o].team != g.team && d[o].body[0] == n && d[o].body.size() >= g.body.size()) {
            int len = cur.depth + 1, at = qi - 1;
            out[len - 1] = k;
            for (int j = len - 2; j >= 0; --j) { out[j] = q[at].dir; at = q[at].parent; }
            return len;
          }
          continue;
        }
        bool seen = false;
        for (int j = 0; j < qn; ++j) if (q[j].tile == n) { seen = true; break; }
        if (seen || qn >= 64) continue;
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


struct Candidate {
  bot::Action act;
  double value = 0;
};

class Judge {
 public:
  explicit Judge(const rl::Spawn& s)
      : sp_(s), mem_(s.width, s.height), rng_(uint32_t(s.id * 7919 + s.team * 104729 + 1)) {}

  // every turn: keep the model's memory and our body current
  bool Reviewed() const { return reviewed_; }
  void Observe(const rl::Observation& o) {
    reviewed_ = false;
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
  }

  // the action this dragon finally sent, so the next turn's body includes its whole path
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

  // gst's action steps onto our own body (known through a portal exit out of view, say)
  bool IntoOwnBody(const bot::Action& a) const {
    if (a.kind != bot::Action::kMove || a.resign || a.steps.empty()) return false;
    const int n = mem_.Step(head_, a.steps[0]);
    return n >= 0 && std::find(body_.begin(), body_.end(), n) != body_.end();
  }

  bool EnemyHeadInView(const rl::Observation& o) const {
    for (const rl::Segment& s : o.segments)
      if (s.head && s.team != sp_.team) return true;
    return false;
  }

  // gst's action, or a candidate whose rollouts beat it clearly
  bool NextToOtherHead(int t) const {
    for (int k = 0; k < 4; ++k) {
      const int n = nb_[t * 4 + k];
      if (n < 0 || n == head_) continue;
      const int o = base_.occ[n];
      if (o > 0 && base_.d[o].moves && base_.d[o].body[0] == n) return true;
    }
    return false;
  }
  bool OtherHeadAndPearlInView(const rl::Observation& o) const {
    bool head = false, pearl = false;
    for (const rl::Segment& s : o.segments)
      if (s.head && s.id != sp_.id) head = true;
    for (int k = 0; k < rl::kWindowTiles; ++k)
      if (o.pearl[k]) pearl = true;
    return head && pearl;
  }

  // with_splits: SPLIT 2 and SPLIT L-2 are candidates too (escapes)
  // cautious: an alternative step may not end next to another dragon's head (contest reviews)
  // veto: gst's action is known fatal, so any other candidate replaces it
  bot::Action Review(const rl::Observation& o, const bot::Action& gst, bool* overridden, bool with_splits = false,
                     bool cautious = false, bool veto = false) {
    *overridden = false;
    reviewed_ = true;
    const int N = mem_.size();
    for (int t = 0; t < N; ++t)
      for (int k = 0; k < 4; ++k) {
        int n = mem_.Step(t, k);
        if (n == strat::kUnknown) {
          // an unseen edge: assume open floor; but between two seen tiles it is a portal whose
          // far end we don't know, so the model treats it as closed
          const int g = mem_.Tile(mem_.X(t) + dragons::kDx[k], mem_.Y(t) + dragons::kDy[k]);
          n = mem_.at(t).seen_round >= 0 && mem_.at(g).seen_round >= 0 ? -1 : g;
        }
        nb_[t * 4 + k] = n;
      }
    BuildModel(o);
    std::vector<Candidate> cands;
    cands.push_back({gst, 0});
    for (int d = 0; d < 4; ++d) {
      const int n = nb_[head_ * 4 + d];
      if (n < 0 || base_.occ[n] == 0) continue;
      if (gst.steps.size() == 1 && gst.steps[0] == d) continue;
      if (cautious && NextToOtherHead(n)) continue;
      cands.push_back({bot::Action::Move(d), 0});
    }
    if (with_splits && L_ >= 4 && o.unit_count < sp_.unit_limit) {
      for (int n : {2, L_ - 2}) {
        if (gst.kind == bot::Action::kSplit && gst.split == n) continue;
        cands.push_back({bot::Action::Split(n), 0});
        if (L_ - 2 == 2) break;
      }
    }
    if (const std::vector<int> lunge = Lunge(o); !lunge.empty() && lunge != gst.steps) {
      bot::Action a = bot::Action::Move(lunge[0]);
      a.steps = lunge;
      cands.push_back({a, 0});
    }
    const uint32_t seed = rng_();
    for (Candidate& c : cands) {
      rng_.seed(seed);   // common random numbers: every candidate faces the same sampled futures
      double sum = 0;
      for (int r = 0; r < kRollouts; ++r) sum += Rollout(c);
      c.value = sum / kRollouts;
    }
    rng_.seed(seed + 1);
    size_t best = 0;
    for (size_t i = 1; i < cands.size(); ++i)
      if (cands[i].value > cands[best].value) best = i;
    if (veto && cands.size() > 1) {
      best = 1;
      for (size_t i = 2; i < cands.size(); ++i)
        if (cands[i].value > cands[best].value) best = i;
    } else if (best == 0 || cands[best].value < cands[0].value + kMargin) {
      return gst;
    }
    *overridden = true;
    bot::Action a = cands[best].act;
    a.sonar = gst.sonar;
    return a;
  }

 private:
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
      // a multi-step move (a lunge, a portal crossing) laid every tile of its path
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

  double Rollout(const Candidate& c) {
    sim_ = base_;
    Sim& s = sim_;
    const double start = s.Material(0, 1.0);
    if (c.act.kind == bot::Action::kSplit) s.Split(0, c.act.split);
    else if (c.act.resign) s.Kill(0);
    else
      for (size_t k = 0; k < c.act.steps.size(); ++k)
        if (!s.Step(0, c.act.steps[k], int(k) >= FreeSteps(L_))) break;
    for (int t = 0; t < kHorizon; ++t) {
      if (t > 0) {
        s.Tick(t);
        s.Policy(0, rng_, round_ + t < kSplitUntil);
      }
      const size_t n = s.d.size();
      for (size_t i = 1; i < n; ++i) s.Policy(int(i), rng_, s.d[i].team != sp_.team || round_ + t < kSplitUntil);
    }
    return s.Material(0, 1.0) - start;
  }

  // The model of what this dragon knows now. Index 0 is us.
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

  // shortest free path onto an enemy head at least as long, within reach
  std::vector<int> Lunge(const rl::Observation& o) {
    if (L_ < 2) return {};
    const Sim& s = base_;
    for (size_t e = 1; e < s.d.size(); ++e) {
      const SimDragon& g = s.d[e];
      if (g.team == sp_.team || !g.moves || int(g.body.size()) < L_) continue;
      const int target = g.body[0];
      std::vector<int> prev(mem_.size(), -2), pdir(mem_.size(), -1), dist(mem_.size(), -1);
      std::deque<int> q{head_};
      dist[head_] = 0;
      bool found = false;
      while (!q.empty() && !found) {
        const int t = q.front();
        q.pop_front();
        if (dist[t] >= MoveReach(L_)) continue;
        for (int k = 0; k < 4 && !found; ++k) {
          const int n = nb_[t * 4 + k];
          if (n < 0 || dist[n] >= 0) continue;
          if (n != target && s.occ[n] >= 0) continue;
          dist[n] = dist[t] + 1, prev[n] = t, pdir[n] = k;
          if (n == target) found = true;
          else q.push_back(n);
        }
      }
      if (!found) continue;
      std::vector<int> steps;
      for (int t = target; t != head_; t = prev[t]) steps.push_back(pdir[t]);
      std::reverse(steps.begin(), steps.end());
      if (steps.size() >= 2) return steps;   // one-step trades are already a move candidate
    }
    (void)o;
    return {};
  }


  rl::Spawn sp_;
  strat::BoardMemory mem_;
  std::mt19937 rng_;
  bool reviewed_ = false;
  int W_ = 0, H_ = 0, round_ = 0, head_ = 0, L_ = 0;
  std::deque<int> body_;
  std::vector<int> planned_;
  std::vector<int> nb_, low_seen_;
  std::vector<uint8_t> hot_;
  Sim base_, sim_;
};

}  // namespace gstg1r
