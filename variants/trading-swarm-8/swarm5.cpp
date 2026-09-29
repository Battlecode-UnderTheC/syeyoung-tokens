// trading-swarm-2: a clone of the stronger real server opponent (M491281), built on
// trading-swarm-1 (a clone of split-v7, M489641). Adds: SPLIT 2 from length 4 always,
// sonar every turn (the champion's length/position, relayed), portal crossing when no
// pearl is reachable in view, the reverse split when boxed in, and late consolidation
// (from r380 short dragons resign beside a much longer ally so it eats their pearls).
// trading-swarm-4: routes over a remembered board (BoardMemory) through portal pairs.
// trading-swarm-5: crosses portals readily and farms portal-only rooms (beds reachable
// from the dragon's home only through a portal) on their timers, like the real one.
// Originally: no sonar, no memory. Every dragon: trade heads whenever it can (adjacent enemy heads
// always; 2-4 step lunges from short dragons onto enemy heads at least as long), otherwise
// greedy nearest pearl in view, otherwise drift toward the map centre. Short dragons keep
// splitting off 2-long children (parent length 4-6) all game long; long ones don't.
#include <algorithm>
#include <deque>
#include <random>
#ifdef TS8_DEBUG
#include <cstdio>
#endif

#include "plugin.hpp"
#include "window.hpp"
#include "board_memory.hpp"

namespace ts8 {

inline constexpr bool kNew = true;

class SwarmAgent : public bot::Agent {
 public:
  explicit SwarmAgent(const rl::Spawn& s)
      : spawn_(s), rng_(static_cast<uint32_t>(s.id * 2654435761u + s.team * 97u + 12345u)),
        mem_(s.width, s.height), portal_(size_t(s.width) * s.height, 0) {
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
    // stepping next to a portal whose far end we have never seen: ping only through it,
    // so next turn's echoes say whether a dragon lies past its exit
    if (!a.resign && a.kind == bot::Action::kMove && a.steps.size() == 1 && o.round < 480) {
      const int d = a.steps[0];
      const int n = mem_.Step(mem_.Tile(o.head_x(), o.head_y()), d);
      if (n >= 0 && mem_.Current(n))
        for (int e = 0; e < 4; ++e)
          if (e != (d + 2) % 4 && (portal_[n] >> e & 1) && mem_.Step(n, e) == strat::kUnknown) {
            const uint64_t w = a.sonar[e].value_or(1);
            a.sonar = {};
            a.sonar[e] = w;
            ping_round_ = o.round, ping_tile_ = n, ping_dir_ = e;
            break;
          }
    }
    return a;
  }

