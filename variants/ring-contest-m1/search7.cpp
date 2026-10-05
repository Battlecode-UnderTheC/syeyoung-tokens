// (copied from variants/arena-search-7/search.cpp for gst-search-3's late game)
// arena-search-1: every dragon plans by exhaustive lookahead over its own next moves on a
// model of the rules (own body simulated segment by segment, other bodies retracting on a
// schedule, pearls eaten and growth, enemy lunge reach as a risk field), with a potential
// field over remembered pearls, ripening beds and the unexplored frontier at the leaves and a
// time-aware flood fill (tail-following room) as the survival test.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <deque>
#include <set>
#include <random>
#include <string>
#include <vector>

#include "board_memory.hpp"
#include "plugin.hpp"

namespace {

constexpr int kDepth = 8;
constexpr double kGamma = 0.9;
constexpr int kInf = 1 << 28;
constexpr int kSplitUntil = 360;
constexpr int kSmallTeam = 14;
constexpr int kFeedFrom = 380;      // non-champions walk to the champion and die beside its head
constexpr int kSonarFrom = 250;     // start agreeing on a champion
constexpr uint64_t kTag = 0xA5;
constexpr uint64_t kFoodTag = 0xA6;
constexpr int kUnseenRoom = 3;
constexpr double kStale = 0.35;
constexpr bool kFoodSonar = true;
constexpr double kGoalPull = 2.0;
constexpr bool kSeal = true;
constexpr int kFieldReach = 30;
constexpr bool kAdversary = true;   // enemy replies and our trades by who ends up ahead
constexpr double kReplyP = 0.25;     // chance an enemy takes a trade that favours it
constexpr double kTradeMargin = 0.0;
constexpr int kRemainsRadius = 5;
constexpr bool kSqueeze = true;          // a late champion moves on rather than splitting its bulk away
constexpr bool kPretenderGate = true;    // only length >= kChampMin names itself champion
constexpr int kChampMin = 6;
constexpr bool kDeepReply = false;       // trade-value replies at moves 1-2 too
constexpr double kDeepP[3] = {0.0, 0.25, 0.1};
constexpr bool kOddFeed = false;
constexpr bool kSplitNeedsCalm = false;

// food report: tag 8 | x 7 | y 7 | richness 8 (10 x pearls per dragon in view) | round 9
struct Food {
  int x = 0, y = 0, rich = 0, round = -1000;
  double Score(int now) const { return rich - 2.0 * (now - round); }
};
inline uint64_t PackFood(const Food& f) {
  return kFoodTag << 56 | uint64_t(f.x & 0x7F) << 49 | uint64_t(f.y & 0x7F) << 42 | uint64_t(std::min(f.rich, 255)) << 34 |
         uint64_t(f.round & 0x1FF) << 25;
}
inline bool UnpackFood(uint64_t w, Food& f) {
  if ((w >> 56) != kFoodTag) return false;
  f.x = int(w >> 49 & 0x7F);
  f.y = int(w >> 42 & 0x7F);
  f.rich = int(w >> 34 & 0xFF);
  f.round = int(w >> 25 & 0x1FF);
  return true;
}

// champion report: tag 8 | id 12 | length 10 | x 7 | y 7 | round 9
struct Report {
  int id = -1, len = 0, x = 0, y = 0, round = -1000;
  double Score(int now) const { return id < 0 ? -1e9 : len - 0.5 * (now - round); }
};
inline uint64_t Pack(const Report& r) {
  return kTag << 56 | uint64_t(r.id & 0xFFF) << 44 | uint64_t(std::min(r.len, 1023)) << 34 | uint64_t(r.x & 0x7F) << 27 |
         uint64_t(r.y & 0x7F) << 20 | uint64_t(r.round & 0x1FF) << 11;
}
inline bool Unpack(uint64_t w, Report& r) {
  if ((w >> 56) != kTag) return false;
  r.id = int(w >> 44 & 0xFFF);
  r.len = int(w >> 34 & 0x3FF);
  r.x = int(w >> 27 & 0x7F);
  r.y = int(w >> 20 & 0x7F);
  r.round = int(w >> 11 & 0x1FF);
  return true;
}

// discount tables (pow is the hottest libm call otherwise)
struct PowTable {
  double v[1024];
  explicit PowTable(double base) { v[0] = 1; for (int i = 1; i < 1024; ++i) v[i] = v[i - 1] * base; }
  double operator()(int i) const { return i <= 0 ? 1.0 : i >= 1024 ? 0.0 : v[i]; }
};
const PowTable kG(kGamma), kAge(0.985), kFar(0.97);

struct Params {
  double pearl = 1.0, bed = 0.8, explore = 0.35, stale = 0.25;
  double risk_p[kDepth + 1] = {0.05, 0.02, 0.01, 0, 0, 0, 0, 0, 0};
  double death = -1000.0;
};

struct Enemy {
  int id, head, len;
};

class SearchAgent : public bot::Agent {
 public:
  explicit SearchAgent(const rl::Spawn& s) : id_(s.id), team_(s.team), mem_(s.width, s.height) {
    rng_.seed(static_cast<uint32_t>(s.id * 7919 + s.team * 104729 + 17));
    const int n = s.width * s.height;
    free_at_.assign(n, 0);
    body_idx_.assign(n, -1);
    field_.assign(n, 0.0);
    field_tgt_.assign(n, -1);
    field2_.assign(n, 0.0);
    field2_tgt_.assign(n, -1);
    thr_m_.assign(n, kInf);
    thr_len_.assign(n, 0);
    stamp_.assign(n, 0);
    distbuf_.assign(n, -1);
    pearl_.assign(n, 0);
    bed_next_.assign(n, -1);
    hot_near_.assign(n, 0);
    odist_.assign(n, 1 << 28);
    bed_gap_.assign(n, 0);
    bed_seen_.assign(n, 0);
    bed_last_.assign(n, -100);
    unseen_cache_round_.assign(n, -1);
    unseen_cache_id_.assign(n, -1);
    unseen_cache_.assign(n, 0.0);
  }

