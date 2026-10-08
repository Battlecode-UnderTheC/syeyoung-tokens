#include "strategy.hpp"
#include <algorithm>
#include <array>
#include <deque>
#include <cstdio>

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
  int start=index(head_);
#ifdef QDEAD_DBG
  std::fprintf(stderr,"QAD r%d team%d id%d dist%d len%d\n",round_,spawn_.team,spawn_.id,start<0?-1:distance[start],length_);
#endif
  if(start<0 || distance[start]<0 || distance[start]>limit)return false;
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
#ifdef QDEAD_DBG
        std::fprintf(stderr,"QATK r%d team%d id%d at%d,%d tgt%d,%d len%d\n",round_,spawn_.team,spawn_.id,board_.X(head_),board_.Y(head_),board_.X(target),board_.Y(target),length_);
#endif
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

} // namespace gm2
