// trading-swarm-2: a clone of the stronger real server opponent (M491281), built on
// trading-swarm-1 (a clone of split-v7, M489641). Adds: SPLIT 2 from length 4 always,
// sonar every turn (the champion's length/position, relayed), portal crossing when no
// pearl is reachable in view, the reverse split when boxed in, and late consolidation
// (from r380 short dragons resign beside a much longer ally so it eats their pearls).
// No sonar, no memory. Every dragon: trade heads whenever it can (adjacent enemy heads
// always; 2-4 step lunges from short dragons onto enemy heads at least as long), otherwise
// greedy nearest pearl in view, otherwise drift toward the map centre. Short dragons keep
// splitting off 2-long children (parent length 4-6) all game long; long ones don't.
#include <algorithm>
#include <deque>
#include <random>

#include "plugin.hpp"
#include "window.hpp"

namespace ts2 {

class SwarmAgent : public bot::Agent {
 public:
  explicit SwarmAgent(const rl::Spawn& s)
      : spawn_(s), rng_(static_cast<uint32_t>(s.id * 2654435761u + s.team * 97u + 12345u)) {
    NewThreshold();
  }

  bot::Action Turn(const rl::Observation& o) override {
    bot::Action a = Decide(o);
    if (!a.resign && champ_len_ > 0 && o.round - champ_round_ <= kRelayAge) {
      const uint64_t w = (1u << 31) | (uint32_t(std::min(champ_len_, 127)) << 24) |
                         (uint32_t(champ_x_ & 0xff) << 16) | (uint32_t(champ_y_ & 0xff) << 8) |
                         uint32_t(champ_round_ & 0xff);
      for (int d = 0; d < 4; ++d) a.sonar[d] = w;
    }
    return a;
  }