  bot::Action Turn(const rl::Observation& o) override {
    mem_.Update(o);
    ++cache_turn_;
    const int N = mem_.size();
    const int head = mem_.Tile(o.head_x(), o.head_y());
    len_ = o.length;
    units_ = o.unit_count;
    // champion: the best of our own report, what we heard, and what we knew
    {
      Report self{id_, len_, o.head_x(), o.head_y(), round_};
      if (champ_.id == id_) champ_ = self;
      for (uint64_t w : o.inbox) {
        Food f;
        if (UnpackFood(w, f) && f.round <= round_ && f.Score(round_) > food_.Score(round_)) food_ = f;
        Report r;
        if (!Unpack(w, r) || r.round > round_) continue;
        if (r.id == champ_.id && r.round >= champ_.round) champ_ = r;
        else if (r.Score(round_) > champ_.Score(round_)) champ_ = r;
      }
      // only a dragon of some size may call itself champion (fewer pretenders)
      const bool eligible = !kPretenderGate || len_ >= kChampMin;
      if (eligible && self.Score(round_) >= champ_.Score(round_)) champ_ = self;
      if (round_ - champ_.round > 40) champ_ = eligible ? self : Report{};
    }
    round_ = o.round;
    BuildAdjacency(o);
    TrackBody(o, head);

    // occupancy by others and their retraction schedule
    for (int t : free_set_) free_at_[t] = 0;
    free_set_.clear();
    enemies_.clear();
    ally_heads_.clear();
    {
      // group visible segments by dragon id, in order (head first)
      std::vector<std::pair<int, std::vector<int>>> groups;
      std::vector<uint8_t> gteam;
      for (const rl::Segment& s : o.segments) {
        if (s.id == id_) continue;
        const int t = mem_.Tile(s.x, s.y);
        if (groups.empty() || groups.back().first != s.id) {
          groups.push_back({s.id, {}});
          gteam.push_back(s.team);
        }
        groups.back().second.push_back(t);
        (void)s.head;
      }
      for (size_t g = 0; g < groups.size(); ++g) {
        auto& segs = groups[g].second;
        const int L = static_cast<int>(segs.size());
        // is the head visible? the first listed segment is the head only if flagged
        bool head_seen = false;
        for (const rl::Segment& s : o.segments)
          if (s.id == groups[g].first && s.head) head_seen = true;
        // whether the whole body is in view: we can't be sure; tails near the window edge are
        // treated as continuing (never free)
        const bool tail_edge = Edge(o, segs.back());
        for (int i = 0; i < L; ++i) {
          const int t = segs[i];
          int fa = tail_edge ? kInf : L - i;   // head: frees after L moves (optimistic w/o eating)
          if (head_seen && i == 0) fa = std::max(fa, 1);
          free_at_[t] = std::max(free_at_[t], fa);
          free_set_.push_back(t);
        }
        if (head_seen) {
          if (gteam[g] != team_) enemies_.push_back({groups[g].first, segs[0], tail_edge ? std::max(L, 3) : L});
          else ally_heads_.push_back(segs[0]);
        }
      }
    }
    for (int t : pearl_set_) pearl_[t] = 0;
    pearl_set_.clear();
    for (int k = 0; k < rl::kWindowTiles; ++k)
    {
      const int t = mem_.Tile(o.tile_x[k], o.tile_y[k]);
      if (o.pearl[k]) { pearl_[t] = 1; pearl_set_.push_back(t); }
      const int c = o.countdown[k];
      if (c >= 0 && bed_last_[t] != round_) {
        bed_next_[t] = round_ + std::max(c, 1);
        bed_gap_[t] = std::max(bed_gap_[t], c);
        ++bed_seen_[t];
        bed_last_[t] = round_;
      }
    }

    for (int k = 0; k < rl::kWindowTiles; ++k) {
      const int t = mem_.Tile(o.tile_x[k], o.tile_y[k]);
      if (bed_next_[t] < 0) continue;
      int c = 0;
      for (int dy = -2; dy <= 2; ++dy)
        for (int dx = -2 + std::abs(dy); dx <= 2 - std::abs(dy); ++dx) {
          const int u = mem_.Tile(mem_.X(t) + dx, mem_.Y(t) + dy);
          if (u != t && bed_next_[u] >= 0 && GapEst(u) <= 3) ++c;
        }
      hot_near_[t] = c;
    }
    feeding_ = round_ >= FeedFrom() && champ_.id != id_ && len_ < 0.8 * champ_.len && round_ - champ_.round <= 30;
    champ_tile_ = feeding_ ? mem_.Tile(champ_.x, champ_.y) : -1;
    BuildOtherHeads();
    BuildThreat();
    UpdateGoal(head);
    BuildField(head);

    // search each first step
    best_ = -1e18;
    int best_dir = -1;
    double vals[4];
    alpha_ = -1e18;
    acc_ = 0;
    for (int k = 0; k < 4; ++k) {
      const int d = (o.facing + k) % 4;   // straight on first: a good line early prunes more
      path_len_ = 0;
      neaten_ = 0;
      vals[d] = Step(head, d, 0, len_, 1.0) + (d == o.facing ? 0.01 : 0.0);
      if (vals[d] > best_) {
        best_ = vals[d];
        best_dir = d;
      }
    }

    // guard: never step onto a tile a segment of ours is seen on while a clear step exists
    auto own_seen = [&](int t) {
      for (const rl::Segment& sg : o.segments)
        if (sg.id == id_ && mem_.Tile(sg.x, sg.y) == t) return true;
      return false;
    };
    if (best_dir >= 0 && (nb_[head * 4 + best_dir] >= 0 && own_seen(nb_[head * 4 + best_dir]))) {
      double alt = -1e18;
      int alt_dir = -1;
      for (int d = 0; d < 4; ++d) {
        const int n = nb_[head * 4 + d];
        if (n == -1 || (n >= 0 && (own_seen(n) || free_at_[n] > 0))) continue;
        if (vals[d] > alt) { alt = vals[d]; alt_dir = d; }
      }
      if (alt_dir >= 0) { best_dir = alt_dir; best_ = alt; }
    }
    bot::Action act = bot::Action::Move(best_dir < 0 ? o.facing : best_dir);
    why_ = "search";

    // trades: lunge onto a visible enemy head at least as long as us
    if (auto trade = Trade(head); !trade.empty()) {
      act = bot::Action::Move(trade[0]);
      act.steps = trade;
      why_ = "trade";
    } else {
      const bool can_split = o.unit_count < o.spawn.unit_limit;
      const bool doomed = best_ < -500;
      int split_c = 0;
      if (can_split && len_ >= 4 && best_ < -1.0 && static_cast<int>(body_list_.size()) == len_ &&
          round_ < SplitUntil() && !(round_ >= SplitUntil() - 60 && champ_.id == id_)) {
        // boxed in: search both halves of each possible split and compare with moving on
        auto score = [](double v, int L) { return v < -500 ? -3.0 * (L + 5) : v; };
        double best_split = score(best_, len_);
        const std::vector<int> body(body_list_.begin(), body_list_.end());
        for (int c : {2, len_ - 2}) {
          std::vector<int> parent(body.begin(), body.end() - c), child(body.rbegin(), body.rbegin() + c);
          const double v = score(EvalBody(parent, child), len_ - c) + score(EvalBody(child, parent), c);
          if (v > best_split + 0.5) {
            best_split = v;
            split_c = c;
          }
          if (len_ - 2 == 2) break;
        }
      }
      if (split_c > 0) {
        act = bot::Action::Split(split_c);
        why_ = "searched split";
      } else if (doomed && can_split && len_ >= 4 &&
                 !(kSqueeze && champ_.id == id_ && round_ >= SplitUntil() - 60 && best_ > P.death + 50)) {
        // (a late champion with any step that doesn't die at once squeezes on instead: a split
        // would hand its bulk to a new id)
        act = bot::Action::Split(len_ - 2);
        why_ = "trapped split";
      } else if (can_split && len_ >= 4 && round_ < SplitUntil() && !(round_ >= SplitUntil() - 60 && champ_.id == id_) && (!kSplitNeedsCalm || thr_m_[head] > 1) && best_ > -50 && SplitSafe()) {
        act = bot::Action::Split(2);
        why_ = "split";
      }
    }
    // endgame: feed the champion by dying beside its head
    if (feeding_ && act.kind == bot::Action::kMove && why_ != std::string("trade")) {
      for (const rl::Segment& sg : o.segments)
        if (sg.id == champ_.id && sg.head) {
          const int ct = mem_.Tile(sg.x, sg.y);
          int dx = std::abs(sg.x - o.head_x()), dy = std::abs(sg.y - o.head_y());
          dx = std::min(dx, mem_.width() - dx);
          dy = std::min(dy, mem_.height() - dy);
          (void)ct;
          if (dx + dy <= 3 && (!kOddFeed || len_ % 2 == 1 || len_ == 2 || round_ >= 470)) {
            act = bot::Action::Resign();
            why_ = "feed";
          }
        }
    }
    if (kFoodSonar && round_ < kSonarFrom) {
      // report the food around us (per dragon here), or relay the best report we heard
      int pearls = 0, allies = 1;
      for (int k = 0; k < rl::kWindowTiles; ++k) {
        const int t = mem_.Tile(o.tile_x[k], o.tile_y[k]);
        pearls += o.pearl[k];
        if (bed_next_[t] >= 0 && GapEst(t) <= 3) ++pearls;
      }
      allies += static_cast<int>(ally_heads_.size());
      Food mine{o.head_x(), o.head_y(), std::min(255, 10 * pearls / allies), round_};
      const Food& send = mine.Score(round_) >= food_.Score(round_) ? mine : food_;
      if (send.rich > 0)
        for (int d = 0; d < 4; ++d) act.sonar[d] = PackFood(send);
    }
    if (round_ >= kSonarFrom && champ_.id >= 0) {
      const uint64_t w = Pack(champ_);
      for (int d = 0; d < 4; ++d) act.sonar[d] = w;
    }
    // remember what we did, to extend our body next turn
    planned_.clear();
    if (act.kind == bot::Action::kMove) {
      int at = head;
      for (int d : act.steps) {
        const int n = nb_[at * 4 + d];
        if (n < 0) break;
        planned_.push_back(n);
        at = n;
      }
    }
    return act;
  }
  const char* reason() const override { return why_; }

