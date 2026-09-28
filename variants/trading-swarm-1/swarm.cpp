// trading-swarm-1: a clone of the first real server opponent (split-v7, M489641).
// No sonar, no memory. Every dragon: trade heads whenever it can (adjacent enemy heads
// always; 2-4 step lunges from short dragons onto enemy heads at least as long), otherwise
// greedy nearest pearl in view, otherwise drift toward the map centre. Short dragons keep
// splitting off 2-long children (parent length 4-6) all game long; long ones don't.
#include <algorithm>
#include <deque>
#include <random>

#include "plugin.hpp"
#include "window.hpp"

namespace ts1 {

class SwarmAgent : public bot::Agent {
 public:
  explicit SwarmAgent(const rl::Spawn& s)
      : spawn_(s), rng_(static_cast<uint32_t>(s.id * 2654435761u + s.team * 97u + 12345u)) {
    NewThreshold();
  }

  bot::Action Turn(const rl::Observation& o) override {
    const int L = o.length;
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

    // 3. split off a 2-long child while short
    const bool rare = L > 8 && std::uniform_int_distribution<int>(0, 39)(rng_) == 0;   // long ones: now and then
    if (((L >= threshold_ && L <= 8 && L >= 4) || rare) && o.unit_count < spawn_.unit_limit) {
      NewThreshold();
      why_ = "split 2";
      return bot::Action::Split(2);
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

    // 5. no pearl in view: drift toward the centre (the pearl fields), else straight on / wander
    int free[4], nf = 0;
    for (int d = 0; d < 4; ++d)
      if (const int k = win::Step(o, win::kCenter, d); k >= 0 && !occ[k] && Room(o, occ, k) >= 3) free[nf++] = d;
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
    const int dx = TorusDelta(o.head_x(), spawn_.width / 2, spawn_.width);
    const int dy = TorusDelta(o.head_y(), spawn_.height / 2, spawn_.height);
    if (std::abs(dx) + std::abs(dy) > 4) {
      int pref[2] = {-1, -1};
      const int hx = dx > 0 ? 1 : dx < 0 ? 3 : -1, hy = dy > 0 ? 2 : dy < 0 ? 0 : -1;
      if (std::abs(dx) >= std::abs(dy)) pref[0] = hx, pref[1] = hy;
      else pref[0] = hy, pref[1] = hx;
      for (int d : pref)
        if (d >= 0 && ok(d) && std::uniform_int_distribution<int>(0, 5)(rng_) != 0) {
          why_ = "to centre";
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
  const char* why_ = "";
};

}  // namespace ts1

LEAGUE_VARIANT(ts1::SwarmAgent)