  bot::Action Decide(const rl::Observation& o) {
    const int L = o.length;
    Listen(o);
    Remember(o);
    if (home_ < 0) home_ = mem_.Tile(o.head_x(), o.head_y());
    HomeComp();
    const bool inside = kNew && !home_comp_[mem_.Tile(o.head_x(), o.head_y())];   // in a portal-only room
#ifdef TS8_DEBUG
    {
      const int tm = spawn_.team & 1;
      if (o.round == 0 && dbg_last_[tm] != 0) {
        delete dbg_mem_[tm];
        dbg_mem_[tm] = new strat::BoardMemory(spawn_.width, spawn_.height);
        dbg_portal_[tm].assign(size_t(spawn_.width) * spawn_.height, 0);
        dbg_home_[tm] = home_;
      }
      dbg_last_[tm] = o.round;
      dbg_mem_[tm]->Update(o);
      for (int t = 0; t < mem_.size(); ++t) dbg_portal_[tm][t] |= portal_[t];
    }
    if (prev_len_ > 0 && L > prev_len_) {
      // label by the team's round-0 home over everything the team has seen (true portal-only)
      const int tm = spawn_.team & 1;
      std::vector<uint8_t> comp;
      Flood(*dbg_mem_[tm], dbg_portal_[tm], dbg_home_[tm], comp);
      const bool room = !comp[mem_.Tile(o.head_x(), o.head_y())];
      (room ? dbg_[spawn_.team & 1][1] : dbg_[spawn_.team & 1][2]) += L - prev_len_;
    }
    prev_len_ = L;
#endif
    clear_dir_ = -1;
    if (ping_round_ == o.round - 1 && ping_tile_ == mem_.Tile(o.head_x(), o.head_y()) && o.echo_present) {
      bool any = false;
      // ally, ally head, enemy, enemy head; ts8: only a head past the exit stops us (a
      // body somewhere down the ping's long line rarely matters)
      for (int k = 1; k < 5; ++k)
        if (!kNew || k == 2 || k == 4) any |= o.echo[k] != "0";
      if (!any) clear_dir_ = ping_dir_;
    }
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

    // 3. split off a 2-long child whenever length allows (not the late champion). Measured
    // from the real-opponent replays (25 games, M547322/M561834/M572141/M578650,
    // decode.our_team() confirms side): it never really consolidates -- longest stays flat
    // (mean ~4-6) from r25 to r200 while its dragon count keeps climbing (mean 7 -> 12 -> 17
    // -> 27 at r25/50/100/200), and it keeps splitting through r400+ (split-round histogram in
    // 50-round buckets: 464,659,751,793,870,786,598,459,304,235 -- only a mild tail-off, no
    // early freeze). Tried: pushing ts5's champion freeze/feed (kFreezeRound/kLateRound,
    // normally 330/350) out to 450 to track this. Reverted: on big_empty an even more
    // aggressive draft (freeze removed outright) blew CPU to 10ms/turn (many more surviving
    // short dragons paying full remembered-board search costs), and even the milder 450 version
    // measurably cost worldgen win rate vs trading-swarm-5 (18-0-22 vs the unmodified 20-0-20).
    // The dragon-count gap is a throughput/goal-selection issue upstream of this split logic
    // (per KNOWLEDGE.md and strategist-a's diagnosis), not something this consolidation knob
    // can close safely in this budget, so ts8 keeps ts5's 330/350 here.
    const bool champ = champ_x_ == o.head_x() && champ_y_ == o.head_y();
    // Opponent's split-parent-length tail extends past 8 more than ts5's rare roll allows for
    // (measured 4:3731, 5:836, 6:615, 7:207, 8:150, 9:68, 10:57, 11:51, 12:26, 13:27 across the
    // 25 real-opponent replays); trading-swarm-6 measured the same shape against a similar
    // opponent id and found bumping this roll 1/40 -> 1/25 a net worldgen win (24-0-16 vs its
    // parent) with no CPU cost (same code path, just more often), so ts8 carries that forward.
    const bool rare = L > 8 && o.round < 300 && std::uniform_int_distribution<int>(0, 24)(rng_) == 0;
    const bool late_champ = o.round >= kFreezeRound && champ;
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

    // 4. nearest remembered pearl (or ripening bed) over the remembered board, through
    //    remembered portal pairs; portal-only stores included
    const int here = mem_.Tile(o.head_x(), o.head_y());
    MemSearch(o, occ);
    {
      int best = -1, best_d = 1 << 30, bed = -1, bed_s = 1 << 30;
      for (int t = 0; t < mem_.size(); ++t) {
        const int dd = dist_[t];
        if (dd <= 0) continue;
        const strat::TileMemory& m = mem_.at(t);
        const int age = o.round - m.seen_round;
        const bool room = kNew && !home_comp_[t];   // a portal-only store
        if (m.pearl && age <= (room ? kRoomPearlAge : kPearlAge) && dd < best_d) best_d = dd, best = t;
        if (!m.pearl && m.countdown >= 0 && dd >= 2) {
          const int left = m.countdown - age;
          // portal-only beds: go from further, and earlier (farm them on their timers)
          const int slack = room ? kRoomBedSlack : 8;
          if (left >= -2 && dd <= (room ? kRoomBedDist : kBedDist) && left - dd <= slack &&
              left - dd - (room ? kRoomBedSlack - 8 : 0) < bed_s)
            bed_s = left - dd - (room ? kRoomBedSlack - 8 : 0), bed = t;
        }
      }
      if (best >= 0) {
        why_ = portal_path_[best] ? "pearl via portal" : "nearest pearl";
        return TakeFirst(o, best);
      }
      if (bed >= 0) {
        why_ = home_comp_[bed] ? "to bed" : "to portal bed";
        return TakeFirst(o, bed);
      }
    }

    // 5. no pearl known: cross a portal now and then; walk to a portal whose far end we
    //    have never seen and cross it (exploring)
    if (goal_ == 0 && inside && RoomBed(o)) {
      // farming a portal-only room: stay by its beds instead of wandering back out
      why_ = "farm room";
      return TakeFirst(o, room_bed_);
    }
    if (goal_ == 0) {
      const int pd = PortalDir(o, occ, true);
      if (pd >= 0 && o.round < 480) {
        why_ = "portal: explore";
        return Commit(o, pd);
      }
      const int kd = PortalDir(o, occ, false);
      if (kd >= 0 && (kNew ? (!inside || std::uniform_int_distribution<int>(0, 2)(rng_) != 0) : std::uniform_int_distribution<int>(0, 2)(rng_) != 0)) {
        why_ = "portal: random";
        return Commit(o, kd);
      }
      if (o.round < 480) {
        int best = -1, best_d = kExploreDist + 1;
        for (int t = 0; t < mem_.size(); ++t) {
          if (dist_[t] <= 0 || dist_[t] >= best_d || !portal_[t]) continue;
          for (int d = 0; d < 4; ++d)
            if (portal_[t] >> d & 1 && mem_.Step(t, d) == strat::kUnknown) { best = t, best_d = dist_[t]; break; }
        }
        if (best >= 0) {
          why_ = "to unknown portal";
          return TakeFirst(o, best);
        }
        // the real one lives in its portals: walk to a known portal (not the one just
        // used) and take it
        if (kNew && (!inside || std::uniform_int_distribution<int>(0, 1)(rng_) == 0)) {
          best = -1, best_d = kPortalWalk + 1;
          for (int t = 0; t < mem_.size(); ++t) {
            if (dist_[t] < 2 || dist_[t] >= best_d || !portal_[t] || portal_path_[t]) continue;
            if (o.round - last_portal_round_ <= 30 &&
                std::abs(TorusDelta(mem_.X(t), mem_.X(exit_tile_), spawn_.width)) +
                        std::abs(TorusDelta(mem_.Y(t), mem_.Y(exit_tile_), spawn_.height)) <= 3)
              continue;
            best = t, best_d = dist_[t];
          }
          if (best >= 0) {
            why_ = "to portal";
            return TakeFirst(o, best);
          }
        }
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
    if (nf == 0)   // no plain step: a portal whose far end is in view and free
      for (int d = 0; d < 4; ++d) {
        int idx;
        if (EdgeCode(o, win::kCenter, d, &idx) < 0) continue;
        if (const int t = mem_.Step(here, d); t >= 0 && Free(t)) {
          why_ = "portal: escape";
          return Commit(o, d);
        }
      }
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

  static constexpr int kRelayAge = 12, kLateRound = 350, kFreezeRound = 330;

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
  int PortalDir(const rl::Observation& o, const bool* occ, bool unknown_only) {
    int cand[4], nc = 0;
    const int W = rl::kWindow;
    for (int d = 0; d < 4; ++d) {
      int idx;
      const int id = EdgeCode(o, win::kCenter, d, &idx);
      if (id < 0 || d == (o.facing + 2) % 4) continue;
      if (unknown_only != (mem_.Step(mem_.Tile(o.head_x(), o.head_y()), d) == strat::kUnknown)) continue;
      if (unknown_only && d != clear_dir_) continue;   // only after a clean sonar check
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
    return cand[std::uniform_int_distribution<int>(0, nc - 1)(rng_)];
  }

  static constexpr int kPearlAge = 25, kMaxDist = 40, kExploreDist = kNew ? 30 : 16, kBedDist = 10;
  static constexpr int kRoomPearlAge = 60, kRoomBedDist = 30, kRoomBedSlack = 20, kPortalWalk = 10, kFarmSlack = 12;

  // tiles reachable from home_ over remembered plain edges (no portal, no unseen edge)
  void HomeComp() { Flood(mem_, portal_, home_, home_comp_); }
  static void Flood(const strat::BoardMemory& mem, const std::vector<uint8_t>& portal, int home,
                    std::vector<uint8_t>& comp) {
    comp.assign(mem.size(), 0);
    std::vector<int>& st = stack_;
    st.clear();
    st.push_back(home), comp[home] = 1;
    while (!st.empty()) {
      const int t = st.back();
      st.pop_back();
      for (int d = 0; d < 4; ++d) {
        if (portal[t] >> d & 1) continue;
        const int m = mem.Step(t, d);
        if (m < 0 || comp[m]) continue;
        comp[m] = 1;
        st.push_back(m);
      }
    }
  }

  // inside a room: the room bed (in our plain component, dist_ known) ripening soonest
  bool RoomBed(const rl::Observation& o) {
    room_bed_ = -1;
    int best = 1 << 30;
    for (int t = 0; t < mem_.size(); ++t) {
      if (dist_[t] < 1 || home_comp_[t] || portal_path_[t]) continue;
      const strat::TileMemory& m = mem_.at(t);
      if (m.countdown < 0) continue;
      const int left = std::max(0, m.countdown - (o.round - m.seen_round));
      const int s = std::max(left, dist_[t]) * 2 + dist_[t];
      if (s < best) best = s, room_bed_ = t, farm_ok_ = left <= dist_[t] + kFarmSlack;
    }
    return room_bed_ >= 0 && o.round < 490 && farm_ok_;
  }

  void Remember(const rl::Observation& o) {
    mem_.Update(o);
    const int W = rl::kWindow, hx = o.head_x() - 3, hy = o.head_y() - 3;
    for (int r = 0; r <= W; ++r)
      for (int c = 0; c < W; ++c)
        if (o.hedge[r * W + c] >= 0) {
          portal_[mem_.Tile(hx + c, hy + r)] |= 1;        // north side of (c, r)
          portal_[mem_.Tile(hx + c, hy + r - 1)] |= 4;    // south side of (c, r-1)
        }
    for (int r = 0; r < W; ++r)
      for (int c = 0; c <= W; ++c)
        if (o.vedge[r * (W + 1) + c] >= 0) {
          portal_[mem_.Tile(hx + c, hy + r)] |= 8;        // west side of (c, r)
          portal_[mem_.Tile(hx + c - 1, hy + r)] |= 2;    // east side of (c-1, r)
        }
  }

  bool Free(int t) const { return !(mem_.Current(t) && mem_.at(t).occupant >= 0); }

  // BFS over the remembered board from the head: dist_, first_ (first direction),
  // portal_path_ (the path crosses a portal). First steps through portals follow the
  // parent's safety rules.
  void MemSearch(const rl::Observation& o, const bool* occ) {
    const int n = mem_.size(), here = mem_.Tile(o.head_x(), o.head_y());
    dist_.assign(n, -1), first_.assign(n, -1), portal_path_.assign(n, 0);
    std::deque<int> q;
    dist_[here] = 0;
    for (int d = 0; d < 4; ++d) {
      const int t = mem_.Step(here, d);
      if (t < 0 || dist_[t] >= 0 || !Free(t)) continue;
      int idx;
      const int id = EdgeCode(o, win::kCenter, d, &idx);
      if (id >= 0) {
        if (d == (o.facing + 2) % 4) continue;
        if (id == last_portal_ && o.round - last_portal_round_ <= o.length + 1) continue;
      } else if (const int k = win::Step(o, win::kCenter, d); k < 0 || occ[k]) {
        continue;
      }
      dist_[t] = 1, first_[t] = d, portal_path_[t] = id >= 0;
      q.push_back(t);
    }
    while (!q.empty()) {
      const int t = q.front();
      q.pop_front();
      if (dist_[t] >= kMaxDist) continue;
      for (int d = 0; d < 4; ++d) {
        const int m = mem_.Step(t, d);
        if (m < 0 || dist_[m] >= 0 || !Free(m)) continue;
        dist_[m] = dist_[t] + 1, first_[m] = first_[t];
        portal_path_[m] = portal_path_[t] || (portal_[t] >> d & 1);
        q.push_back(m);
      }
    }
  }

  bot::Action Commit(const rl::Observation& o, int d) {
    int idx;
    const int id = EdgeCode(o, win::kCenter, d, &idx);
    if (id >= 0) {
#ifdef TS8_DEBUG
      ++dbg_[spawn_.team & 1][0];
#endif
      last_portal_ = id, last_portal_round_ = o.round;
      exit_tile_ = mem_.Step(mem_.Tile(o.head_x(), o.head_y()), d);
      if (exit_tile_ < 0) exit_tile_ = mem_.Tile(o.head_x(), o.head_y());
      if (why_[0] != 'p' || why_[1] != 'o') why_ = "portal: route";   // not "portal: explore"
    }
    return bot::Action::Move(d);
  }
  bot::Action TakeFirst(const rl::Observation& o, int t) { return Commit(o, first_[t]); }

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
  int ping_round_ = -1000, ping_tile_ = -1, ping_dir_ = -1, clear_dir_ = -1;
  int home_ = -1, room_bed_ = -1, exit_tile_ = 0;
  bool farm_ok_ = false;
  strat::BoardMemory mem_;
  std::vector<uint8_t> portal_;              // per tile: bit d set if side d is a portal
  // search scratch, shared by the agents of one process (one turn at a time)
  static inline thread_local std::vector<int> dist_, first_;
  static inline thread_local std::vector<uint8_t> portal_path_, home_comp_;
  static inline thread_local std::vector<int> stack_;
#ifdef TS8_DEBUG
  int prev_len_ = 0;
 public:
  static inline int dbg_[2][3] = {}, dbg_home_[2] = {}, dbg_last_[2] = {-1, -1};
  static inline strat::BoardMemory* dbg_mem_[2] = {};
  static inline std::vector<uint8_t> dbg_portal_[2];   // per team: crossings, pearls eaten in rooms, elsewhere
  struct DbgPrint {
    ~DbgPrint() {
      for (int t = 0; t < 2; ++t)
        std::fprintf(stderr, "TS8DBG team %d crossings %d room_pearls %d open_pearls %d\n", t, dbg_[t][0], dbg_[t][1], dbg_[t][2]);
    }
  };
  static inline DbgPrint dbg_print_;
#endif
};

}  // namespace ts8

LEAGUE_VARIANT(ts8::SwarmAgent)