  // consolidation timed by team size: a small team consolidates early, a big swarm keeps eating
  bool SmallTeam() const { return round_ >= 280 && units_ < kSmallTeam; }
  int SplitUntil() const { return SmallTeam() ? 320 : kSplitUntil; }
  int FeedFrom() const { return SmallTeam() ? 340 : kFeedFrom; }

 private:
  bool Edge(const rl::Observation& o, int t) const {
    const int hx = o.head_x(), hy = o.head_y(), W = mem_.width(), H = mem_.height();
    int dx = std::abs(mem_.X(t) - hx), dy = std::abs(mem_.Y(t) - hy);
    dx = std::min(dx, W - dx);
    dy = std::min(dy, H - dy);
    return std::max(dx, dy) >= dragons::kVisionRadius;
  }

  // Our step table follows BoardMemory::Step. Only the window (and the ring just outside it)
  // can change in a turn, except when a new portal end shows up: then every portal side is
  // redone (portal sides are rare, and they're kept in a list with their exits for Preds).
  void SetStep(int t) {
    for (int d = 0; d < 4; ++d) {
      const int s = mem_.Step(t, d);
      nb_[t * 4 + d] = s == strat::kBlocked ? -1 : s == strat::kUnknown ? -2 : s;
    }
  }
  void BuildAdjacency(const rl::Observation& o) {
    const int N = mem_.size(), W = rl::kWindow;
    bool new_portal = nb_.empty();
    auto note = [&](bool h, int x, int y, int code) {
      if (code < 0) return;
      const long long key = (static_cast<long long>(code) << 20) | (h ? 1 << 19 : 0) | mem_.Tile(x, y);
      if (portal_keys_.insert(key).second) new_portal = true;
    };
    const int hx = o.head_x(), hy = o.head_y(), R = dragons::kVisionRadius;
    for (int r = 0; r <= W; ++r)
      for (int c = 0; c < W; ++c) note(true, hx - R + c, hy - R + r, o.hedge[r * W + c]);
    for (int r = 0; r < W; ++r)
      for (int c = 0; c <= W; ++c) note(false, hx - R + c, hy - R + r, o.vedge[r * (W + 1) + c]);
    if (new_portal) {
      nb_.assign(N * 4, -2);
      if (plain_.empty()) {
        plain_.resize(N * 4);
        for (int t = 0; t < N; ++t)
          for (int d = 0; d < 4; ++d) plain_[t * 4 + d] = mem_.Tile(mem_.X(t) + dragons::kDx[d], mem_.Y(t) + dragons::kDy[d]);
      }
      // only tiles we have seen, and the ring around them, can have a known side
      for (int t = 0; t < N; ++t)
        if (mem_.at(t).seen_round >= 0) {
          SetStep(t);
          for (int d = 0; d < 4; ++d) SetStep(plain_[t * 4 + d]);
        }
      portal_pred_.clear();
      for (int t = 0; t < N; ++t)
        for (int d = 0; d < 4; ++d) {
          const int n = nb_[t * 4 + d];
          if (n >= 0 && n != plain_[t * 4 + d]) portal_pred_.push_back({n, t});
        }
      std::sort(portal_pred_.begin(), portal_pred_.end());
      return;
    }
    for (int dy = -R - 1; dy <= R + 1; ++dy)
      for (int dx = -R - 1; dx <= R + 1; ++dx) SetStep(mem_.Tile(hx + dx, hy + dy));
  }

