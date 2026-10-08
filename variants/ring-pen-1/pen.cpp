// g52-4 (ring-pen-1): the queen's pen cycle (planner-design 4.4, stage S4).
//
// Observed (REPORT section 4/8/9, server replays of ring-sprint-21): our queen is threat-free on
// 90-100% of r150-450 and enclosed 52-69% of it, yet she stays 2-3 long until r450: she circles a
// fixed 6-ring and splits back to 3 whenever she reaches 5. When both queens survive we lose the
// queen race 28-56; their queens are 26-30 at r450+ v our 12.
//
// The plan: from kPenFrom, while no hunter has had a free path to her within its reach + 2 for
// kPenQuiet rounds, she travels a closed cycle that contains her whole body. A body that follows a
// closed cycle longer than itself (plus what it eats meanwhile) can never run into itself, so she
// can grow there without the room tests that kept her short.
//  - Build: her body plus the shortest free return path from her head to beside her tail.
//  - Grow: square insertion. For a cycle edge a->b and free tiles c (next to a) and d (next to b)
//    that are next to each other, a->b becomes a->c->d->b. This keeps a valid cycle on any grid,
//    torus or portal graph (adjacency is board_.Step), and it only lengthens the cycle ahead of her
//    head, so it never makes her body less safe. Insertions prefer pearls, beds, tiles next to
//    pearls (feeder lines) and a compact coil; they avoid corridor tiles and enemy reach.
//    The cycle is kept at least L + 2 ceil(L/4) + 3 long: her body, one turn of free steps all
//    eating, and a margin (the design's L + ceil(L/4) + 2, plus the growth of the turn she moves).
//  - Move: free steps only, along the cycle. A step may take a shortcut to a cycle tile further
//    ahead that is next to her head (the safe-snake shortcut rule), and is taken only if following
//    the cycle from there never reaches a body segment before that segment has left, even if she
//    eats on each of her next ceil(L/4) + 1 steps. She heads for the nearest pearl on the cycle.
//  - Drop: a hunter (a visible enemy head with a free path to her within its reach + 2) clears the
//    cycle, and the existing queen code (QueenMove's escape/harvest beam, RingQueen) takes over;
//    after kPenQuiet quiet rounds she rebuilds from her body.
//  - Feeders: she broadcasts her cycle's box and her head (pen word 0xBD, relayed by workers); the
//    conveyor runs while it is heard instead of only from r430, and workers outside the box leave
//    its food to her (Ring5Reserved).
#include "strategy.hpp"

#include <algorithm>
#include <cstdio>
#include <deque>