  bot::Action Decide(const rl::Observation& o) {
    const int L = o.length;
    Listen(o);
    bool occ[rl::kWindowTiles] = {};
    int enemy_head_len[rl::kWindowTiles];
    std::fill(std::begin(enemy_head_len), std::end(enemy_head_len), 0);
    for (const rl::Segment& s : o.segments) {
      const int k = TileOf(o, s.x, s.y);
      if (k >= 0) occ[k] = true;
    }
    for (const rl::Segment& s : o.segments) {
      if (!s.head || s.team == spawn_.team) continue;
      const int k = TileOf(o, s.x, s.y);
      if (k < 0) continue;
      int n = 0;
      for (const rl::Segment& t : o.segments) n += t.id == s.id;
      enemy_head_len[k] = n;
    }

    // 1. adjacent enemy head: take the even trade (a long dragon only for an enemy as long)
    {
      int best = -1, best_len = -1;
      for (int d = 0; d < 4; ++d) {
        const int k = win::Step(o, win::kCenter, d);
        if (k < 0 || !enemy_head_len[k]) continue;
        if (L > 8 && enemy_head_len[k] < L) continue;
        if (enemy_head_len[k] > best_len) best_len = enemy_head_len[k], best = d;
      }
      if (best >= 0) {
        why_ = "trade head";
        return bot::Action::Move(best);
      }
    }

    // 2. lunge: a 3-6 long dragon runs 2-4 steps onto an enemy head at least as long
    if (L >= 3 && L <= 6) {
      const int max_steps = std::min(4, L - 1);
      int dist[rl::kWindowTiles], prev[rl::kWindowTiles], pdir[rl::kWindowTiles];
      std::fill(std::begin(dist), std::end(dist), -1);
      dist[win::kCenter] = 0;
      std::deque<int> q{win::kCenter};
      int target = -1, target_len = 0, target_dist = 99;
      while (!q.empty()) {
        const int k = q.front();
        q.pop_front();
        if (dist[k] >= max_steps) continue;
        for (int d = 0; d < 4; ++d) {
          const int n = win::Step(o, k, d);
          if (n < 0 || dist[n] >= 0) continue;
          if (enemy_head_len[n]) {
            if (dist[k] + 1 >= 2 && enemy_head_len[n] >= std::max(L, 3) &&
                (enemy_head_len[n] > target_len ||
                 (enemy_head_len[n] == target_len && dist[k] + 1 < target_dist))) {
              dist[n] = dist[k] + 1, prev[n] = k, pdir[n] = d;
              target = n, target_len = enemy_head_len[n], target_dist = dist[n];
            }
            continue;
          }
          if (occ[n]) continue;
          dist[n] = dist[k] + 1, prev[n] = k, pdir[n] = d;
          q.push_back(n);
        }
      }
      if (target >= 0) {
        std::vector<int> steps;
        for (int k = target; k != win::kCenter; k = prev[k]) steps.push_back(pdir[k]);
        std::reverse(steps.begin(), steps.end());
        bot::Action a;
        a.kind = bot::Action::kMove;
        a.steps = steps;
        why_ = "lunge";
        return a;
      }
    }

    // 2b. pincer: a 2-3 long dragon closes on a long enemy head (in front of it), so the
    //     next turn's adjacent-head trade can take it
    if (L <= 3) {
      int best_k = -1, best_len = 0, best_face = 0;
      for (const rl::Segment& sg : o.segments) {
        if (!sg.head || sg.team == spawn_.team) continue;
        const int k = TileOf(o, sg.x, sg.y);
        if (k < 0 || enemy_head_len[k] < 5) continue;
        const int md = std::abs(k / 7 - 3) + std::abs(k % 7 - 3);
        if (md > 4 || md < 2) continue;
        if (enemy_head_len[k] > best_len) best_len = enemy_head_len[k], best_k = k, best_face = sg.facing;
      }
      if (best_k >= 0) {
        const int front = win::Step(o, best_k, best_face);   // the tile it's about to enter
        int first[rl::kWindowTiles];
        std::fill(std::begin(first), std::end(first), -1);
        std::deque<int> q;
        for (int d = 0; d < 4; ++d)
          if (const int k = win::Step(o, win::kCenter, d); k >= 0 && !occ[k]) first[k] = d, q.push_back(k);
        int go = -1, go_alt = -1;
        for (; !q.empty() && go < 0; q.pop_front()) {
          const int k = q.front();
          if (k == front) go = first[k];
          const int kd = std::abs(best_k / 7 - k / 7) + std::abs(best_k % 7 - k % 7);
          if (kd == 1 && go_alt < 0) go_alt = first[k];
          for (int d = 0; d < 4; ++d)
            if (const int n = win::Step(o, k, d); n >= 0 && n != win::kCenter && first[n] < 0 && !occ[n])
              first[n] = first[k], q.push_back(n);
        }
        if (go < 0) go = go_alt;
        if (go >= 0) {
          why_ = "pincer";
          return bot::Action::Move(go);
        }
      }
    }

    // 3. split off a 2-long child whenever length allows (not the late champion)
    const bool champ = champ_x_ == o.head_x() && champ_y_ == o.head_y();
    const bool rare = L > 8 && o.round < 300 && std::uniform_int_distribution<int>(0, 39)(rng_) == 0;
    const bool late_champ = o.round >= 330 && champ;
    if (((L >= 4 && L <= 7) || rare) && !late_champ && o.unit_count < spawn_.unit_limit) {
      why_ = "split 2";
      return bot::Action::Split(2);
    }

    // 3b. late consolidation: a short dragon feeds itself to a much longer ally
    goal_ = 0;
    if (o.round >= kLateRound && L <= 5 && !champ) {
      const int need = std::max(L + 3, 6);
      int best_k = -1, best_len = 0;
      for (const rl::Segment& sg : o.segments) {
        if (!sg.head || sg.team != spawn_.team || sg.id == spawn_.id) continue;
        const int k = TileOf(o, sg.x, sg.y);
        if (k < 0) continue;
        int n = 0;
        for (const rl::Segment& t : o.segments) n += t.id == sg.id;
        if (n >= need && n > best_len) best_len = n, best_k = k;
      }
      if (best_k >= 0) {
        const int md = std::abs(best_k / 7 - 3) + std::abs(best_k % 7 - 3);
        if (md <= 2) {
          why_ = "feed";
          return bot::Action::Resign();
        }
        int first[rl::kWindowTiles];
        std::fill(std::begin(first), std::end(first), -1);
        std::deque<int> q;
        for (int d = 0; d < 4; ++d)
          if (const int k = win::Step(o, win::kCenter, d); k >= 0 && !occ[k]) first[k] = d, q.push_back(k);
        for (; !q.empty(); q.pop_front()) {
          const int k = q.front();
          const int kd = std::abs(best_k / 7 - k / 7) + std::abs(best_k % 7 - k % 7);
          if (kd <= 2) {
            why_ = "to champion";
            return bot::Action::Move(first[k]);
          }
          for (int d = 0; d < 4; ++d)
            if (const int n = win::Step(o, k, d); n >= 0 && n != win::kCenter && first[n] < 0 && !occ[n])
              first[n] = first[k], q.push_back(n);
        }
      }
      if (champ_len_ >= need && o.round - champ_round_ <= kRelayAge) goal_ = 1;
    }

    // 4. greedy nearest pearl in view
    {
      int first[rl::kWindowTiles];
      std::fill(std::begin(first), std::end(first), -1);
      std::deque<int> q;
      for (int d = 0; d < 4; ++d)
        if (const int k = win::Step(o, win::kCenter, d); k >= 0 && !occ[k] && first[k] < 0) {
          first[k] = d;
          q.push_back(k);
        }
      int bed_dir = -1, bed_cd = 1 << 30;
      int depth[rl::kWindowTiles] = {};
      for (int k2 = 0; k2 < rl::kWindowTiles; ++k2) depth[k2] = first[k2] >= 0 ? 1 : 0;
      for (; !q.empty(); q.pop_front()) {
        const int k = q.front();
        if (o.pearl[k]) {
          why_ = "nearest pearl";
          return bot::Action::Move(first[k]);
        }
        if (o.countdown[k] >= 0 && depth[k] >= 2 && o.countdown[k] - depth[k] < bed_cd)
          bed_cd = o.countdown[k] - depth[k], bed_dir = first[k];
        for (int d = 0; d < 4; ++d)
          if (const int n = win::Step(o, k, d); n >= 0 && n != win::kCenter && first[n] < 0 && !occ[n]) {
            first[n] = first[k];
            depth[n] = depth[k] + 1;
            q.push_back(n);
          }
      }
      if (bed_dir >= 0 && bed_cd <= 8) {
        why_ = "to bed";
        return bot::Action::Move(bed_dir);
      }
    }

    // 5. no pearl reachable in view: cross a portal now and then (always if pearls are
    //    visible but unreachable: portal-only stores)
    if (goal_ == 0) {
      bool pearl_seen = false;
      for (int k = 0; k < rl::kWindowTiles; ++k) pearl_seen |= o.pearl[k] != 0;
      const int pd = PortalDir(o, occ);
      if (pd >= 0 && (pearl_seen || std::uniform_int_distribution<int>(0, 2)(rng_) == 0)) {
        why_ = "cross portal";
        return bot::Action::Move(pd);
      }
    }

    // 6. drift toward the centre (the pearl fields) or the champion, else straight on / wander
    int free[4], nf = 0;
    for (int d = 0; d < 4; ++d)
      if (const int k = win::Step(o, win::kCenter, d); k >= 0 && !occ[k] && Room(o, occ, k) >= 3) free[nf++] = d;
    if (nf == 0 && L >= 5 && o.unit_count < spawn_.unit_limit) {
      why_ = "reverse split";
      return bot::Action::Split(L - 2);
    }
    if (nf == 0)
      for (int d = 0; d < 4; ++d)
        if (const int k = win::Step(o, win::kCenter, d); k >= 0 && !occ[k]) free[nf++] = d;
    if (nf == 0) {
      why_ = "stuck";
      for (int d = 0; d < 4; ++d)   // die somewhere that isn't an ally's head
        if (const int k = win::Step(o, win::kCenter, d); k < 0 || !AllyHead(o, k)) return bot::Action::Move(d);
      return bot::Action::Move(o.facing);
    }
    auto ok = [&](int d) { return std::find(free, free + nf, d) != free + nf; };
    const int gx = goal_ ? champ_x_ : spawn_.width / 2, gy = goal_ ? champ_y_ : spawn_.height / 2;
    const int dx = TorusDelta(o.head_x(), gx, spawn_.width);
    const int dy = TorusDelta(o.head_y(), gy, spawn_.height);
    if (std::abs(dx) + std::abs(dy) > (goal_ ? 1 : 4)) {
      int pref[2] = {-1, -1};
      const int hx = dx > 0 ? 1 : dx < 0 ? 3 : -1, hy = dy > 0 ? 2 : dy < 0 ? 0 : -1;
      if (std::abs(dx) >= std::abs(dy)) pref[0] = hx, pref[1] = hy;
      else pref[0] = hy, pref[1] = hx;
      for (int d : pref)
        if (d >= 0 && ok(d) && std::uniform_int_distribution<int>(0, 5)(rng_) != 0) {
          why_ = goal_ ? "to champion" : "to centre";
          return bot::Action::Move(d);
        }
    }
    if (ok(o.facing) && std::uniform_int_distribution<int>(0, 3)(rng_) != 0) {
      why_ = "wander";
      return bot::Action::Move(o.facing);
    }
    why_ = "wander";
    return bot::Action::Move(free[std::uniform_int_distribution<int>(0, nf - 1)(rng_)]);
  }