  // tiles whose step lands on t
  int Preds(int t, int* out) const {
    int n = 0;
    for (int d = 0; d < 4; ++d) {
      const int p = plain_[t * 4 + (d + 2) % 4];   // the tile behind t, seen from direction d
      if (nb_[p * 4 + d] == t) out[n++] = p;
    }
    if (!portal_pred_.empty()) {
      auto it = std::lower_bound(portal_pred_.begin(), portal_pred_.end(), std::pair<int, int>{t, -1});
      for (; it != portal_pred_.end() && it->first == t && n < 8; ++it) out[n++] = it->second;
    }
    return n;
  }

  void TrackBody(const rl::Observation& o, int head) {
    std::vector<int> seen;
    for (const rl::Segment& s : o.segments)
      if (s.id == id_) seen.push_back(mem_.Tile(s.x, s.y));
    std::deque<int> nbody;
    if (static_cast<int>(seen.size()) == len_) {
      nbody.assign(seen.begin(), seen.end());
    } else {
      nbody = body_;
      if (!planned_.empty() && planned_.back() == head) {
        for (int t : planned_) nbody.push_front(t);
      } else if (nbody.empty() || nbody.front() != head) {
        nbody.push_front(head);
      }
      // merge in the visible prefix exactly
      if (nbody.empty()) nbody.assign(seen.begin(), seen.end());
    }
    while (static_cast<int>(nbody.size()) > len_) nbody.pop_back();
    body_ = nbody;
    for (int t : body_list_) body_idx_[t] = -1;
    body_list_.assign(body_.begin(), body_.end());
    for (int i = static_cast<int>(body_list_.size()) - 1; i >= 0; --i) body_idx_[body_list_[i]] = i;
    for (int t : seen)
      if (body_idx_[t] < 0) {   // visible but unexplained: treat as a tail segment
        body_idx_[t] = len_ - 1;
        body_list_.push_back(t);
      }
  }

  // my body at time t (after t of my planned moves) covers tile x?
  bool Mine(int x, int t, int Lt) const {
    const int k = std::min(t, Lt);
    for (int i = 0; i < k; ++i)
      if (path_[t - 1 - i] == x) return true;
    const int j = body_idx_[x];
    return j >= 0 && j < Lt - t;
  }

  // enemy reach field: thr_m[x] = min over enemies of (steps to x - lunge reach)
  // how soon any other head (ally or enemy) could stand on each tile, walking
  void BuildOtherHeads() {
    for (int t : odist_set_) odist_[t] = kInf;
    std::vector<int>& q = odist_set_;
    q.clear();
    for (const Enemy& e : enemies_) { odist_[e.head] = 0; q.push_back(e.head); }
    for (int a : ally_heads_) { odist_[a] = 0; q.push_back(a); }
    for (size_t qi = 0; qi < q.size(); ++qi) {
      const int t = q[qi], dt = odist_[t];
      if (dt >= kDepth + 16) continue;
      for (int d = 0; d < 4; ++d) {
        const int n = nb_[t * 4 + d];
        if (n < 0 || odist_[n] <= dt + 1) continue;
        if (free_at_[n] > dt + 1) continue;
        if (body_idx_[n] >= 0) continue;
        odist_[n] = dt + 1;
        q.push_back(n);
      }
    }
  }

  void BuildThreat() {
    for (int t : thr_set_) thr_m_[t] = kInf;
    thr_set_.clear();
    for (const Enemy& e : enemies_) {
      const int reach = std::max(1, e.len - 1);
      const int limit = reach + kDepth + 1;
      ++stamp_id_;
      std::vector<int> q{e.head};
      stamp_[e.head] = stamp_id_;
      distbuf_[e.head] = 0;
      for (size_t qi = 0; qi < q.size(); ++qi) {
        const int t = q[qi], dt = distbuf_[t];
        const int m = dt - reach;
        if (t != e.head && (m < thr_m_[t] || (m == thr_m_[t] && e.len < thr_len_[t]))) {
          if (thr_m_[t] == kInf) thr_set_.push_back(t);
          thr_m_[t] = m;
          thr_len_[t] = e.len;
        }
        if (dt >= limit) continue;
        for (int d = 0; d < 4; ++d) {
          const int n = nb_[t * 4 + d];
          if (n < 0 || stamp_[n] == stamp_id_) continue;
          if (free_at_[n] > dt + 1 && n != e.head) continue;
          stamp_[n] = stamp_id_;
          distbuf_[n] = dt + 1;
          q.push_back(n);
        }
      }
    }
  }