namespace gm2 {

namespace {
constexpr uint64_t kPenTag = 0xBD;
}

// a visible enemy head with a free path to her head within its reach + 2 (the report's threat signal)
bool StrategyAgent::PenThreat() const {
  for (const auto& [id, e] : seen_) {
    if (e.team == spawn_.team || e.head < 0 || round_ - e.round > kPenMemory) continue;
    if (e.round == round_ && (!board_.Current(e.head) || board_.at(e.head).occupant != id || !board_.at(e.head).occupant_head)) continue;
    // the reach QueenRisk uses (free steps plus the length it can lunge), plus 2, grown by its free
    // speed for each round since it was seen (a head out of view now may be closer)
    const int age = round_ - e.round;
    const int reach = (e.partial ? 8 : std::min(8, std::max(3, (e.length + 3) / 4 + e.length - 2))) + 2 +
                      age * std::max(1, (e.length + 3) / 4);
    const auto paths = Search(board_, e.head, [&](int32_t t) { return t == head_ || !Occupied(t); }, reach);
    if (paths.dist[head_] >= 0 && paths.dist[head_] <= reach) return true;
  }
  return false;
}

void StrategyAgent::PenIndex() const {
  pen_idx_.Begin(board_.size());
  for (int i = 0; i < int(pen_.size()); ++i) pen_idx_.Set(pen_[i], i);
}


// her head is on the cycle (other dragons on it are met when she gets there: PenDetour)
bool StrategyAgent::PenValid() const {
  if (pen_.size() < 4 || int(body_.size()) != length_) return false;
  PenIndex();
  return pen_idx_.Has(head_);
}

// the cycle: a stretch of her body from the head back to segment m, closed by a free path from her
// head to a tile beside segment m. The segments behind m retract before she could come round to
// them and are not on the cycle. The longest such cycle (the whole body when her tail is in reach).
bool StrategyAgent::PenRebuild() {
  pen_.clear();
  if (int(body_.size()) != length_ || length_ < 2) return false;
  q_body_.Begin(board_.size());
  for (int i = 0; i < int(body_.size()); ++i) q_body_.Set(body_[i], i);
  auto free = [&](int32_t t) {
    return t >= 0 && board_.Current(t) && !q_body_.Has(t) && board_.at(t).occupant < 0;
  };
  q_dist_.Begin(board_.size());
  q_queue_.assign(1, head_);
  std::vector<int> dist(1, 0);
  q_dist_.Set(head_, -1);   // value: the parent tile
  int best_len = -1, best_m = -1;
  int32_t best_end = -1;
  for (size_t qi = 0; qi < q_queue_.size(); ++qi) {
    const int32_t u = q_queue_[qi];
    const int du = dist[qi];
    ++work_;
    // close here: u is next to segment m (m >= 3 when u is the head itself, so the cycle has 4+ tiles)
    for (int e = 0; e < 4; ++e) {
      const int32_t y = board_.Step(u, e);
      if (y < 0 || !q_body_.Has(y)) continue;
      const int m = q_body_.Get(y);
      if (m < 1 || (du == 0 && m < 3) || du + m + 1 < 4) continue;
      if (du + m + 1 > best_len || (du + m + 1 == best_len && m > best_m)) { best_len = du + m + 1; best_m = m; best_end = u; }
    }
    if (du >= kPenSearch) continue;
    for (int d = 0; d < 4; ++d) {
      const int32_t n = board_.Step(u, d);
      if (!free(n) || q_dist_.Has(n)) continue;
      q_dist_.Set(n, u);
      q_queue_.push_back(n);
      dist.push_back(du + 1);
    }
  }
  if (best_end < 0 || best_len > kPenMax) {
#ifdef PEN_DBG
    int fr = 0;
    for (int d = 0; d < 4; ++d) fr += free(board_.Step(head_, d));
    std::fprintf(stderr, "PENF r%d team%d len%d headfree%d reached%d\n", round_, spawn_.team, length_, fr, int(q_queue_.size()));
#endif
    return false;
  }
  std::vector<int32_t> path;
  for (int32_t t = best_end; t != head_; t = q_dist_.Get(t)) path.push_back(t);
  std::reverse(path.begin(), path.end());
  for (int i = best_m; i >= 0; --i) pen_.push_back(body_[i]);
  for (const int32_t t : path) pen_.push_back(t);
  PenIndex();
  return true;
}

// another dragon (or a hunter's reach) blocks the cycle ahead: replace the arc from her head to a
// later free cycle tile with a free path around it (only tiles ahead of her and not her body go)
bool StrategyAgent::PenDetour(const std::deque<int32_t>& body) {
  const int C = int(pen_.size());
  const int32_t h = body.front();
  const int pos = pen_idx_.Get(h);
  q_body_.Begin(board_.size());
  for (const int32_t b : body) q_body_.Set(b);
  // the arc ahead that may be dropped: up to the first body tile ahead (her tail end)
  int limit = C;
  for (int s = 1; s < C; ++s) if (q_body_.Has(pen_[(pos + s) % C])) { limit = s; break; }
  auto free = [&](int32_t t) {
    return t >= 0 && board_.Current(t) && !q_body_.Has(t) && board_.at(t).occupant < 0 && QueenRisk(t) == 0;
  };
  q_dist_.Begin(board_.size());
  q_queue_.assign(1, h);
  std::vector<int> dist(1, 0);
  q_dist_.Set(h, -1);
  int best_f = 1 << 30, best_du = 0;
  int32_t best_u = -1, best_j = -1;
  for (size_t qi = 0; qi < q_queue_.size(); ++qi) {
    const int32_t u = q_queue_[qi];
    const int du = dist[qi];
    ++work_;
    if (du > 0)
      for (int e = 0; e < 4; ++e) {
        const int32_t j = board_.Step(u, e);
        if (j < 0 || !pen_idx_.Has(j) || !free(j)) continue;
        const int f = ((pen_idx_.Get(j) - pos) % C + C) % C;
        if (f < 2 || f >= limit) continue;
        // keep the most of the cycle: the nearest rejoin ahead, then the shortest path
        if (f < best_f || (f == best_f && du < best_du)) { best_f = f; best_du = du; best_u = u; best_j = j; }
      }
    if (du >= kPenSearch) continue;
    for (int d = 0; d < 4; ++d) {
      const int32_t n = board_.Step(u, d);
      if (!free(n) || q_dist_.Has(n) || pen_idx_.Has(n)) continue;
      q_dist_.Set(n, u);
      q_queue_.push_back(n);
      dist.push_back(du + 1);
    }
  }
  if (best_u < 0) return false;
  std::vector<int32_t> path;
  for (int32_t t = best_u; t != h; t = q_dist_.Get(t)) path.push_back(t);
  std::reverse(path.begin(), path.end());
  std::vector<int32_t> cycle;
  for (int s = best_f; s != C; ++s) cycle.push_back(pen_[(pos + s) % C]);   // j ... round to her head
  cycle.push_back(h);
  for (const int32_t t : path) cycle.push_back(t);
  if (int(cycle.size()) > kPenMax) return false;
  // rotate so the order is the same travel direction: ..., h, path..., j, ...
  pen_ = std::move(cycle);
  PenIndex();
  return true;
}

// square insertions until the cycle holds `need` tiles, then more where they bring the cycle nearer
// a pearl in view (a feeder's line, a fresh bed pearl) or take in a bed, up to cap
int StrategyAgent::PenGrow(int need, int cap) {
  std::vector<int32_t> pearls;
  for (int32_t t = 0; t < board_.size(); ++t)
    if (board_.Current(t) && board_.at(t).pearl && board_.at(t).occupant < 0 && !pen_idx_.Has(t)) pearls.push_back(t);
  std::vector<int> pcd(pearls.size());
  auto usable = [&](int32_t t) {
    return t >= 0 && board_.Current(t) && board_.at(t).occupant < 0 && !pen_idx_.Has(t) && !Mine(t) && QueenRisk(t) == 0;
  };
  auto adjacent = [&](int32_t from, int32_t to) {
    for (int e = 0; e < 4; ++e) if (board_.Step(from, e) == to) return true;
    return false;
  };
  const int head_at = pen_idx_.Has(head_) ? pen_idx_.Get(head_) : 0;
  int inserted = 0;
  while (inserted < kPenInsert && int(pen_.size()) + 2 <= std::min(cap, kPenMax)) {
    const int C = int(pen_.size());
    // each pearl's distance to the cycle now
    for (size_t k = 0; k < pearls.size(); ++k) {
      int dmin = 1 << 20;
      if (pen_idx_.Has(pearls[k])) dmin = 0;
      else for (const int32_t t : pen_) dmin = std::min(dmin, Torus(t, pearls[k]));
      pcd[k] = dmin;
      work_ += C / 8;
    }
    double best = -1e30, best_gain = 0;
    int best_i = -1, best_beds = 0;
    int32_t best_c = -1, best_d = -1;
    for (int i = 0; i < C; ++i) {
      const int32_t a = pen_[i], b = pen_[(i + 1) % C];
      for (int n = 0; n < 4; ++n) {
        ++work_;
        const int32_t c = board_.Step(a, n);
        if (!usable(c)) continue;
        for (int m = 0; m < 4; ++m) {
          const int32_t d = board_.Step(c, m);
          if (d == a || !usable(d) || !adjacent(d, b)) continue;
          double gain = 0, score = 0;
          int beds = 0;
          for (size_t k = 0; k < pearls.size(); ++k) {
            const int dn = std::min(Torus(c, pearls[k]), Torus(d, pearls[k]));
            if (dn < pcd[k]) gain += double(pcd[k] - dn) / (1 + dn);
          }
          for (const int32_t x : {c, d}) {
            if (Corridor(x)) score -= 4;   // kelp on 2+ sides: a pocket that boxes in whoever else enters
            if (!board_.at(x).pearl && board_.at(x).countdown >= 0) ++beds;   // a bed: it fills while she circles
            for (int e = 0; e < 4; ++e) {   // a compact coil
              const int32_t y = board_.Step(x, e);
              if (y >= 0 && (pen_idx_.Has(y) || y == c || y == d)) score += 0.5;
            }
          }
          score += 10 * gain + 2 * beds - 0.05 * (((i - head_at) % C + C) % C);   // nearer her head first
          if (score > best) { best = score; best_gain = gain; best_beds = beds; best_i = i; best_c = c; best_d = d; }
        }
      }
    }
#ifdef PEN_DBG
    if (!pearls.empty() && inserted == 0) {
      int mn = 1 << 20; for (const int v : pcd) mn = std::min(mn, v);
      std::fprintf(stderr, "PENG r%d team%d pearls%d minpcd%d C%d need%d cap%d best_i%d gain%.2f\n", round_, spawn_.team, int(pearls.size()), mn, C, need, cap, best_i, best_gain);
    }
#endif
    if (best_i < 0) break;
    if (C >= need && best_gain <= 0 && best_beds == 0) break;   // beyond the minimum only towards food
    pen_.insert(pen_.begin() + best_i + 1, {best_c, best_d});
    PenIndex();
    ++inserted;
  }
  // an ear for a pearl no square reaches (beside a corner of the cycle, or past a blocked square):
  // edge a->b becomes a -> (free path) -> pearl -> (another free path) -> b. Like a square, it only
  // lengthens the cycle ahead of her head. The nearest pearls first, at most kPenEars a turn.
  for (int ears = 0; ears < kPenEars; ++ears) {
    int32_t pick = -1;
    int pick_d = 1 << 20;
    for (const int32_t p : pearls) {
#ifdef PEN_DBG
      if (round_ >= kPenDbgFrom && round_ <= kPenDbgTo)
        std::fprintf(stderr, "PENE r%d p%d,%d cyc%d cur%d occ%d mine%d risk%d\n", round_, board_.X(p) - board_.X(head_), board_.Y(p) - board_.Y(head_), int(pen_idx_.Has(p)), int(board_.Current(p)), board_.at(p).occupant, int(Mine(p)), QueenRisk(p));
#endif
      if (pen_idx_.Has(p) || !usable(p)) continue;
      int dmin = 1 << 20;
      for (const int32_t t : pen_) dmin = std::min(dmin, Torus(t, p));
      work_ += int(pen_.size()) / 8;
      if (dmin < pick_d) { pick_d = dmin; pick = p; }
    }
    if (pick < 0 || pick_d > kPenSearch) break;
    if (!PenEar(pick, std::min(cap, kPenMax))) break;
    ++inserted;
  }
  return inserted;
}

// one ear through `p` (see PenGrow): two free paths from p, disjoint but for p, to the two ends of
// one cycle edge; the shortest first path, then the shortest second one beside it
bool StrategyAgent::PenEar(int32_t p, int cap) {
  const int C = int(pen_.size());
  auto usable = [&](int32_t t) {
    return t >= 0 && board_.Current(t) && board_.at(t).occupant < 0 && !pen_idx_.Has(t) && !Mine(t) && QueenRisk(t) == 0;
  };
  // the first path: p to a tile beside some cycle tile a
  auto bfs = [&](int32_t from, const std::vector<int32_t>& banned, auto&& goal) -> std::vector<int32_t> {
    q_dist_.Begin(board_.size());
    q_queue_.assign(1, from);
    q_dist_.Set(from, -1);
    std::vector<int> dist(1, 0);
    for (size_t qi = 0; qi < q_queue_.size(); ++qi) {
      const int32_t u = q_queue_[qi];
      ++work_;
      if (goal(u)) {
        std::vector<int32_t> path;
        for (int32_t t = u; t != from; t = q_dist_.Get(t)) path.push_back(t);
        path.push_back(from);
        std::reverse(path.begin(), path.end());   // from ... u
        return path;
      }
      if (dist[qi] >= kPenSearch) continue;
      for (int d = 0; d < 4; ++d) {
        const int32_t n = board_.Step(u, d);
        if (!usable(n) || q_dist_.Has(n) || std::find(banned.begin(), banned.end(), n) != banned.end()) continue;
        q_dist_.Set(n, u);
        q_queue_.push_back(n);
        dist.push_back(dist[qi] + 1);
      }
    }
    return {};
  };
  auto beside = [&](int32_t u, int32_t c) {
    for (int e = 0; e < 4; ++e) if (board_.Step(u, e) == c) return true;
    return false;
  };
  // every cycle edge a->b near the pearl; the shortest ear wins
  std::vector<int32_t> best;
  int best_at = -1, best_cost = 0;
  const int head_at = pen_idx_.Has(head_) ? pen_idx_.Get(head_) : 0;
  for (int i = 0; i < C; ++i) {
    const int32_t a = pen_[i], b = pen_[(i + 1) % C];
    if (Torus(a, p) > kPenEarReach && Torus(b, p) > kPenEarReach) continue;
    const auto first = bfs(p, {}, [&](int32_t u) { return beside(u, a); });
    if (first.empty()) continue;
    std::vector<int32_t> ear(first.rbegin(), first.rend());   // a's neighbour ... p
    if (!(first.size() == 1 && beside(p, b))) {
      std::vector<int32_t> banned(first.begin() + 1, first.end());
      const auto second = bfs(p, banned, [&](int32_t u) { return u != p && beside(u, b); });
      if (second.empty()) continue;
      ear.insert(ear.end(), second.begin() + 1, second.end());   // ... b's neighbour
    }
    // the ear she reaches soonest: its length plus her cycle distance to it (an ear right ahead of
    // her head takes her to the pearl now, before workers or the bed's next tick take it)
    const int cost = int(ear.size()) + ((i - head_at) % C + C) % C;
    if (best_at < 0 || cost < best_cost) { best = ear; best_at = i; best_cost = cost; }
  }
#ifdef PEN_DBG
  if (round_ >= kPenDbgFrom && round_ <= kPenDbgTo) std::fprintf(stderr, "PENE r%d ear at%d size%zu C%d cap%d\n", round_, best_at, best.size(), C, cap);
#endif
  if (best_at < 0 || C + int(best.size()) > cap) return false;
  pen_.insert(pen_.begin() + best_at + 1, best.begin(), best.end());
  PenIndex();
  return true;
}

uint64_t StrategyAgent::PenWord() const {
  // the cycle's box, relative to her head on the torus (it may wrap)
  const int W = board_.width(), H = board_.height();
  const int hx = board_.X(head_), hy = board_.Y(head_);
  int x0 = 0, x1 = 0, y0 = 0, y1 = 0;
  for (const int32_t t : pen_) {
    int dx = board_.X(t) - hx, dy = board_.Y(t) - hy;
    if (dx > W / 2) dx -= W; if (dx < -(W - 1) / 2) dx += W;
    if (dy > H / 2) dy -= H; if (dy < -(H - 1) / 2) dy += H;
    x0 = std::min(x0, dx); x1 = std::max(x1, dx); y0 = std::min(y0, dy); y1 = std::max(y1, dy);
  }
  const uint64_t bx = uint64_t(((hx + x0) % W + W) % W) & 63, by = uint64_t(((hy + y0) % H + H) % H) & 63;
  const uint64_t bw = uint64_t(std::min(63, x1 - x0 + 1)), bh = uint64_t(std::min(63, y1 - y0 + 1));
  const int32_t k = EnemyQueenKnown();
  const uint64_t eq = k >= 0 && k > enemy_queen_dead_ ? (uint64_t(1) << 45) | (uint64_t(k & 511) << 46) : 0;
  return (kPenTag << 56) | (uint64_t(spawn_.team & 1) << 55) | eq | (uint64_t(round_ & 511) << 36) |
         (uint64_t(hx & 63) << 24) | (uint64_t(hy & 63) << 30) | bx | (by << 6) | (bw << 12) | (bh << 18);
}

void StrategyAgent::ReadPen(const rl::Observation& o) {
  if (!PEN_MODE) return;
  for (const uint64_t w : o.inbox) {
    if ((w >> 56) != kPenTag || int((w >> 55) & 1) != (spawn_.team & 1)) continue;
    const int stamp = int((w >> 36) & 511), age = (round_ - stamp) & 511;
    if (age > kPenFresh || round_ - age <= pen_stamp_) continue;
    const int bx = int(w & 63), by = int((w >> 6) & 63), bw = int((w >> 12) & 63), bh = int((w >> 18) & 63);
    const int hx = int((w >> 24) & 63), hy = int((w >> 30) & 63);
    if (bx >= board_.width() || by >= board_.height() || hx >= board_.width() || hy >= board_.height() || bw < 1 || bh < 1) continue;
    pen_stamp_ = round_ - age;
    pen_word_ = w;
    pen_bx_ = bx; pen_by_ = by; pen_bw_ = bw; pen_bh_ = bh;
    pen_qhead_ = board_.Tile(hx, hy);
    // the last enemy-queen sighting, as the ring word relays it (strategy.cpp, Decide)
    if (((w >> 45) & 1) && int32_t((w >> 46) & 511) <= round_) enemy_queen_heard_ = std::max(enemy_queen_heard_, int32_t((w >> 46) & 511));
  }
}

// a worker outside her pen's box leaves the box to her: it neither paths into it nor values its food
bool StrategyAgent::PenReserved(int t) const {
  if (!PenHeard() || IsQueen() || t < 0) return false;
  auto inside = [&](int32_t x) {
    const int dx = ((board_.X(x) - pen_bx_) % board_.width() + board_.width()) % board_.width();
    const int dy = ((board_.Y(x) - pen_by_) % board_.height() + board_.height()) % board_.height();
    return dx < pen_bw_ && dy < pen_bh_;
  };
  return inside(t) && !inside(head_);
}

bot::Action StrategyAgent::PenQueen() {
  if (!PEN_MODE || round_ < kPenFrom - kPenQuiet) return {};
  const bool threat = PenThreat();
  if (threat) pen_threat_ = round_;
  if (round_ < kPenFrom) return {};
  if (threat) {
#ifdef PEN_DBG
    if (pen_on_) std::fprintf(stderr, "PEN r%d team%d len%d drop hunter\n", round_, spawn_.team, length_);
#endif
    pen_on_ = false;
    pen_.clear();
    return {};
  }
  if (int(body_.size()) != length_) { pen_.clear(); return {}; }
  if (!pen_on_) {
    if (round_ - pen_threat_ < kPenQuiet) return {};
    if (OpenRoom(head_, std::vector<int32_t>(body_.begin(), body_.end()), length_ + kPenRoom) < length_ + kPenRoom) return {};
    if (!PenRebuild()) return {};
    pen_on_ = true;
    ring_.clear();   // she leaves her 6-ring: the cycle replaces it (and its broadcast)
#ifdef PEN_DBG
    std::fprintf(stderr, "PEN r%d team%d len%d start C%d\n", round_, spawn_.team, length_, int(pen_.size()));
#endif
  } else if (!PenValid()) {
    // another dragon crossed the cycle, or a fallback turn moved her off it: rebuild from her body
    if (!PenRebuild()) {
#ifdef PEN_DBG
      std::fprintf(stderr, "PEN r%d team%d len%d rebuild failed\n", round_, spawn_.team, length_);
#endif
      return {};
    }
#ifdef PEN_DBG
    std::fprintf(stderr, "PEN r%d team%d len%d rebuild C%d\n", round_, spawn_.team, length_, int(pen_.size()));
#endif
  }
#ifdef PEN_DBG
  if (round_ >= kPenDbgFrom && round_ <= kPenDbgTo) {
    std::fprintf(stderr, "PENMAP r%d team%d len%d C%d\n", round_, spawn_.team, length_, int(pen_.size()));
    for (int dy = -4; dy <= 4; ++dy) {
      char row[16] = {};
      for (int dx = -4; dx <= 4; ++dx) {
        const int32_t t = board_.Tile(((board_.X(head_) + dx) % board_.width() + board_.width()) % board_.width(),
                                      ((board_.Y(head_) + dy) % board_.height() + board_.height()) % board_.height());
        char c = '.';
        if (!board_.Current(t)) c = ' ';
        const bool cyc = pen_idx_.Has(t);
        if (t == head_) c = 'H';
        else if (Mine(t)) c = cyc ? 'b' : 'B';
        else if (board_.Current(t) && board_.at(t).occupant >= 0) c = board_.at(t).occupant_team == spawn_.team ? 'a' : 'e';
        else if (board_.Current(t) && board_.at(t).pearl) c = cyc ? '*' : 'o';
        else if (cyc) c = '#';
        row[dx + 4] = c;
      }
      std::fprintf(stderr, "PENMAP   %s\n", row);
    }
  }
#endif
  const int F = (length_ + 3) / 4;
  // the design's L + ceil(L/4) + 2, plus one more turn of free steps all eating (F + 1)
  const int need = length_ + 2 * F + 3;
  const int cap = std::max(need + 16, 3 * length_ + 12);
  const int grown = PenGrow(need, cap);
  (void)grown;
  auto fwd = [&](int from, int to) { const int C = int(pen_.size()); return ((to - from) % C + C) % C; };
  std::deque<int32_t> b(body_.begin(), body_.end());
  std::vector<int32_t> eaten;
  auto has_pearl = [&](int32_t t) {
    return board_.Current(t) && board_.at(t).pearl && std::find(eaten.begin(), eaten.end(), t) == eaten.end();
  };
  bot::Action a;
  int shortcuts = 0, detours = 0, unsafe = 0, had_target = 0;
  for (int k = 0; k < F; ++k) {
    const int32_t h = b.front();
    int best_dir = -1;
    for (int attempt = 0; attempt < 2 && best_dir < 0; ++attempt) {
      if (attempt == 1 && !PenDetour(b)) break;   // blocked ahead: go round it, then try again
      if (attempt == 1) ++detours;
      const int C = int(pen_.size());
      const int pos = pen_idx_.Get(h);
      // the next pearl on the cycle ahead of her (in view)
      int target = -1;
      for (int s = 1; s < C; ++s) if (has_pearl(pen_[(pos + s) % C])) { target = (pos + s) % C; break; }
      if (k == 0 && attempt == 0) had_target = target >= 0;
      if (k > 0 && target < 0) break;   // nothing to eat: one step a turn is enough
      int best_key = 1 << 30;
      for (int margin_try = 0; margin_try < 2 && best_dir < 0; ++margin_try) {
        // the design's margin: she may eat on each of her next ceil(L/4) + 1 steps; on the first
        // step only, if nothing keeps that margin, the exact no-collision test (margin 0)
        const int g = margin_try == 0 ? F + 1 : 0;
        if (margin_try == 1 && k > 0) break;
        for (int d = 0; d < 4; ++d) {
          const int32_t n = board_.Step(h, d);
          if (n < 0 || !pen_idx_.Has(n) || !board_.Current(n)) continue;
          if (board_.at(n).occupant >= 0 && board_.at(n).occupant != spawn_.id) continue;
          if (std::find(b.begin(), b.end(), n) != b.end()) continue;   // her own body, tail included
          if (QueenRisk(n) > 0) continue;
          const int j = pen_idx_.Get(n);
          const int fd = fwd(pos, j);
          if (fd == 0) continue;
          std::deque<int32_t> nb = b;   // her body after this step
          nb.push_front(n);
          if (!has_pearl(n)) nb.pop_back();
          // following the cycle from j, segment s (head 0) leaves after len - s more non-eating
          // steps and the head reaches its tile after fwd(j, its index) steps; up to g eats meanwhile
          bool safe = true;
          const int len = int(nb.size());
          for (int s = 1; s < len && safe; ++s) {
            ++work_;
            if (!pen_idx_.Has(nb[s])) continue;
            if (fwd(j, pen_idx_.Get(nb[s])) < len - s + 1 + g) safe = false;
          }
          if (!safe) { ++unsafe; continue; }
          // the nearest pearl ahead first (a shortcut must not jump past it); else the plain next tile
          const int key = target >= 0 ? (fwd(j, target) <= fwd(pos, target) ? fwd(j, target) : 1 << 20) : fd;
          if (key < best_key) { best_key = key; best_dir = d; }
        }
      }
    }
    if (best_dir < 0) break;
    const int32_t n = board_.Step(h, best_dir);
    if (fwd(pen_idx_.Get(h), pen_idx_.Get(n)) > 1) ++shortcuts;
    const bool eat = has_pearl(n);
    b.push_front(n);
    if (eat) eaten.push_back(n);
    else b.pop_back();
    a.steps.push_back(best_dir);
  }
  const int C = int(pen_.size());
#ifdef PEN_DBG
  {
    int off = 0;
    for (int32_t t = 0; t < board_.size(); ++t) off += board_.Current(t) && board_.at(t).pearl && !pen_idx_.Has(t) && board_.at(t).occupant < 0;
    std::fprintf(stderr, "PENX r%d team%d len%d C%d off%d target%d unsafe%d steps%d eat%d\n", round_, spawn_.team, length_, int(pen_.size()), off, had_target, unsafe, int(a.steps.size()), int(eaten.size()));
  }
  std::fprintf(stderr, "PEN r%d team%d len%d C%d steps%d eat%d short%d grown%d detour%d\n", round_, spawn_.team, length_, C,
               int(a.steps.size()), int(eaten.size()), shortcuts, grown, detours);
#endif
  if (a.steps.empty()) {
    // no safe step on the cycle this turn: the existing queen code moves her; rebuild next turn
    pen_.clear();
    return {};
  }
  body_.assign(b.begin(), b.end());
  return Because(eaten.empty() ? (shortcuts ? "pen: shortcut" : "pen: circle") : "pen: eat", a);
}

}  // namespace gm2
