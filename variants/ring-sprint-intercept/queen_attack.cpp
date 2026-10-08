#include "strategy.hpp"
#include <algorithm>
#include <array>
#include <deque>
#include <cstdio>
#include <utility>

namespace gm2 {
bool StrategyAgent::TryQueenAttack(const rl::Observation& o,bot::Action* out) const {
  if(IsQueen())return false;
  int target=-1,own_head=-1;
  for(const auto& s:o.segments)if(s.head && s.id<=1) {
    if(s.team!=spawn_.team)target=board_.Tile(s.x,s.y);
    else own_head=board_.Tile(s.x,s.y);
  }
  if(target<0)return false;
  const bool paid_ok=RING12_ATTACK_MODE==1 && units_-1>=kLungeMinWorkers; // g38-6 guard
  return HeadAttack(o,target,own_head,paid_ok,out,nullptr);
}

// The bounded search of TryQueenAttack, for any enemy head `target`: our head lands on it this move
// (free steps, then paid ones if paid_ok) through free tiles in view, never through our queen's sole
// empty exit. *rival (optional): whether an ally in `rivals` (head tile, ally id, its rule reach) is
// nearer to the target by path in view (ties to the lower id); then no action is produced.
bool StrategyAgent::HeadAttack(const rl::Observation& o,int target,int own_head,bool paid_ok,bot::Action* out,
                               const std::vector<std::array<int,3>>* rivals) const {
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
  if(rivals)for(const auto& r:*rivals) {
    int j=index(r[0]);if(j<0 || distance[j]<0 || distance[j]>r[2])continue;
    if(distance[j]<distance[start] || (distance[j]==distance[start] && r[1]<spawn_.id))return false; // it goes
  }
  struct State {std::vector<int> body,eaten,path,visited;int head,length;};
  const bool complete=int(ring4_observed_body_.size())==length_;
  State initial;initial.body.assign(ring4_observed_body_.begin(),ring4_observed_body_.end());initial.head=head_;initial.length=length_;
  std::vector<State> frontier{initial};int expanded=0;
  const int free=(length_+3)/4;
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

// g52-15 (ring-sprint-intercept): arena-r26 P3 "queen killed in a live race" 311 games (lunged 293),
// and in 337 of 653 S1 losses an ally was within 3 of her. A short worker (length <= 6: a stub costs
// little, and gst's long workers carry the growth) that sees our queen's head and an enemy head H
// that could reach a tile within 1 of her head (H's rule reach ceil(Le/4)+Le-2, the cost rule the
// lunge field uses, plus 2: H may act before us this round and again next round) trades heads with
// H when its own move can land on H's head. Only the nearest such ally by path goes.
bool StrategyAgent::TryIntercept(const rl::Observation& o,bot::Action* out) const {
#ifdef INTERCEPT_DBG
  if(IsQueen())return false;
#else
  if(IsQueen() || length_>kInterceptMaxLength)return false;
#endif
  int queen=-1;
  for(const auto& s:o.segments)if(s.head && s.id<=1 && s.team==spawn_.team)queen=board_.Tile(s.x,s.y);
  if(queen<0)return false;
  std::vector<std::array<int,3>> rivals;   // other short non-queen allies in view: head, id, reach
  for(const auto& s:o.segments) {
    if(!s.head || s.team!=spawn_.team || s.id<=1 || s.id==spawn_.id)continue;
    auto w=seen_.find(s.id);
    if(w==seen_.end() || w->second.round!=round_ || w->second.partial || w->second.length>kInterceptMaxLength)continue;
    const int la=w->second.length;
    rivals.push_back({board_.Tile(s.x,s.y),int(s.id),la>=2?(la+3)/4+la-2:1});
  }
  // the enemy heads closing on her, nearest (by their path in view) first
  std::vector<std::pair<int,int>> threats;   // (distance to her, head tile)
  std::vector<int> dist(board_.size(),-1);
  std::vector<int> queue;
  for(const auto& s:o.segments) {
    if(!s.head || s.team==spawn_.team)continue;
    auto w=seen_.find(s.id);
    const int le=std::max<int>(2,w!=seen_.end()?w->second.length+(w->second.partial?1:0):2);
    const int reach=(le+3)/4+le-2+2;
    const int h=board_.Tile(s.x,s.y);
    std::fill(dist.begin(),dist.end(),-1);queue.assign(1,h);dist[h]=0;
    int best=-1;
    for(size_t qi=0;qi<queue.size();++qi) {
      const int t=queue[qi];
      bool near=t==queen;
      for(int d=0;d<4 && !near;++d)near=board_.Step(t,d)==queen;
      if(near){best=dist[t];break;}
      if(dist[t]>=reach)continue;
      for(int d=0;d<4;++d) {
        const int n=board_.Step(t,d);
        if(n<0 || dist[n]>=0 || !board_.Current(n) || Classify(n)!=Cell::kFree)continue;
        dist[n]=dist[t]+1;queue.push_back(n);
      }
    }
    if(best>=0)threats.push_back({best,h});
  }
  std::sort(threats.begin(),threats.end());
#ifdef INTERCEPT_DBG
  if(!threats.empty()) {
    bot::Action probe;bool can=false;for(const auto& [d,h]:threats)can=can||HeadAttack(o,h,queen,true,&probe,nullptr);
    const int h=threats[0].second;
    const int md=std::abs(board_.X(h)-board_.X(head_))+std::abs(board_.Y(h)-board_.Y(head_));
    std::fprintf(stderr,"ITHREAT r%d team%d id%d len%d threats%d d%d md%d can%d\n",round_,int(spawn_.team),int(spawn_.id),length_,int(threats.size()),threats[0].first,md,int(can));
  }
  if(length_>kInterceptMaxLength)return false;
#endif
  for(const auto& [d,h]:threats)if(HeadAttack(o,h,queen,true,out,&rivals)) {
#ifdef INTERCEPT_DBG
    std::fprintf(stderr,"INTERCEPT r%d team%d id%d len%d steps%d hunter_dist%d\n",round_,int(spawn_.team),int(spawn_.id),length_,int(out->steps.size()),d);
#endif
    return true;
  }
  return false;
}
} // namespace gm2