  // potential field over the best targets (pearls, beds, frontier), by reverse BFS
  void BuildField(int head) {
    const int N = mem_.size();
    // forward BFS from head over non-body tiles
    ++stamp_id_;
    std::vector<int> q{head};
    stamp_[head] = stamp_id_;
    distbuf_[head] = 0;
    struct Cand { double v; int t; int eta; double base; };
    std::vector<Cand> cands;
    for (size_t qi = 0; qi < q.size(); ++qi) {
      const int t = q[qi], dt = distbuf_[t];
      if (dt > kFieldReach) break;
      const double base = TargetValue(t, dt);
      if (base > 0 && t != head) {
        cands.push_back({base * kG(dt), t, dt, base});
      }
      for (int d = 0; d < 4; ++d) {
        const int n = nb_[t * 4 + d];
        if (n < 0 || stamp_[n] == stamp_id_) continue;
        if (free_at_[n] > dt + 1) continue;
        if (body_idx_[n] >= 0 && body_idx_[n] < len_ - dt - 1) continue;
        stamp_[n] = stamp_id_;
        distbuf_[n] = dt + 1;
        q.push_back(n);
      }
    }
    for (int t : field_touched_) {
      field_[t] = 0.0;
      field_tgt_[t] = -1;
      field2_[t] = 0.0;
      field2_tgt_[t] = -1;
    }
    field_touched_.clear();
    // ally claim: targets an ally head in view is clearly nearer to count 0.3
    int used = 0;
    for (Cand& c : cands) c.base = ally_heads_.empty() ? 1.0 : Claimed(c.t, head) ? 0.3 : 1.0;
    if (champ_tile_ >= 0 && champ_tile_ != head) cands.insert(cands.begin(), Cand{1e9, champ_tile_, 0, 1.0});
    food_tile_ = -1;
    if (kFoodSonar && champ_tile_ < 0 && round_ - food_.round <= 25 && food_.rich >= 20) {
      const int ft = mem_.Tile(food_.x, food_.y);
      const int W = mem_.width(), H = mem_.height();
      int dx = std::abs(food_.x - mem_.X(head)), dy = std::abs(food_.y - mem_.Y(head));
      if (std::min(dx, W - dx) + std::min(dy, H - dy) > 4) {
        food_tile_ = ft;
        food_value_ = std::min(1.5, 0.05 * food_.rich);
        cands.insert(cands.begin(), Cand{1e8, ft, 0, 1.0});
      }
    }
    {
      auto better = [](const Cand& a, const Cand& b) { return a.v * a.base > b.v * b.base; };
      const size_t k = std::min<size_t>(12, cands.size());
      std::partial_sort(cands.begin(), cands.begin() + k, cands.end(), better);
    }
    for (const Cand& c : cands) {
      if (used >= 12) break;
      ++used;
      ++stamp_id_;
      std::vector<int>& rq = rq_;
      rq.clear();
      rq.push_back(c.t);
      stamp_[c.t] = stamp_id_;
      distbuf_[c.t] = 0;
      const bool timed = bed_next_[c.t] >= 0 && c.t != champ_tile_ && c.t != food_tile_;   // only beds depend on dt
      const double flat = timed ? 0.0 : c.base * TargetValue(c.t, 0);
      for (size_t qi = 0; qi < rq.size(); ++qi) {
        const int t = rq[qi], dt = distbuf_[t];
        const double v = (timed ? c.base * TargetValue(c.t, dt) : flat) * kG(dt);
        if (field_tgt_[t] < 0 && field2_tgt_[t] < 0) field_touched_.push_back(t);
        if (v <= field2_[t] && field_tgt_[t] != c.t && qi > 0) continue;   // two better targets here already
        if (v > field_[t]) {
          if (field_tgt_[t] != c.t) {
            field2_[t] = field_[t];
            field2_tgt_[t] = field_tgt_[t];
          }
          field_[t] = v;
          field_tgt_[t] = c.t;
        } else if (v > field2_[t] && field_tgt_[t] != c.t) {
          field2_[t] = v;
          field2_tgt_[t] = c.t;
        }
        if (dt > kFieldReach || v < 1e-3) continue;
        int preds[8], np = Preds(t, preds);
        for (int i = 0; i < np; ++i) {
          const int p = preds[i];
          if (stamp_[p] == stamp_id_) continue;
          if (free_at_[p] >= kInf) continue;
          stamp_[p] = stamp_id_;
          distbuf_[p] = dt + 1;
          rq.push_back(p);
        }
      }
    }
    (void)N;
  }

  bool Claimed(int t, int head) const {
    const int W = mem_.width(), H = mem_.height();
    auto man = [&](int a, int b) {
      int dx = std::abs(mem_.X(a) - mem_.X(b)), dy = std::abs(mem_.Y(a) - mem_.Y(b));
      return std::min(dx, W - dx) + std::min(dy, H - dy);
    };
    const int mine = man(head, t);
    for (int a : ally_heads_)
      if (man(a, t) + 1 < mine) return true;
    return false;
  }

  // value of target tile t reached in `dist` steps (before discounting)
  int TorusDist(int a, int b) const {
    const int W = mem_.width(), H = mem_.height();
    int dx = std::abs(mem_.X(a) - mem_.X(b)), dy = std::abs(mem_.Y(a) - mem_.Y(b));
    return std::min(dx, W - dx) + std::min(dy, H - dy);
  }

  // a committed exploration goal: an unexplored region far enough to be worth the walk, kept until
  // seen or stale, so that explorers stop dithering between equal nearby frontiers
  void UpdateGoal(int head) {
    if (goal_ >= 0 && (mem_.at(goal_).seen_round >= 0 || round_ - goal_set_ > 40)) goal_ = -1;
    if (goal_ >= 0) return;
    const int W = mem_.width(), H = mem_.height();
    goal_span_ = (W + H) / 2.0;
    double best = 0;
    for (int y = 1; y < H; y += 3)
      for (int x = 1; x < W; x += 3) {
        const int t = mem_.Tile(x, y);
        if (mem_.at(t).seen_round >= 0) continue;
        int unseen = 0;
        for (int dy = -2; dy <= 2; ++dy)
          for (int dx = -2; dx <= 2; ++dx) unseen += mem_.at(mem_.Tile(x + dx, y + dy)).seen_round < 0;
        const int d = TorusDist(head, t);
        if (d < 6) continue;
        const double score = unseen / 25.0 * kFar(d) * (0.5 + std::uniform_real_distribution<double>(0, 1)(rng_));
        if (score > best) { best = score; goal_ = t; }
      }
    goal_set_ = round_;
  }