  const char* reason() const override { return why_; }

 private:
  void NewThreshold() {
    const int r = std::uniform_int_distribution<int>(0, 9)(rng_);
    threshold_ = r < 3 ? 4 : r < 7 ? 5 : 6;
  }

  static constexpr int kRelayAge = 12, kLateRound = 350;

  // sonar: remember the longest ally we've heard of (or are)
  void Listen(const rl::Observation& o) {
    if (o.round - champ_round_ > kRelayAge) champ_len_ = 0;
    for (uint64_t w : o.inbox) {
      if (!(w >> 31 & 1) || w >> 32) continue;
      const int len = int(w >> 24 & 0x7f), x = int(w >> 16 & 0xff), y = int(w >> 8 & 0xff);
      const int age = (o.round - int(w & 0xff)) & 0xff;
      if (age > kRelayAge || x >= spawn_.width || y >= spawn_.height) continue;
      const int r = o.round - age;
      if (len > champ_len_ || (len == champ_len_ && r > champ_round_))
        champ_len_ = len, champ_x_ = x, champ_y_ = y, champ_round_ = r;
    }
    if (o.length >= champ_len_) champ_len_ = o.length, champ_x_ = o.head_x(), champ_y_ = o.head_y(), champ_round_ = o.round;
  }

  static int EdgeCode(const rl::Observation& o, int k, int d, int* idx) {
    const int r = k / rl::kWindow, c = k % rl::kWindow, W = rl::kWindow;
    if (d == 0) return o.hedge[*idx = r * W + c];
    if (d == 2) return o.hedge[*idx = (r + 1) * W + c];
    if (d == 3) return o.vedge[*idx = r * (W + 1) + c];
    return o.vedge[*idx = r * (W + 1) + c + 1];
  }

