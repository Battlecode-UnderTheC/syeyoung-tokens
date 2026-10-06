#include "strategy.hpp"
#include <algorithm>
#include <array>
#include <deque>
#ifdef QDEAD_DBG
#include <cstdio>
#endif

#include "pathing.hpp"

namespace gm2 {
bool StrategyAgent::TryQueenAttack(const rl::Observation& o,bot::Action* out) const {
  if(IsQueen())return false;
  int target=-1,own_head=-1;
  for(const auto& s:o.segments)if(s.head && s.id<=1) {
    if(s.team!=spawn_.team)target=board_.Tile(s.x,s.y);
    else own_head=board_.Tile(s.x,s.y);
  }
  if(target<0)return false;
  // Direct observation only. Preserve the queen's sole observed empty exit.
  int reserved=-1;
  if(own_head>=0) {
    int empty=0;
    for(int d=0;d<4;++d) {
      int t=board_.Step(own_head,d);
      if(t>=0 && board_.Current(t) && board_.at(t).occupant<0){++empty;reserved=t;}
    }
    if(empty!=1)reserved=-1;
  }
  constexpr int limit=12,budget=1024;
  std::vector<int> tiles;
  for(int k=0;k<rl::kWindowTiles;++k) {
    int t=board_.Tile(o.tile_x[k],o.tile_y[k]);
    if(std::find(tiles.begin(),tiles.end(),t)==tiles.end())tiles.push_back(t);
  }
  auto index=[&](int t){auto p=std::find(tiles.begin(),tiles.end(),t);return p==tiles.end()?-1:int(p-tiles.begin());};
  int goal=index(target);if(goal<0)return false;
  std::vector<std::vector<int>> reverse(tiles.size());
  for(size_t i=0;i<tiles.size();++i)for(int d=0;d<4;++d) {
    int t=board_.Step(tiles[i],d),j=index(t);
    if(j<0 || t==reserved || !board_.Current(t))continue;
    int owner=board_.at(t).occupant;
    if(owner>=0 && owner!=spawn_.id && t!=target)continue;
    reverse[j].push_back(int(i));
  }
  std::vector<int> distance(tiles.size(),-1);std::deque<int> queue{goal};distance[goal]=0;
  while(!queue.empty()) {
    int t=queue.front();queue.pop_front();
    for(int p:reverse[t])if(distance[p]<0){distance[p]=distance[t]+1;queue.push_back(p);}
  }
  int start=index(head_);if(start<0 || distance[start]<0 || distance[start]>limit)return false;
  struct State {std::vector<int> body,eaten,path,visited;int head,length;};
  const bool complete=int(ring4_observed_body_.size())==length_;
  State initial;initial.body.assign(ring4_observed_body_.begin(),ring4_observed_body_.end());initial.head=head_;initial.length=length_;
  std::vector<State> frontier{initial};int expanded=0;
  const int free=(length_+3)/4;
  const bool paid_ok=RING12_ATTACK_MODE==1 && units_-1>=kLungeMinWorkers; // g38-6 guard
  const int maximum=paid_ok?limit:std::min(limit,free);
  for(int depth=0;depth<maximum && !frontier.empty();++depth) {
    std::vector<State> next;
    for(const auto& state:frontier)for(int d=0;d<4;++d) {
      if(++expanded>budget)return false; // Exhaustion means no attack override.
      bool paid=depth>=free;if(paid && state.length<=2)continue;
      int t=board_.Step(state.head,d),j=index(t);
      if(j<0 || t==reserved || !board_.Current(t) || distance[j]<0 || depth+1+distance[j]>maximum)continue;
      if(complete) {
        if(std::find(state.body.begin(),state.body.end(),t)!=state.body.end())continue;
      } else {
        // Unknown tails are never assumed released. Newly traversed cells are
        // conservatively blocked for the rest of this action as well.
        if(board_.at(t).occupant==spawn_.id || std::find(state.visited.begin(),state.visited.end(),t)!=state.visited.end())continue;
      }
      int owner=board_.at(t).occupant;
      if(owner>=0 && owner!=spawn_.id && t!=target)continue;
      State child=state;child.path.push_back(d);
      if(t==target) {
        out->kind=bot::Action::kMove;out->resign=false;out->steps=std::move(child.path);
        return true;
      }
      child.head=t;child.visited.push_back(t);
      bool food=board_.at(t).pearl && std::find(child.eaten.begin(),child.eaten.end(),t)==child.eaten.end();
      if(food){child.eaten.push_back(t);++child.length;}
      if(paid)--child.length;
      if(complete) {
        child.body.insert(child.body.begin(),t);
        if(!food)child.body.pop_back();
        if(paid)child.body.pop_back();
      }
      next.push_back(std::move(child));
    }
    frontier=std::move(next);
  }
  return false;
}
// g45-7 (ring-sprint-qdead2): the turn before a trade with the enemy queen. A dragon that attacks
// her dies with her in the same step, and a dead dragon casts no sonar, so the turn before is the
// last turn it can tell its allies. It claims her death only when she can't get away: she moves
// before us next round (ids 0-1 act first), and every tile her head can stop on (her free steps,
// through tiles free now; or her own tile if she is long enough to split instead) is within the
// steps we may take next turn (TryQueenAttack's limit: our free steps, plus paid steps while we stay
// longer than 2, at most 12) inside next turn's 7x7 view. A plain "she is within reach now" claim was right 22 times in
// 540 in sim (she steps away), and allies that heard it but missed the withdrawal took it for a death.
bool StrategyAgent::AboutToTakeQueen(const bot::Action& a) const {
  if (IsQueen() || a.resign || a.kind != bot::Action::kMove || a.steps.empty() || head_ < 0) return false;
  int32_t q = -1, qlen = 0;
  for (int32_t i = 0; i < 2; ++i)
    if (const auto s = seen_.find(i); s != seen_.end() && s->second.team != spawn_.team && s->second.head_round == round_ && !s->second.partial)
      q = s->second.head, qlen = s->second.length;
  if (q < 0 || qlen < 2) return false;   // her whole body in view: we know her length, so her free steps
  int32_t h = head_, grown = 0;
  for (const int d : a.steps) {
    const int32_t n = board_.Step(h, d);
    if (n < 0 || n == q) return false;   // an attack this very turn (or a step we can't follow)
    if (board_.Current(n) && board_.at(n).pearl) ++grown;
    h = n;
  }
  const auto free = [&](int32_t t) { return t != h && board_.Current(t) && !Occupied(t); };
  const auto hers = Search(board_, q, free, (qlen + 3) / 4);
  const int32_t after = length_ + grown - std::max(0, int32_t(a.steps.size()) - (length_ + 3) / 4);
  // each paid step needs length > 2 (docs/game.md), so at most after - 2 of them
  const int32_t budget = std::min(12, (after + 3) / 4 + (RING12_ATTACK_MODE == 1 && units_ - 1 >= kLungeMinWorkers ? std::max(0, after - 2) : 0));
  const auto ours = Search(board_, h, [&](int32_t t) { return board_.Current(t) && !Occupied(t); }, budget);
  const auto in_view = [&](int32_t t) {
    int32_t dx = std::abs(board_.X(t) - board_.X(h)), dy = std::abs(board_.Y(t) - board_.Y(h));
    return std::min(dx, board_.width() - dx) <= 3 && std::min(dy, board_.height() - dy) <= 3;
  };
  const auto caught = [&](int32_t t) { return in_view(t) && ours.dist[t] >= 0 && ours.dist[t] <= budget; };
  if (qlen >= 4) {   // she may split and stay: we must reach her tile too (an attack enters her head)
    bool next_to = false;
    for (int d = 0; d < 4; ++d) { const int32_t n = board_.Step(q, d); next_to |= n >= 0 && caught(n); }
    if (!in_view(q) || !next_to) return false;
  }
  for (int32_t t = 0; t < board_.size(); ++t)
    if (t != q && hers.dist[t] > 0 && !caught(t)) return false;
  return true;
}

// g45-7 word: magic 0xBD (8) | team (1, bit 55) | kind (2, bits 53-54) | round (9, bits 10-18)
//   kind 1: claim, she dies in that round (the sender reaches her head next turn)
//   kind 2: she was alive in that round (the claimant withdraws its claim)
//   kind 3: she is known dead since that round (relayed)
namespace {
constexpr uint64_t kQDeadMagic = 0xBD;
}
void StrategyAgent::ReadQueenTradeWords(const rl::Observation& o) {
  for (const uint64_t w : o.inbox) {
    if ((w >> 56) != kQDeadMagic || int32_t((w >> 55) & 1) != (spawn_.team & 1)) continue;
    const int32_t kind = int32_t((w >> 53) & 3), r = int32_t((w >> 10) & 511);
    if (r <= 0 || r > round_ + 1) continue;
    if (kind == 1) {
      trade_claim_ = std::max(trade_claim_, r);
      enemy_queen_heard_ = std::max(enemy_queen_heard_, std::min(r - 1, round_));   // the claimant saw her alive
    } else if (kind == 2 && r <= round_) {
      enemy_queen_heard_ = std::max(enemy_queen_heard_, r);
    } else if (kind == 3 && r <= round_) {
      enemy_queen_dead_ = std::max(enemy_queen_dead_, r);
    }
  }
  // every claimant has acted in round X by our turn in X+1: a claim nobody withdrew is a death
  if (trade_claim_ > 0 && round_ >= trade_claim_ + 1 && trade_claim_ > EnemyQueenKnown()) {
    enemy_queen_dead_ = std::max(enemy_queen_dead_, trade_claim_);
#ifdef QDEAD_DBG
    std::fprintf(stderr, "QCLAIMDEAD r%d team%d id%d claim%d\n", round_, spawn_.team, spawn_.id, trade_claim_);
#endif
  }
}
void StrategyAgent::QueenTradeWord(bot::Action* a) {
  const auto word = [&](int32_t kind, int32_t r) {
    return (kQDeadMagic << 56) | (uint64_t(spawn_.team & 1) << 55) | (uint64_t(kind) << 53) | (uint64_t(r & 511) << 10);
  };
  if (IsQueen()) return;
  if (AboutToTakeQueen(*a)) {
    own_claim_ = round_ + 1;
    for (int d = 0; d < 4; ++d) a->sonar[d] = word(1, own_claim_);
#ifdef QDEAD_DBG
    std::fprintf(stderr, "QCLAIM r%d team%d id%d\n", round_, spawn_.team, spawn_.id);
#endif
    return;
  }
  if (own_claim_ == round_) {   // alive and not trading: withdraw
    for (int d = 0; d < 4; ++d) a->sonar[d] = word(2, round_);
    own_claim_ = -1;
    return;
  }
  // a known death goes out on one direction a turn (rotating); dragons that know the queen's ring
  // carry it in their ring words on all four instead (RingWord)
  if (EnemyQueenDead() && (ring_.empty() || round_ - ring_seen_ > 12)) a->sonar[round_ & 3] = word(3, enemy_queen_dead_);
}
} // namespace gm2