  double TargetValue(int t, int dist) {
    if (t == champ_tile_) return 5.0;
    if (t == food_tile_) return food_value_;
    const strat::TileMemory& m = mem_.at(t);
    if (m.seen_round < 0) {
      if (unseen_cache_round_[t] == round_ && unseen_cache_id_[t] == cache_turn_) return unseen_cache_[t];
      // unexplored: worth more where more of the surroundings is unseen, and toward the centre early
      const int x = mem_.X(t), y = mem_.Y(t);
      int unseen = 0;
      for (int dy = -3; dy <= 3; ++dy)
        for (int dx = -3; dx <= 3; ++dx)
          if (mem_.at(mem_.Tile(x + dx, y + dy)).seen_round < 0) ++unseen;
      double v = P.explore * (0.3 + 0.7 * unseen / 49.0);
      if (round_ < 80) {
        const int W = mem_.width(), H = mem_.height();
        const double cx = std::abs(x - W / 2.0) / (W / 2.0), cy = std::abs(y - H / 2.0) / (H / 2.0);
        v *= 1.0 + 0.6 * (1.0 - 0.5 * (cx + cy));
      }
      if (goal_ >= 0) v *= 1.0 + kGoalPull * (1.0 - std::min(1.0, TorusDist(t, goal_) / goal_span_));
      unseen_cache_round_[t] = round_;
      unseen_cache_id_[t] = cache_turn_;
      unseen_cache_[t] = v;
      return v;
    }
    const int age = round_ - m.seen_round;
    if (m.pearl) return P.pearl * kAge(age);
    double v = 0;
    if (bed_next_[t] >= 0) {
      const int g = GapEst(t);
      int f = bed_next_[t];
      if (f < round_) f += ((round_ - f + g - 1) / g) * g;
      const int wait = std::max(0, f - round_ - dist);
      const double base = g <= 3 ? P.pearl * (1.0 + 0.25 * hot_near_[t]) : P.bed;
      v = std::max(v, base * kG(2 * wait));
    }
    // frontier: a known tile with an unknown side
    for (int d = 0; d < 4; ++d)
      if (nb_[t * 4 + d] == -2) {
        v = std::max(v, P.explore);
        break;
      }
    if (age >= 6) v = std::max(v, kStale * std::min(1.0, (age - 6) / 30.0));
    return v;
  }

  // depth-first over my moves; returns discounted value of the best continuation
  double Step(int from, int d, int t, int Lt, double disc) {
    const int to = nb_[from * 4 + d];
    if (to == -1) return P.death + 100.0 * t;
    if (to == -2) {   // unknown portal: an exploration leaf with some risk
      if (static_cast<int>(body_list_.size()) < len_) return P.death + 100.0 * t + 50.0;   // our lost segments may be there
      return disc * (P.explore * 1.2) - disc * 0.3 * Lt;
    }
    // collision checks at move t
    if (Mine(to, t, Lt)) return P.death + 100.0 * t;
    if (free_at_[to] > t) {
      bool enemy_head = false;
      if (t == 0)
        for (const Enemy& e : enemies_)
          if (e.head == to) enemy_head = true;
      (void)enemy_head;
      return P.death + 100.0 * t;
    }
    double gain = 0;
    bool ate = false;
    if (PearlAt(to, t)) {
      ate = true;
      gain += disc * P.pearl;
      eaten_t_[neaten_] = t;
      eaten_[neaten_++] = to;
    }
    // a blind step (a portal exit out of view): an unseen head or body may sit there
    // risk: an enemy can bring its head onto ours before our next move
    if (kAdversary && t == 0) {
      path_[0] = to;
      gain -= ReplyCost(to, Lt + (ate ? 1 : 0));
    } else if (thr_m_[to] <= t) {
      const int L2 = Lt + (ate ? 1 : 0);
      if (kDeepReply && t <= 2) {
        // a later reply: an enemy that can get there trades if it comes out ahead
        Enemy e{-1, to, thr_len_[to]};
        const double delta = TradeDelta(to, L2, L2, e);
        if (delta < 0) gain -= disc * kDeepP[t] * -delta;
      } else {
        const double cost = std::max(0.5, L2 - thr_len_[to] + 1.0);
        gain -= disc * P.risk_p[t] * cost;
      }
    }
    path_[t] = to;
    const int L2 = Lt + (ate ? 1 : 0);
    if (t == 0 && kSeal) gain += SealBonus(to, L2);
    double best;
    if (t + 1 >= kDepth) {
      // the room test only lowers a leaf, so a leaf that can't beat the best line so far skips it
      const double pot = LeafPotential(to);
      const double bound = gain + disc * kGamma * pot;
      if (acc_ + bound <= alpha_) {
        best = bound;
      } else {
        best = gain + disc * kGamma * (pot - RoomPenalty(to, t + 1, L2));
        alpha_ = std::max(alpha_, acc_ + best);
      }
    } else {
      best = -1e18;
      acc_ += gain;
      for (int k = 0; k < 4; ++k) {
        const int nd = (d + (k == 0 ? 0 : k == 1 ? 1 : k == 2 ? 3 : 2)) % 4;   // straight on first
        if (nd == (d + 2) % 4) continue;
        const double v = Step(to, nd, t + 1, L2, disc * kGamma);
        best = std::max(best, v);
      }
      acc_ -= gain;
      best += gain;
    }
    if (ate) --neaten_;
    return best;
  }

