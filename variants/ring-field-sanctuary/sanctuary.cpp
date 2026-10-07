// g49-2 (ring-field-sanctuary): the queen moves her ring to a sanctuary on repeated contact
// (strategist g49 #2).
//
// Observed cause (arena-r23): 70 P3 losses where our queen died after the enemy queen was already
// dead. For the last 50-100 rounds she had an enemy head 1-5 tiles away at most snapshots with no
// ally near (224832 r240-300, 223741 r230-300, 224598 r260-330): the front came to her ring, and
// she stayed, circling her 2x3 beside it. Knowing the enemy queen's death can't trigger this (9 of
// 20 died by resign, unseen), so the trigger is what she sees herself: an enemy head in view in
// kContacts of the last kWindow rounds.
//
// Then, before kUntil (r430: from there the queen race / ring exit owns her) and while she is not
// growing (the race, the conveyor, the opening forage), she picks the known open tile within
// kReach steps (no corridor tile, QueenSpace >= her need, room for her 2x3 ring) farthest from
// every enemy head she saw in the last kMemory rounds, plus a bonus per ally head seen or reported
// within kWindow rounds near it. She moves only if it is >= kGain tiles farther from the enemy than
// where she is. She walks there on tiles out of every visible enemy head's reach (QueenRisk 0), with
// free steps only, queen_home_ moves there, and her ring is rebuilt there. kCooldown rounds between
// moves keep her from shuttling.
#include "strategy.hpp"

#include <algorithm>
#include <cstdio>
#include <unordered_map>