  // a direction whose edge from the head is a portal, landing (when visible) on a free tile
  int PortalDir(const rl::Observation& o, const bool* occ) {
    int cand[4], nc = 0;
    const int W = rl::kWindow;
    for (int d = 0; d < 4; ++d) {
      int idx;
      const int id = EdgeCode(o, win::kCenter, d, &idx);
      if (id < 0 || d == (o.facing + 2) % 4) continue;
      if (id == last_portal_ && o.round - last_portal_round_ <= o.length + 1) continue;   // our body is over there
      int land = -2;
      if (d == 0 || d == 2) {
        for (int j = 0; j < (W + 1) * W; ++j) {
          if (j == idx || o.hedge[j] != id) continue;
          const int rr = j / W, cc = j % W, lr = d == 0 ? rr - 1 : rr;
          land = lr >= 0 && lr < W ? lr * W + cc : -2;
          break;
        }
      } else {
        for (int j = 0; j < W * (W + 1); ++j) {
          if (j == idx || o.vedge[j] != id) continue;
          const int rr = j / (W + 1), cc = j % (W + 1), lc = d == 3 ? cc - 1 : cc;
          land = lc >= 0 && lc < W ? rr * W + lc : -2;
          break;
        }
      }
      if (land >= 0 && (occ[land] || o.tile_x[land] < 0)) continue;
      cand[nc++] = d;
    }
    if (!nc) return -1;
    const int d = cand[std::uniform_int_distribution<int>(0, nc - 1)(rng_)];
    int idx;
    last_portal_ = EdgeCode(o, win::kCenter, d, &idx), last_portal_round_ = o.round;
    return d;
  }

  static int TileOf(const rl::Observation& o, int x, int y) {
    for (int k = 0; k < rl::kWindowTiles; ++k)
      if (o.tile_x[k] == x && o.tile_y[k] == y) return k;
    return -1;
  }

  bool AllyHead(const rl::Observation& o, int k) const {
    for (const rl::Segment& s : o.segments)
      if (s.head && s.team == spawn_.team && s.x == o.tile_x[k] && s.y == o.tile_y[k]) return true;
    return false;
  }

  static int TorusDelta(int from, int to, int n) {
    int d = ((to - from) % n + n) % n;
    if (d > n / 2) d -= n;
    return d;
  }

  // free tiles reachable from k within the window (capped), a cheap dead-end check
  static int Room(const rl::Observation& o, const bool* occ, int start) {
    bool seen[rl::kWindowTiles] = {};
    seen[start] = seen[win::kCenter] = true;
    int stack[rl::kWindowTiles], sp = 0, n = 0;
    stack[sp++] = start;
    while (sp && n < 8) {
      const int k = stack[--sp];
      ++n;
      for (int d = 0; d < 4; ++d)
        if (const int m = win::Step(o, k, d); m >= 0 && !seen[m] && !occ[m]) {
          seen[m] = true;
          stack[sp++] = m;
        }
    }
    return n;
  }

  rl::Spawn spawn_;
  std::mt19937 rng_;
  int threshold_ = 5;
  int champ_len_ = 0, champ_x_ = -1, champ_y_ = -1, champ_round_ = -1000, goal_ = 0;
  int last_portal_ = -1, last_portal_round_ = -1000;
  const char* why_ = "";
};

}  // namespace ts2

LEAGUE_VARIANT(ts2::SwarmAgent)