  // best line value for a dragon with body `segs` (head first) while `other` stands as a body
  // that retracts as it moves
  double EvalBody(const std::vector<int>& segs, const std::vector<int>& other) {
    const std::vector<int> saved_list = body_list_;
    for (int t : body_list_) body_idx_[t] = -1;
    body_list_ = segs;
    for (int i = static_cast<int>(segs.size()) - 1; i >= 0; --i) body_idx_[segs[i]] = i;
    std::vector<int> saved_free(other.size());
    for (size_t i = 0; i < other.size(); ++i) {
      saved_free[i] = free_at_[other[i]];
      free_at_[other[i]] = std::max(free_at_[other[i]], static_cast<int>(other.size() - i));
    }
    double best = -1e18;
    const int L = static_cast<int>(segs.size());
    const double saved_alpha = alpha_;
    alpha_ = -1e18;
    acc_ = 0;
    for (int d = 0; d < 4; ++d) {
      neaten_ = 0;
      best = std::max(best, Step(segs[0], d, 0, L, 1.0));
    }
    alpha_ = saved_alpha;
    for (size_t i = other.size(); i-- > 0;) free_at_[other[i]] = saved_free[i];
    for (int t : body_list_) body_idx_[t] = -1;
    body_list_ = saved_list;
    for (int i = static_cast<int>(body_list_.size()) - 1; i >= 0; --i) body_idx_[body_list_[i]] = i;
    return best;
  }

  // after our first move to `to`: enemy heads left with (almost) no room die on their own
  double SealBonus(int to, int L1) {
    double bonus = 0;
    for (const Enemy& e : enemies_) {
      if (TorusDist(e.head, to) > 4) continue;
      // flood from the enemy head over tiles free after our move (their next move is 1 step)
      ++stamp_id_;
      room_q_.clear();
      room_q_.push_back(e.head);
      stamp_[e.head] = stamp_id_;
      distbuf_[e.head] = 0;
      int count = 0;
      bool head_adjacent = false;
      for (size_t qi = 0; qi < room_q_.size() && count < 6; ++qi) {
        const int x = room_q_[qi], dx = distbuf_[x];
        for (int d = 0; d < 4; ++d) {
          const int n = nb_[x * 4 + d];
          if (n == -2) { count += 3; continue; }
          if (n < 0 || stamp_[n] == stamp_id_) continue;
          if (n == to) { if (dx == 0) head_adjacent = true; continue; }
          if (free_at_[n] > 1 + dx) continue;
          if (MineAt(n, 1, L1, dx)) continue;
          stamp_[n] = stamp_id_;
          distbuf_[n] = dx + 1;
          room_q_.push_back(n);
          ++count;
        }
      }
      if (count == 0 && !head_adjacent) bonus += 1.0 + e.len;          // it must crash
      else if (count == 0) bonus += 0.3 * std::max(0, e.len - L1);     // its only way out is a trade
      else if (count < std::min(e.len, 4)) bonus += 0.25 * e.len;       // boxed in
    }
    return bonus;
  }

  // A trade seen from our side: what they lose minus what we lose, plus the remains (every
  // other segment of both bodies) split by who has more heads near the collision to eat them.
  double TradeDelta(int at, int our_len, int our_len_at_impact, const Enemy& e) const {
    int ours = 0, theirs = 0;
    for (int a : ally_heads_)
      if (TorusDist(a, at) <= kRemainsRadius) ++ours;
    for (const Enemy& o : enemies_)
      if (o.id != e.id && TorusDist(o.head, at) <= kRemainsRadius) ++theirs;
    const double share = (ours + 0.5) / (ours + theirs + 1.0);
    const double remains = (our_len_at_impact + 1) / 2 + (e.len + 1) / 2;
    return e.len - our_len + remains * (2.0 * share - 1.0);
  }

  // The enemy's best answer to our first move: any enemy head that can reach our new head this
  // coming turn trades into it when that trade is good for them.
  double ReplyCost(int to, int L1) {
    double cost = 0;
    for (const Enemy& e : enemies_) {
      const int reach = std::min(e.len - 1, 8);
      if (reach < 1 || TorusDist(e.head, to) > reach) continue;
      // their lunge: a BFS from their head over tiles free once we have moved
      ++stamp_id_;
      room_q_.clear();
      room_q_.push_back(e.head);
      stamp_[e.head] = stamp_id_;
      distbuf_[e.head] = 0;
      int hit = -1;
      for (size_t qi = 0; qi < room_q_.size() && hit < 0; ++qi) {
        const int x = room_q_[qi], dx = distbuf_[x];
        if (dx >= reach) continue;
        for (int d = 0; d < 4; ++d) {
          const int n = nb_[x * 4 + d];
          if (n < 0 || stamp_[n] == stamp_id_) continue;
          if (n == to) { hit = dx + 1; break; }
          if (free_at_[n] > 0 || Mine(n, 1, L1)) continue;
          stamp_[n] = stamp_id_;
          distbuf_[n] = dx + 1;
          room_q_.push_back(n);
        }
      }
      if (hit < 0) continue;
      const double delta = TradeDelta(to, L1, L1, e);   // our view of their trade
      if (delta < 0) cost = std::max(cost, kReplyP * -delta);
    }
    return cost;
  }

  // will a pearl be on x when we step there at our move t? Pearls seen now (or recently)
  // count unless eaten earlier on this line; beds fire at their known next round, then every
  // (estimated) gap, whether or not we ate there
  bool PearlAt(int x, int t) const {
    int last = -1;
    for (int i = 0; i < neaten_; ++i)
      if (eaten_[i] == x) last = eaten_t_[i];
    if (last < 0 && (pearl_[x] || (mem_.at(x).pearl && mem_.at(x).seen_round >= round_ - 10))) return true;
    if (bed_next_[x] < 0) return false;
    const int R = round_ + t, E = last < 0 ? round_ - 1 : round_ + last;
    if (last < 0 && !pearl_[x] && mem_.at(x).seen_round == round_ && bed_next_[x] <= round_) return false;
    int f = bed_next_[x];
    const int g = GapEst(x);
    if (f <= E) f += ((E + 1 - f + g - 1) / g) * g;
    return f <= R;
  }
  int GapEst(int x) const { return bed_seen_[x] >= 2 ? std::max(1, bed_gap_[x]) : std::max(10, bed_gap_[x]); }