namespace gm2 {
namespace {
constexpr int kWindow = 20;      // contact window (strategist: "≥4 of the last 20")
constexpr int kContacts = 4;     // a passing head is 1-2 rounds in view; 4 means it stays near
constexpr int kMemory = 40;      // enemy heads that count as the front: seen in the last 40 rounds
constexpr int kUntil = 430;      // the queen race (Ring4Exit, conveyor) starts at r430
constexpr int kCooldown = 30;    // rounds between re-anchors
constexpr int kGain = 4;         // the new tile must be this many tiles farther from the front
constexpr int kReach = 14;       // path length searched: ~10 rounds at 1-2 free steps a turn
constexpr int kTimeout = 24;     // a walk that hasn't arrived by then is dropped
constexpr int kDistCap = 24;     // farther than this from every head is all the same
constexpr int kAllyNear = 4;     // an ally head within this many tiles of the sanctuary helps
constexpr int kAllyMax = 3;      // at most this many allies count
}  // namespace

void StrategyAgent::SanctuaryNote() {
  bool contact = false;
  for (const auto& [id, e] : seen_) {
    if (e.team == spawn_.team || e.round != round_ || e.head < 0 || !board_.Current(e.head) ||
        board_.at(e.head).occupant != id || !board_.at(e.head).occupant_head) continue;
    contact = true;
    sanct_enemy_.push_back({round_, e.head});
  }
  if (contact) sanct_contact_.push_back(round_);
  while (!sanct_contact_.empty() && sanct_contact_.front() <= round_ - kWindow) sanct_contact_.pop_front();
  sanct_enemy_.erase(std::remove_if(sanct_enemy_.begin(), sanct_enemy_.end(),
                                    [&](const auto& p) { return p.first <= round_ - kMemory; }),
                     sanct_enemy_.end());
}

// A 2x3 or 3x2 loop through t whose tiles are known, not kelp, and joined by open edges.
bool StrategyAgent::SanctuaryRingFits(int32_t t) const {
  for (auto [w, h] : {std::pair{2, 3}, std::pair{3, 2}})
    for (int x = board_.X(t) - w + 1; x <= board_.X(t); ++x)
      for (int y = board_.Y(t) - h + 1; y <= board_.Y(t); ++y) {
        if (x < 0 || y < 0 || x + w > board_.width() || y + h > board_.height()) continue;
        bool good = true;
        for (int i = 0; i < w && good; ++i)
          for (int j = 0; j < h && good; ++j) {
            const int32_t u = board_.Tile(x + i, y + j);
            if (board_.at(u).seen_round < 0) { good = false; break; }
            if (i + 1 < w && board_.Step(u, 1) != board_.Tile(x + i + 1, y + j)) good = false;
            if (j + 1 < h && board_.Step(u, 2) != board_.Tile(x + i, y + j + 1)) good = false;
          }
        if (good) return true;
      }
  return false;
}

bot::Action StrategyAgent::Sanctuary() {
  auto drop = [&] { sanct_target_ = -1; };
  if (round_ >= kUntil || queen_growing_ || queen_forage_) { drop(); return bot::Action{}; }
  if (sanct_target_ < 0) {
    if (int(sanct_contact_.size()) < kContacts || round_ - sanct_last_ < kCooldown || sanct_enemy_.empty())
      return bot::Action{};
  }
  // the reach of every visible enemy head (QueenMove's threat zone: QueenRisk > 0)
  std::vector<uint8_t> threat(board_.size(), 0);
  for (const auto& [id, e] : seen_) {
    if (e.team == spawn_.team || e.round != round_ || e.head < 0 || !board_.Current(e.head) ||
        board_.at(e.head).occupant != id || !board_.at(e.head).occupant_head) continue;
    const int reach = QUEEN_ENDGAME && e.partial ? 8 : std::min(8, std::max(3, (e.length + 3) / 4 + e.length - 2));
    const auto paths = Search(board_, e.head, [&](int t) { return t == head_ || !Occupied(t); }, reach);
    for (int t = 0; t < board_.size(); ++t)
      if (paths.dist[t] >= 0 && paths.dist[t] <= reach) threat[t] = 1;
  }
  // safe paths from her head: known tiles, no dragon, out of every visible head's reach
  std::vector<int32_t> dist(board_.size(), -1), parent(board_.size(), -1);
  std::vector<int8_t> pdir(board_.size(), -1);
  std::vector<int32_t> queue{head_};
  dist[head_] = 0;
  for (size_t qi = 0; qi < queue.size(); ++qi) {
    const int32_t u = queue[qi];
    ++work_;
    if (dist[u] >= kReach) continue;
    for (int d = 0; d < 4; ++d) {
      const int32_t n = board_.Step(u, d);
      // she doesn't eat on the way: her ring is rebuilt only at length <= 3 (RingQueen), and a
      // queen grown to 4 off her ring roams (arena duel game 0: ate at r272, roamed to her death)
      if (n < 0 || dist[n] >= 0 || Occupied(n) || threat[n] || (board_.Current(n) && board_.at(n).pearl)) continue;
      dist[n] = dist[u] + 1; parent[n] = u; pdir[n] = int8_t(d);
      queue.push_back(n);
    }
  }
  auto front = [&](int32_t t) {
    int best = kDistCap;
    for (const auto& p : sanct_enemy_) best = std::min(best, int(Torus(t, p.second)));
    return best;
  };
  if (sanct_target_ < 0) {
    std::unordered_map<int32_t, int32_t> allies;   // id -> head tile, seen or reported within kWindow
    for (const auto& [id, e] : seen_)
      if (id != spawn_.id && e.team == spawn_.team && e.head >= 0 && round_ - e.round <= kWindow) allies[id] = e.head;
    for (const auto& [id, r] : reports_)
      if (id != spawn_.id && r.tile >= 0 && round_ - r.round <= kWindow && !allies.count(id)) allies[id] = r.tile;
    auto score = [&](int32_t t) {
      int near = 0;
      for (const auto& [id, a] : allies) if (Torus(t, a) <= kAllyNear) ++near;
      return front(t) + std::min(near, kAllyMax);
    };
    const int here = score(queen_home_ >= 0 && Torus(queen_home_, head_) <= 3 ? queen_home_ : head_);
    std::vector<std::pair<int, int32_t>> cands;
    for (const int32_t t : queue) {
      if (t == head_ || Corridor(t) || board_.at(t).countdown >= 0) continue;
      const int s = score(t);
      if (s >= here + kGain) cands.push_back({s * 64 - dist[t], t});
    }
    std::sort(cands.begin(), cands.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
    const int need = std::clamp(kQueenSpacePerLength * length_, kForageRoom, kQueenSpaceCap);
    q_body_.Begin(board_.size());
    for (const int32_t b : body_) if (b >= 0) q_body_.Set(b);
    int checked = 0;
    for (const auto& [s, t] : cands) {
      if (++checked > 16) break;
      if (!SanctuaryRingFits(t) || QueenSpace(t, need) < need) continue;
      sanct_target_ = t; sanct_since_ = round_; sanct_last_ = round_;
#ifdef SANCT_DBG
      std::fprintf(stderr, "SANCT pick r%d team%d len%d from %d,%d to %d,%d here%d score%d dist%d contacts%d\n", round_, spawn_.team, length_, board_.X(head_), board_.Y(head_), board_.X(t), board_.Y(t), here, s / 64 + 1, dist[t], int(sanct_contact_.size()));
#endif
      queen_home_ = t; ring_.clear(); ++sanct_moves_;
      break;
    }
    if (sanct_target_ < 0) return bot::Action{};
  }
  if (round_ - sanct_since_ > kTimeout) {
#ifdef SANCT_DBG
    std::fprintf(stderr, "SANCT timeout r%d team%d\n", round_, spawn_.team);
#endif
    drop(); return bot::Action{};
  }
  if (Torus(head_, sanct_target_) <= 1) {   // arrived: her ring is rebuilt here
    drop(); ring_.clear(); ++sanct_arrived_;
#ifdef SANCT_DBG
    std::fprintf(stderr, "SANCT arrive r%d team%d len%d at %d,%d slots%d\n", round_, spawn_.team, length_, board_.X(head_), board_.Y(head_), FreeSlots());
#endif
    if (length_ >= 4 && FreeSlots() > 0 && int(body_.size()) == length_ && SplitSafe(length_ - 2))
      return Because("sanctuary: shorten for ring", bot::Action::Split(length_ - 2));
    return bot::Action{};
  }
  if (dist[sanct_target_] < 0) return bot::Action{};   // no safe path this turn: QueenMove decides
  std::vector<int> path;
  for (int32_t t = sanct_target_; t != head_; t = parent[t]) path.push_back(pdir[t]);
  std::reverse(path.begin(), path.end());
  const int free_steps = (length_ + 3) / 4;
  std::vector<int32_t> b(body_.begin(), body_.end());
  bot::Action a;
  for (int i = 0; i < free_steps && i < int(path.size()); ++i) {
    const int32_t t = board_.Step(b.front(), path[i]);
    if (t < 0 || !board_.Current(t) || board_.at(t).occupant >= 0 || board_.at(t).pearl || QueenRisk(t) > 0 ||
        std::find(b.begin(), b.end(), t) != b.end()) break;
    std::vector<int32_t> nb = b;
    nb.insert(nb.begin(), t);
    if (!board_.at(t).pearl) nb.pop_back();
    if (EscapeLevel(nb, EscapeCap(nb.size())) == 0) break;
    b = std::move(nb);
    a.steps.push_back(path[i]);
  }
  if (a.steps.empty()) return bot::Action{};
  body_.assign(b.begin(), b.end());
  return Because("sanctuary: walk", a);
}

}  // namespace gm2