  double LeafPotential(int head) const {
    const int t1 = field_tgt_[head], t2 = field2_tgt_[head];
    auto eaten = [&](int x) {
      for (int i = 0; i < neaten_; ++i)
        if (eaten_[i] == x) return true;
      return false;
    };
    if (t1 >= 0 && !eaten(t1)) return field_[head];
    if (t2 >= 0 && !eaten(t2)) return field2_[head];
    return 0.0;
  }
  double RoomPenalty(int head, int t, int Lt) {
    const int need = std::min(Lt + 1, 14);
    const int room = Room(head, t, Lt, need);
    return room < need ? 3.0 + 2.0 * (need - room) : 0.0;
  }

  int Room(int head, int t, int Lt, int need) {
    ++stamp_id_;
    room_q_.clear();
    room_q_.push_back(head);
    stamp_[head] = stamp_id_;
    distbuf_[head] = 0;
    int count = 0;
    for (size_t qi = 0; qi < room_q_.size(); ++qi) {
      const int x = room_q_[qi], dx = distbuf_[x];
      for (int d = 0; d < 4; ++d) {
        const int n = nb_[x * 4 + d];
        if (n == -2) {   // unknown ground: worth a few tiles, not a sure escape
          count += kUnseenRoom;
          if (count >= need) return count;
          continue;
        }
        if (n < 0 || stamp_[n] == stamp_id_) continue;
        if (free_at_[n] > t + dx) continue;
        if (MineAt(n, t, Lt, dx)) continue;
        if (odist_[n] <= t + dx) continue;   // another head can get there first
        stamp_[n] = stamp_id_;
        distbuf_[n] = dx + 1;
        room_q_.push_back(n);
        if (++count >= need) return count;
      }
    }
    return count;
  }

  // my body (as of time t, length Lt) still covering x after `extra` more moves?
  bool MineAt(int x, int t, int Lt, int extra) const {
    // body sequence: path[t-1..0], body[0..]; element i frees after Lt - i moves
    const int k = std::min(t, Lt);
    for (int i = 0; i < k; ++i)
      if (path_[t - 1 - i] == x) return i < Lt - extra;
    const int j = body_idx_[x];
    if (j < 0) return false;
    const int i = t + j;
    return i < Lt - extra;
  }

  bool SplitSafe() const {
    // the child (our tail) must have a free step
    if (body_.size() < 2) return false;
    const int tail = body_.back();
    for (int d = 0; d < 4; ++d) {
      const int n = nb_[tail * 4 + d];
      if (n >= 0 && free_at_[n] == 0 && body_idx_[n] < 0) return true;
    }
    return false;
  }

  // shortest free path onto an enemy head we'd gladly trade with
  std::vector<int> Trade(int head) {
    const int reach = len_ - 1;
    if (reach < 1) return {};
    ++stamp_id_;
    std::vector<int> q{head};
    std::vector<std::pair<int, int>> prev;   // per queue index: parent index, dir
    prev.push_back({-1, -1});
    stamp_[head] = stamp_id_;
    distbuf_[head] = 0;
    for (size_t qi = 0; qi < q.size(); ++qi) {
      const int x = q[qi], dx = distbuf_[x];
      if (dx >= reach) continue;
      for (int d = 0; d < 4; ++d) {
        const int n = nb_[x * 4 + d];
        if (n < 0 || stamp_[n] == stamp_id_) continue;
        for (const Enemy& e : enemies_)
          if (e.head == n && dx + 1 <= len_ - 1 &&
              (kAdversary ? TradeDelta(n, len_, len_ - dx, e) > kTradeMargin : (e.len > len_ || (e.len == len_ && units_ >= 4)))) {
            std::vector<int> fwd;
            for (int i = static_cast<int>(qi); prev[i].first >= 0; i = prev[i].first) fwd.push_back(prev[i].second);
            std::reverse(fwd.begin(), fwd.end());
            fwd.push_back(d);
            return fwd;
          }
        if (free_at_[n] > 0) continue;
        if (body_idx_[n] >= 0) continue;   // a pearl on the way would keep our tail in place
        stamp_[n] = stamp_id_;
        distbuf_[n] = dx + 1;
        q.push_back(n);
        prev.push_back({static_cast<int>(qi), d});
      }
    }
    return {};
  }

  Params P;
  int id_, team_;
  strat::BoardMemory mem_;
  int len_ = 0, round_ = 0, units_ = 0;
  std::vector<int> nb_, rq_, field_touched_;
  std::vector<std::pair<int, int>> portal_pred_;
  std::vector<int> plain_;
  std::set<long long> portal_keys_;
  std::deque<int> body_;
  std::vector<int> body_list_, body_idx_, planned_;
  std::vector<int> free_at_;
  std::vector<Enemy> enemies_;
  std::vector<int> ally_heads_;
  std::vector<double> field_, field2_;
  std::vector<int> field_tgt_, field2_tgt_;
  std::vector<int> thr_m_, thr_len_;
  std::vector<int> stamp_, distbuf_, room_q_;
  std::vector<uint8_t> pearl_;
  std::vector<int> pearl_set_, free_set_, odist_set_, thr_set_;
  int stamp_id_ = 0, cache_turn_ = 0;
  std::vector<int> unseen_cache_round_, unseen_cache_id_;
  std::vector<double> unseen_cache_;
  int path_[kDepth + 1];
  int eaten_[kDepth + 1], eaten_t_[kDepth + 1];
  std::vector<int> hot_near_, odist_;
  std::vector<int> bed_next_, bed_gap_, bed_seen_, bed_last_;
  int neaten_ = 0, path_len_ = 0;
  double alpha_ = -1e18, acc_ = 0;
  double best_ = 0;
  const char* why_ = "";
  Report champ_;
  bool feeding_ = false;
  int champ_tile_ = -1, food_tile_ = -1;
  double food_value_ = 0;
  Food food_;
  int goal_ = -1, goal_set_ = 0;
  double goal_span_ = 1;
  std::mt19937 rng_{12345};
};

}  // namespace

namespace gstsearch8 {
// arena-search-7's per-dragon lookahead agent, for the hybrid's late game
bot::Agent* MakeSearch7(const rl::Spawn& s) { return new SearchAgent(s); }
}  // namespace gstsearch8
