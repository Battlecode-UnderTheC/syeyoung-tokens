#include "strategy.hpp"
#include <algorithm>
#include <set>
namespace gm2 {
#ifndef RING_SIZE
#define RING_SIZE 6
#endif
#ifndef RING_RELEASE
#define RING_RELEASE 500
#endif
namespace {
std::vector<int> rectangle(const strat::BoardMemory& b,int x,int y,int w,int h) {
 std::vector<int> r;
 for(int i=0;i<w;++i)r.push_back(b.Tile(x+i,y));
 for(int j=1;j<h;++j)r.push_back(b.Tile(x+w-1,y+j));
 for(int i=w-2;i>=0;--i)r.push_back(b.Tile(x+i,y+h-1));
 for(int j=h-2;j>0;--j)r.push_back(b.Tile(x,y+j));
 return r;
}
}
uint64_t StrategyAgent::RingWord() const {
 int x=board_.width(),y=board_.height(),xx=0,yy=0;
 for(int t:ring_) {x=std::min(x,board_.X(t));y=std::min(y,board_.Y(t));xx=std::max(xx,board_.X(t));yy=std::max(yy,board_.Y(t));}
 return (uint64_t(0xBA)<<56)|(uint64_t(spawn_.team)<<25)|uint64_t(x)|(uint64_t(y)<<6)|(uint64_t(xx-x+1)<<12)|(uint64_t(yy-y+1)<<14)|(uint64_t(ring_seen_&511)<<16)|QGateSymField();   // g42-4, g43-1
}
// g42-4's sighting field (bits 26-34 round, bit 35 valid), sharing its bits with the symmetry
// candidates (g43-1; ring-symearly-1 carried them in bits 26-28, which qgate took since). The gate
// (EnemyQueenRace) reads a sighting before kQGateFrom only as "seen, and stale": any value in
// [0, kQGateFrom) means the same. So a sighting from kQGateFrom on goes as its round (>= 300 > 7);
// an earlier one goes as the candidate mask (0-7, still a pre-300 round to the gate); with no
// sighting the valid bit stays clear and bits 26-28 hold the mask.
uint64_t StrategyAgent::QGateSymField() const {
 const uint64_t mask=uint64_t(sym_.Alive()&7);
 const int32_t k=EnemyQueenKnown();
 if(k>=0&&k>enemy_queen_dead_) return (uint64_t(1)<<35)|(uint64_t(k>=kQGateFrom?(k&511):mask)<<26);
 return mask<<26;
}
bot::Action StrategyAgent::RingQueen() {
 ring5_next_=-1;
 if(queen_growing_ || round_>=RING_RELEASE) {ring_.clear();return bot::Action{};}
 auto free=[&](int t){return t>=0&&board_.Current(t)&&(board_.at(t).occupant<0||board_.at(t).occupant==spawn_.id);};
 if(ring_.empty()&&length_>=4&&round_<100&&FreeSlots()>1&&int(body_.size())==length_&&SplitSafe(length_-2))
  return Because("ring: bootstrap shorten",bot::Action::Split(length_-2));
 if(ring_.empty()&&(length_<=3||((RING5_MODE==1||RING5_MODE==2)&&round_<430&&round_<=ring5_recover_until_&&length_<=5))&&int(body_.size())==length_) {
  double best=-1e30;std::vector<int> selected;
  for(auto [w,h]:std::vector<std::pair<int,int>>{{2,2},{3,2},{2,3}}) {
   if(2*(w+h)-4!=6)continue;
   for(int x=board_.X(head_)-w+1;x<=board_.X(head_);++x)for(int y=board_.Y(head_)-h+1;y<=board_.Y(head_);++y) {
    if(x<0||y<0||x+w>board_.width()||y+h>board_.height())continue;
    auto base=rectangle(board_,x,y,w,h);
    for(int reverse=0;reverse<2;++reverse) {
     auto r=base;if(reverse)std::reverse(r.begin(),r.end());
     auto at=std::find(r.begin(),r.end(),head_);if(at==r.end())continue;std::rotate(r.begin(),at,r.end());
     bool good=true;std::set<int> exits;
     for(size_t i=0;i<r.size();++i) {
      if(!free(r[i]))good=false;
      bool edge=false;for(int d=0;d<4;++d)if(board_.Step(r[i],d)==r[(i+1)%r.size()])edge=true;
      if(!edge)good=false;
      for(int d=0;d<4;++d){int n=board_.Step(r[i],d);if(n>=0&&std::find(r.begin(),r.end(),n)==r.end()&&free(n))exits.insert(n);}
     }
     for(int i=1;i<length_;++i)if(body_[i]!=r[r.size()-i])good=false;
     if(!good||exits.size()<2)continue;
     // Prefer a six-cell loop with room for a three-cell child, then outside escape room.
     double score=10*int(r.size())+exits.size();
     for(const auto&[id,e]:seen_)if(id!=spawn_.id&&e.round==round_&&e.head>=0){
      int near=1000;for(int t:r){int dx=std::abs(board_.X(t)-board_.X(e.head)),dy=std::abs(board_.Y(t)-board_.Y(e.head));near=std::min(near,std::min(dx,board_.width()-dx)+std::min(dy,board_.height()-dy));}
      if(near<=3)score-=e.team==spawn_.team?3:8;
     }
     for(int t:r)if(board_.at(t).countdown>=0)score-=2;

     if(score>best){best=score;selected=r;}
    }
   }
  }
  ring_=selected;
 }
 if(ring_.empty())return bot::Action{};
 ring_seen_=round_;
 auto pos=std::find(ring_.begin(),ring_.end(),head_);
 if(pos==ring_.end()){ring_.clear();return bot::Action{};}
 int next=ring_[(pos-ring_.begin()+1)%ring_.size()];
 ring5_next_=next;
 bool danger=false;
 for(const auto& [id,e]:seen_)if(e.team!=spawn_.team&&e.round==round_&&e.head>=0&&board_.Current(e.head)&&board_.at(e.head).occupant==id&&board_.at(e.head).occupant_head) {
  int reach=QUEEN_ENDGAME&&e.partial?8:std::min(8,std::max(3,(e.length+3)/4+e.length-2));
  auto paths=Search(board_,e.head,[&](int t){return t==head_||t==next||!Occupied(t);},reach);
  if((paths.dist[head_]>=0&&paths.dist[head_]<=reach)||(paths.dist[next]>=0&&paths.dist[next]<=reach))danger=true;
 }
 if(danger&&RING5_MODE!=2){
  // Keep the broadcast reservation this turn, even when the queen must evade.
  if(QUEEN_ENDGAME && round_>=QUEEN_SAFETY_FROM)return bot::Action{};
  ring_.clear();return bot::Action{};
 }
 const int threshold=ring_.size()==4?4:5;
 if(!danger&&length_>=threshold&&FreeSlots()>0&&int(body_.size())==length_&&!(round_==499&&free(next)&&!Mine(next))) {
  int split=round_==499?2:length_-2;
  return Because(ring_.size()==4?"ring: pearl split two":"ring: pearl split three",bot::Action::Split(split));
 }
 if(RING5_MODE==2&&danger)return bot::Action{};
 if(free(next)&&!Mine(next))for(int d=0;d<4;++d)if(board_.Step(head_,d)==next) {
  ring5_last_patrol_=round_;ring5_recover_until_=round_+12;
  body_.push_front(next);if(!board_.at(next).pearl)body_.pop_back();bot::Action a;a.steps={d};return Because("ring: pearl patrol",a);
 }
 ring_.clear();return bot::Action{};
}
bot::Action StrategyAgent::RingChildEscape() {
 if(int(body_.size())!=length_)return bot::Action::Resign();
 auto inside=[&](int t){return std::find(ring_.begin(),ring_.end(),t)!=ring_.end();};
 struct Route{std::vector<int> body,steps,eaten;};
 std::vector<Route> beam{{std::vector<int>(body_.begin(),body_.end()),{}, {}}};
 const int free_steps=(length_+3)/4;
 for(int depth=0;depth<6;++depth) {
  std::vector<Route> next;
  for(const auto& r:beam)for(int d=0;d<4;++d) {
   int t=board_.Step(r.body.front(),d);
   if(t<0||!board_.Current(t)||inside(t)||std::find(r.body.begin(),r.body.end(),t)!=r.body.end())continue;
   auto& cell=board_.at(t);if(cell.occupant>=0&&cell.occupant!=spawn_.id)continue;
   if(cell.occupant==spawn_.id&&std::find(body_.begin(),body_.end(),t)==body_.end())continue;
   bool pay=depth>=free_steps;if(pay&&r.body.size()<=2)continue;
   Route n=r;n.steps.push_back(d);n.body.insert(n.body.begin(),t);
   if(cell.pearl&&std::find(n.eaten.begin(),n.eaten.end(),t)==n.eaten.end())n.eaten.push_back(t);else n.body.pop_back();
   if(pay)n.body.pop_back();
   if(std::none_of(n.body.begin(),n.body.end(),inside)&&EscapeLevel(n.body,EscapeCap(n.body.size()))>0) {
    body_.assign(n.body.begin(),n.body.end());bot::Action a;a.steps=n.steps;return a;
   }
   next.push_back(std::move(n));
  }
  std::stable_sort(next.begin(),next.end(),[&](const Route&a,const Route&b){return std::count_if(a.body.begin(),a.body.end(),inside)<std::count_if(b.body.begin(),b.body.end(),inside);});
  if(next.size()>32)next.resize(32);beam=std::move(next);
 }
 return bot::Action::Resign();
}
bot::Action StrategyAgent::RingWorker(bot::Action normal,const rl::Observation& o) {
 ring_worker_override_=false;
 if(ring_child_release_>=0&&round_>=ring_child_release_) {rescue_committed_=true;ring_worker_override_=true;return Because("ring: return capacity outside",bot::Action::Resign());}
 for(uint64_t word:o.inbox)if((word>>56)==0xBA||(word>>56)==0xB8) {
  if((word>>56)==0xBA&&int((word>>25)&1)!=spawn_.team)continue;
  int stamp=(word>>16)&511,age=(round_-stamp)&511;
  int x=word&63,y=(word>>6)&63,w=(word>>12)&3,h=(word>>14)&3;
  if((word>>56)==0xB8){
   bool own=false;
   for(int q=0;q<2;++q){auto it=seen_.find(q);if(it==seen_.end())continue;const auto&e=it->second;
    if(e.team==spawn_.team&&e.round==round_&&e.head>=0&&board_.Current(e.head)&&board_.at(e.head).occupant==q&&board_.at(e.head).occupant_head){int qx=board_.X(e.head),qy=board_.Y(e.head);own|=qx>=x&&qx<x+w&&qy>=y&&qy<y+h;}
   }
   if(!own)continue;
  }
  if(age<=12&&round_-age>=ring_seen_&&w>=2&&h>=2&&w+h<=5&&x+w<=board_.width()&&y+h<=board_.height()) {ring_=rectangle(board_,x,y,w,h);ring_seen_=round_-age;}
 }
 if(round_-ring_seen_>12)ring_.clear();
 if(normal.kind==bot::Action::kSplit&&FreeSlots()<=1) {normal=SafeStep(FleeDirection(),false,false);rescue_committed_=true;ring_worker_override_=true;}
 if(ring_.empty())return normal;
 auto inside=[&](int t){return std::find(ring_.begin(),ring_.end(),t)!=ring_.end();};
 if(born_==round_&&!leader_&&int(body_.size())==length_&&std::all_of(body_.begin(),body_.end(),inside)) {
  rescue_committed_=true;ring_worker_override_=true;
  if(length_>=3) {auto a=RingChildEscape();if(!a.resign){if(FreeSlots()==0)ring_child_release_=round_+1;return Because("ring: child escape",a);}}
  return Because("ring: child immediate clearance",bot::Action::Resign());
 }
 // Keep the loop clear and reserve two currently observed exterior cells, allowing other workers to grow.
 std::set<int> external;
 for(int tile:ring_)for(int d=0;d<4;++d){int t=board_.Step(tile,d);if(t>=0&&board_.Current(t)&&!inside(t)&&board_.at(t).occupant<0)external.insert(t);}
 std::vector<int> blocked=ring_;for(int t:external){if(blocked.size()>=ring_.size()+2)break;blocked.push_back(t);}
 auto reserved=[&](int t){return std::find(blocked.begin(),blocked.end(),t)!=blocked.end();};
 bool enters=false;int t=head_;
 if(normal.kind==bot::Action::kMove&&!normal.resign)for(int d:normal.steps){t=board_.Step(t,d);if(t<0)break;if(reserved(t))enters=true;}
 if(normal.kind==bot::Action::kSplit)for(int tile:body_)if(inside(tile))enters=true;
 if(!enters)return normal;
 for(int d=0;d<4;++d){int n=board_.Step(head_,d);if(n<0||reserved(n)||Classify(n)!=Cell::kFree)continue;std::vector<int>b(body_.begin(),body_.end());b.insert(b.begin(),n);if(!board_.at(n).pearl)b.pop_back();if(EscapeLevel(b,EscapeCap(length_))==0)continue;rescue_committed_=true;ring_worker_override_=true;bot::Action a;a.steps={d};return Because("ring: worker yield loop",a);}
 return normal;
}
}

namespace gm2 {
// Risk 16 denotes a direct step; longer approaches are ranked by distance.
// A clipped body is a lower bound, so use the full local search horizon.
int StrategyAgent::QueenRisk(int tile) const {
 if(queen_risk_round_!=round_){queen_risk_round_=round_;queen_risk_cache_.assign(board_.size(),-1);}
 if(queen_risk_cache_[tile]>=0)return queen_risk_cache_[tile];
 int risk=0;
 for(const auto&[id,e]:seen_) {
  if(e.team==spawn_.team||e.round!=round_||e.head<0||!board_.Current(e.head)||(!board_.at(e.head).occupant_head||board_.at(e.head).occupant!=id))continue;
  for(int d=0;d<4;++d)if(board_.Step(e.head,d)==tile)risk=16;
  const int reach=QUEEN_ENDGAME&&e.partial?8:std::min(8,std::max(3,(e.length+3)/4+e.length-2));
  auto paths=Search(board_,e.head,[&](int t){return t==tile||(board_.Current(t)&&(board_.at(t).occupant<0||board_.at(t).occupant==spawn_.id));},reach);
  if(paths.dist[tile]>=0&&paths.dist[tile]<=reach)risk=std::max(risk,9-paths.dist[tile]);
 }
 queen_risk_cache_[tile]=risk;return risk;
}
uint64_t StrategyAgent::DeliveryWord() const {
 int x=board_.X(body_.empty()?head_:body_.front()),y=board_.Y(body_.empty()?head_:body_.front()),w=0,h=0,index=0;
 auto at=std::find(ring_.begin(),ring_.end(),body_.empty()?head_:body_.front());
 if(at!=ring_.end()){
  x=board_.width();y=board_.height();int xx=0,yy=0;
  for(int t:ring_){x=std::min(x,board_.X(t));y=std::min(y,board_.Y(t));xx=std::max(xx,board_.X(t));yy=std::max(yy,board_.Y(t));}
  w=xx-x+1;h=yy-y+1;auto rect=rectangle(board_,x,y,w,h);index=std::find(rect.begin(),rect.end(),body_.empty()?head_:body_.front())-rect.begin();
 }
 int dock=dock_<0?4095:board_.X(dock_)|(board_.Y(dock_)<<6);
 return (uint64_t(0xB9)<<56)|uint64_t(x)|(uint64_t(y)<<6)|(uint64_t(w)<<12)|(uint64_t(h)<<14)|(uint64_t(index)<<16)|(uint64_t(dock)<<19)|(uint64_t(courier_<0?4095:courier_)<<31)|(uint64_t(round_&511)<<43)|(uint64_t(spawn_.id)<<52)|(uint64_t(queen_growing_)<<53)|(uint64_t(dock_>=0)<<54)|(uint64_t(spawn_.team&1)<<55);
}
void StrategyAgent::ReadDelivery(const rl::Observation& o) {
 for(uint64_t word:o.inbox)if((word>>56)==0xB9&&int((word>>55)&1)==spawn_.team){
  int stamp=(word>>43)&511,age=(round_-stamp)&511;if(age>2)continue;
  int x=word&63,y=(word>>6)&63,w=(word>>12)&3,h=(word>>14)&3,index=(word>>16)&7;
  if(x>=board_.width()||y>=board_.height())continue;
  if(w&&h){if(w<2||h<2||w+h>5||x+w>board_.width()||y+h>board_.height())continue;auto rect=rectangle(board_,x,y,w,h);if(index>=int(rect.size()))continue;ring_=rect;ring_seen_=round_-age;delivery_queen_head_=rect[index];}
  else {ring_.clear();delivery_queen_head_=board_.Tile(x,y);}
  delivery_seen_=round_-age;queen_id_=(word>>52)&1;courier_=(word>>31)&4095;if(courier_==4095)courier_=-1;
  int dock=(word>>19)&4095;dock_=((word>>54)&1)?board_.Tile(dock&63,(dock>>6)&63):-1;delivery_growing_=(word>>53)&1;
 }
}
void StrategyAgent::SelectCourier() {
 if(round_>=499){courier_=-1;dock_=-1;return;}
 if(dock_>=0&&board_.Current(dock_)&&board_.at(dock_).pearl&&board_.at(dock_).occupant<0){courier_=-1;return;}
 if(courier_>=0&&round_-courier_since_<12)return;
 courier_=-1;dock_=-1;
 auto distance=[&](int t){int dx=std::abs(board_.X(t)-board_.X(head_)),dy=std::abs(board_.Y(t)-board_.Y(head_));return std::min(dx,board_.width()-dx)+std::min(dy,board_.height()-dy);};
 for(const auto&[id,e]:seen_){
  if(id<2||id>=4095||e.team!=spawn_.team||e.round!=round_||e.head<0||!board_.Current(e.head)||!board_.at(e.head).occupant_head||e.length<2)continue;
  if(distance(e.head)<2||distance(e.head)>3||std::find(ring_.begin(),ring_.end(),e.head)!=ring_.end())continue;
  if(courier_<0||id<courier_){courier_=id;dock_=e.head;}
 }
 courier_since_=round_;
}
bot::Action StrategyAgent::CourierMove() {
 if(round_>=499||round_-delivery_seen_>2||courier_!=spawn_.id||dock_<0)return bot::Action{};
 // Do not act on a relayed location alone. The queen must be directly visible now.
 auto q=seen_.find(queen_id_);if(q==seen_.end()||q->second.round!=round_||q->second.head<0||!board_.Current(q->second.head)||!board_.at(q->second.head).occupant_head)return bot::Action{};
 const int qhead=q->second.head;
 auto distance=[&](int t){int dx=std::abs(board_.X(t)-board_.X(qhead)),dy=std::abs(board_.Y(t)-board_.Y(qhead));return std::min(dx,board_.width()-dx)+std::min(dy,board_.height()-dy);};
 if(head_!=dock_||distance(head_)<2||distance(head_)>3||int(body_.size())!=length_)return bot::Action{};
 for(int t:body_)if(distance(t)<=1||std::find(ring_.begin(),ring_.end(),t)!=ring_.end())return bot::Action{};
 rescue_committed_=true;return Because("delivery: selected outside donor",bot::Action::Resign());
}
bot::Action StrategyAgent::ProtectQueenAction(bot::Action a,const bot::Action& fallback) {
 if(IsQueen())return a;
 auto safe=[&](const bot::Action& action){
  if(action.resign)return true;
  auto inside=[&](int t){return round_-ring_seen_<=12&&std::find(ring_.begin(),ring_.end(),t)!=ring_.end();};
  if(action.kind==bot::Action::kSplit){for(int t:ring4_observed_body_)if(inside(t))return false;return true;}
  if(action.kind!=bot::Action::kMove)return true;
  std::vector<int> b=ring4_observed_body_,eaten;
  if(int(b.size())!=length_)return false;
  const int free=(length_+3)/4;
  for(size_t i=0;i<action.steps.size();++i){int t=board_.Step(b.front(),action.steps[i]);if(t<0)return true;
   if(inside(t))return false;
   if(board_.Current(t)&&board_.at(t).occupant>=0&&board_.at(t).occupant!=spawn_.id&&board_.at(t).occupant_team==spawn_.team)return false;
   if(std::find(b.begin(),b.end(),t)!=b.end())return false;
   bool pay=int(i)>=free;if(pay&&b.size()<=2)return false;
   b.insert(b.begin(),t);
   if(board_.at(t).pearl&&std::find(eaten.begin(),eaten.end(),t)==eaten.end())eaten.push_back(t);else b.pop_back();
   if(pay)b.pop_back();
  }
  int before=0,after=0;for(int t:ring4_observed_body_)before+=inside(t);for(int t:b)after+=inside(t);
  bool ok=before==0?after==0:after<before;
  if(ok){body_.assign(b.begin(),b.end());}
  return ok;
 };
 if(safe(a))return a;
 if(safe(fallback))return Because("delivery: final queen-path veto",fallback);
 for(int d=0;d<4;++d){int t=board_.Step(head_,d);if(t<0||Classify(t)!=Cell::kFree)continue;auto step=bot::Action::Move(d);if(!safe(step))continue;std::vector<int>b=ring4_observed_body_;b.insert(b.begin(),t);if(!board_.at(t).pearl)b.pop_back();if(EscapeLevel(b,EscapeCap(b.size()))>0)return Because("delivery: final queen-path yield",step);}
 // A feeding proximity test is not a path-safety test. Confirm the reservation
 // against a directly visible queen before making it an irreversible veto.
 auto q=seen_.find(queen_id_);
 bool confirmed=q!=seen_.end()&&q->second.round==round_&&q->second.head>=0&&
  board_.Current(q->second.head)&&board_.at(q->second.head).occupant==queen_id_&&
  board_.at(q->second.head).occupant_head&&round_-ring_seen_<=12&&
  std::find(ring_.begin(),ring_.end(),q->second.head)!=ring_.end();
 auto violates=[&](const bot::Action& action){
  if(!confirmed||action.resign)return false;
  auto inside=[&](int t){return std::find(ring_.begin(),ring_.end(),t)!=ring_.end();};
  if(action.kind==bot::Action::kSplit){for(int t:ring4_observed_body_)if(inside(t))return true;return false;}
  int t=head_;for(int d:action.steps){t=board_.Step(t,d);if(t<0)break;if(inside(t))return true;}
  for(int t:ring4_observed_body_)if(inside(t))return true;
  return false;
 };
 if(violates(a)||violates(fallback)) {
  // Try several paid/free steps before sacrificing a worker. This simulator
  // checks pearls and requires its entire resulting body outside the loop.
  if(int(ring4_observed_body_.size())==length_) {
   body_.assign(ring4_observed_body_.begin(),ring4_observed_body_.end());
   auto escape=RingChildEscape();
   if(!escape.resign&&!escape.steps.empty())return Because("ring5: confirmed loop escape",escape);
  }
  return Because("ring5: confirmed loop clear",bot::Action::Resign());
 }
 body_.assign(ring4_observed_body_.begin(),ring4_observed_body_.end());

 return fallback;
}
bool StrategyAgent::GrowthReady() const {
 if(ring_.empty()||int(body_.size())!=length_||round_>=498)return false;
 struct Route{std::vector<int>body,eaten;int outside=0;};
 std::vector<Route>beam{{std::vector<int>(body_.begin(),body_.end()),{},0}};
 for(int depth=0;depth<std::min(8,500-round_);++depth){std::vector<Route>next;
  for(const auto&r:beam)for(int d=0;d<4;++d){int t=board_.Step(r.body.front(),d);if(t<0||!board_.Current(t)||std::find(r.body.begin(),r.body.end(),t)!=r.body.end()||QueenRisk(t)>0)continue;
   auto cell=board_.at(t);if(cell.occupant>=0&&cell.occupant!=spawn_.id)continue;
   if(cell.occupant==spawn_.id&&std::find(body_.begin(),body_.end(),t)==body_.end())continue;
   Route n=r;n.body.insert(n.body.begin(),t);bool outside=std::find(ring_.begin(),ring_.end(),t)==ring_.end();
   if(cell.pearl&&std::find(n.eaten.begin(),n.eaten.end(),t)==n.eaten.end()){n.eaten.push_back(t);n.outside+=outside;}else n.body.pop_back();
   int room=0;int escape=EscapeLevel(n.body,std::max(18,2*int(n.body.size())),&room);
   if(outside&&n.outside>=2&&n.eaten.size()>=3&&escape>0&&room>=int(n.body.size())+2)return true;
   next.push_back(std::move(n));
  }
  std::stable_sort(next.begin(),next.end(),[](const Route&a,const Route&b){return a.eaten.size()+a.outside>b.eaten.size()+b.outside;});if(next.size()>32)next.resize(32);beam=std::move(next);if(beam.empty())break;
 }
 return false;
}
} // namespace gm2

namespace gm2 {
// Exact body/pearl transitions for four future one-step turns, restricted to current observations.
int StrategyAgent::QueenContinuation(const std::vector<int32_t>& body, const std::vector<int>& eaten, int* branches) const {
 // (ring-cpu-1) the same beam without copying bodies: a state is the base body plus at most
 // four new head tiles, truncated to its length. Same order, same pruning, same answer.
 using State=ContState;
 const int base_len=int(body.size());
 cont_index_.Begin(board_.size());
 for(int i=base_len-1;i>=0;--i)if(body[i]>=0)cont_index_.Set(body[i],i);   // the first index wins, as std::find
 auto in_body=[&](const State& s,int t){
  for(int j=0;j<s.k;++j)if(s.added[j]==t&&s.k-1-j<s.size)return true;
  return cont_index_.Has(t)&&s.k+cont_index_.Get(t)<s.size;
 };
 auto eaten_has=[&](const State& s,int t){
  for(int j=0;j<s.ne;++j)if(s.ate[j]==t)return true;
  return std::find(eaten.begin(),eaten.end(),t)!=eaten.end();
 };
 auto front=[&](const State& s){return s.k?s.added[s.k-1]:body.front();};
 State root;root.size=base_len;
 // member vectors keep their capacity from call to call: no allocation per call
 std::vector<State>& beam=cont_beam_;std::vector<State>& next=cont_next_;
 beam.assign(1,root);int reached=0;*branches=0;
 const int horizon=std::min(4,499-round_);
 for(int depth=0;depth<horizon;++depth){next.clear();int mask=0;
  work_+=beam.size();
  for(const auto&state:beam)for(int d=0;d<4;++d){int t=board_.Step(front(state),d);
   if(t<0||!board_.Current(t)||in_body(state,t))continue;
   if(board_.at(t).occupant>=0&&board_.at(t).occupant!=spawn_.id)continue;
   if(board_.at(t).occupant==spawn_.id&&!Mine(t))continue;
   State n=state;if(n.first<0)n.first=d;n.added[n.k++]=t;
   if(board_.at(t).pearl&&!eaten_has(n,t)){n.ate[n.ne++]=t;++n.size;}
   mask|=1<<n.first;next.push_back(n);
  }
  if(next.empty())break;
  // Retain alternatives for every first direction, including paths that avoid pearls.
  beam.clear();int counts[4]={};
  for(auto&n:next)if(counts[n.first]++<16)beam.push_back(n);
  reached=depth+1;*branches=__builtin_popcount(unsigned(mask));
 }
 return reached;
}
queen_survival::Result StrategyAgent::QueenFuture(const std::vector<int>& body, const std::vector<int>& eaten, int& budget, int& expanded) const {
 using namespace queen_survival;
 if(int(body_.size())!=length_ || body.empty())return Result::Unknown;
 const int horizon=std::min(RING8_MODE>=2?3:2,499-round_);
 if(horizon<=0)return Result::Alive;
 Environment env;
 env.step=[&](int tile,int d){return board_.Step(tile,d);};
 env.uncertain=[&](int tile){
   if(!board_.Current(tile))return true;
   int owner=board_.at(tile).occupant;
   return owner>=0 && (owner!=spawn_.id || std::find(body_.begin(),body_.end(),tile)==body_.end());
 };
 env.pearl=[&](int tile){return board_.at(tile).pearl;};
 env.slots_may_open=units_>1;
 env.require_move_witness=true;
 env.pearl_uncertain=[&](int tile,int elapsed,const std::vector<int>& consumed){
   const auto& cell=board_.at(tile);
   return (cell.pearl && !Contains(consumed,tile)) ||
     (elapsed>0 && cell.countdown>=0 &&
      (cell.countdown<=elapsed || Contains(consumed,tile)));
 };
 State state{body,eaten,{},FreeSlots()};
 queen_survival::Search search(env,budget);auto result=search.Run(state,horizon);expanded+=search.expanded;return result;
}
bot::Action StrategyAgent::Ring4Exit() {
 if(ring_.empty()||int(body_.size())!=length_)return bot::Action{};
 struct R{std::vector<int>body,steps,eaten;double value=0;};
 std::vector<R> beam{{std::vector<int>(body_.begin(),body_.end()),{},{},0}};R best;double score=-1e30;
 // One free step per future turn: conservative six-turn exit feasibility.
 bool over_budget=false;
 for(int depth=0;depth<6&&round_+depth<500&&!over_budget;++depth){std::vector<R>next;
  for(const auto&r:beam){
  // (ring-cpu-1) CPU safety net, as in QueenMove's beam: keep the best exit found so far
  if(depth>0&&work_>kQueenWorkCap){over_budget=true;break;}
  for(int d=0;d<4;++d){int t=board_.Step(r.body.front(),d);
   if(t<0||!board_.Current(t)||std::find(r.body.begin(),r.body.end(),t)!=r.body.end()||QueenRisk(t)>0)continue;
   if(board_.at(t).occupant>=0&&board_.at(t).occupant!=spawn_.id)continue;
   work_+=48+int(r.body.size())/4;   // copies, the sort and QueenRisk: ~1.6x the units of QueenMove
   R n=r;n.steps.push_back(d);n.body.insert(n.body.begin(),t);
   if(board_.at(t).pearl&&std::find(n.eaten.begin(),n.eaten.end(),t)==n.eaten.end())n.eaten.push_back(t);else n.body.pop_back();
   int room=0;if(EscapeLevel(n.body,std::max(18,2*int(n.body.size())),&room)==0)continue;
   bool outside=std::none_of(n.body.begin(),n.body.end(),[&](int x){return std::find(ring_.begin(),ring_.end(),x)!=ring_.end();});
   int branches=0,continuation=4;
   if(RING6_MODE>=2){continuation=QueenContinuation(n.body,n.eaten,&branches);if(continuation<std::min(4,499-round_))continue;}
   n.value=3*n.eaten.size()+0.02*room-0.1*n.steps.size()+(RING6_MODE>=2?8*branches:0);
   if(outside&&std::find(ring_.begin(),ring_.end(),board_.Step(head_,n.steps.front()))==ring_.end()&&n.value>score){score=n.value;best=n;}
   next.push_back(std::move(n));
  }
  }
  std::sort(next.begin(),next.end(),[](const R&a,const R&b){return a.value>b.value;});if(next.size()>32)next.resize(32);beam=std::move(next);
 }
 if(best.steps.empty())return bot::Action{};
 // Commit only the observed first step; recalculate all later decisions.
 int d=best.steps.front(),t=board_.Step(head_,d);
 if(RING8_MODE&&round_>=430){
   auto next=std::vector<int>(body_.begin(),body_.end());std::vector<int> eaten;
   next.insert(next.begin(),t);if(board_.at(t).pearl)eaten.push_back(t);else next.pop_back();
   int budget=1000,expanded=0;
   if(QueenFuture(next,eaten,budget,expanded)==queen_survival::Result::Dead)return bot::Action{};
 }
 body_.push_front(t);if(!board_.at(t).pearl)body_.pop_back();
 queen_growing_=true;ring_.clear();return Because("ring4: explicit exit",bot::Action::Move(d));
}
bot::Action StrategyAgent::Ring4Supply() {
 if(SUPPLY14_MODE==1 || SUPPLY14_MODE==2) {
  if(round_<450 || round_>=499 || int(body_.size())!=length_)return bot::Action{};
  const auto it=seen_.find(queen_id_);
  if(it==seen_.end())return bot::Action{};
  const auto&q=it->second;const int qh=q.head;
  if(q.team!=spawn_.team || q.round!=round_ || qh<0 || q.length<2 || !board_.Current(qh) || board_.at(qh).occupant!=queen_id_ || !board_.at(qh).occupant_head)return bot::Action{};
  // Frozen local reachability after removing this donor, never another ally.
  std::vector<int> reachable{qh};
  for(int depth=0,begin=0;depth<3;++depth){int end=reachable.size();
   for(int i=begin;i<end;++i)for(int d=0;d<4;++d){int t=board_.Step(reachable[i],d);
    if(t<0 || !board_.Current(t) || std::find(reachable.begin(),reachable.end(),t)!=reachable.end())continue;
    const auto&cell=board_.at(t);if(cell.occupant>=0 && cell.occupant!=spawn_.id)continue;
    reachable.push_back(t);
   }begin=end;
  }
  for(int i=0;i<length_;i+=2)if(std::find(reachable.begin(),reachable.end(),body_[i])!=reachable.end()){
   rescue_committed_=true;return Because("feed14: nearby sacrifice",bot::Action::Resign());
  }
  return bot::Action{};
 }

 if(round_<450||round_>=499||int(body_.size())!=length_||length_>12)return bot::Action{};
 auto q=seen_.find(queen_id_);if(q==seen_.end()||q->second.round!=round_||q->second.head<0||q->second.length<(RING4_STAGE>=3?2:6))return bot::Action{};
 int qh=q->second.head; if(!board_.Current(qh)||!board_.at(qh).occupant_head)return bot::Action{};
 if(RING4_STAGE>=3&&round_-ring_seen_<=12&&std::find(ring_.begin(),ring_.end(),qh)!=ring_.end())return bot::Action{};
 // No rendezvous: the corpse's head pearl must already be one clear step away.
 bool adjacent=false;for(int d=0;d<4;++d)adjacent|=board_.Step(qh,d)==head_;
 if(!adjacent)return bot::Action{};
 for(const auto&[id,e]:seen_)if(e.team!=spawn_.team&&e.round==round_&&e.head>=0){
  auto paths=Search(board_,e.head,[&](int t){return !Occupied(t)||t==head_||t==qh;},4);
  if((paths.dist[head_]>=0&&paths.dist[head_]<=4)||(paths.dist[qh]>=0&&paths.dist[qh]<=4))return bot::Action{};
 }
 rescue_committed_=true;return Because("ring4: observed nearby supply",bot::Action::Resign());
}
}

namespace gm2 {
void StrategyAgent::ReadRing5(const rl::Observation& o) {
 if(IsQueen())return;
 for(uint64_t word:o.inbox)if((word>>56)==0xBA||(word>>56)==0xB8){
  if((word>>56)==0xBA&&int((word>>25)&1)!=spawn_.team)continue;int stamp=(word>>16)&511,age=(round_-stamp)&511;
  int x=word&63,y=(word>>6)&63,w=(word>>12)&3,h=(word>>14)&3;
  if((word>>56)==0xB8){
   bool own=false;
   for(int q=0;q<2;++q){auto it=seen_.find(q);if(it==seen_.end())continue;const auto&e=it->second;
    if(e.team==spawn_.team&&e.round==round_&&e.head>=0&&board_.Current(e.head)&&board_.at(e.head).occupant==q&&board_.at(e.head).occupant_head){int qx=board_.X(e.head),qy=board_.Y(e.head);own|=qx>=x&&qx<x+w&&qy>=y&&qy<y+h;}
   }
   if(!own)continue;
  }
  if(age<=12&&round_-age>=ring_seen_&&w>=2&&h>=2&&w+h<=5&&x+w<=board_.width()&&y+h<=board_.height()){ring_=rectangle(board_,x,y,w,h);ring_seen_=round_-age;}
 }
 if(round_-ring_seen_>12)ring_.clear();
}
bool StrategyAgent::Ring5Reserved(int t) const {
 if(RING6_MODE>=3&&!IsQueen()&&round_>=430){
  for(int q=0;q<2;++q){auto it=seen_.find(q);if(it==seen_.end())continue;const auto&e=it->second;
   if(e.team!=spawn_.team||e.round!=round_||e.head<0||!board_.Current(e.head)||board_.at(e.head).occupant!=q||!board_.at(e.head).occupant_head)continue;
   for(int d=0;d<4;++d)if(board_.Step(e.head,d)==t&&board_.Current(t)&&board_.at(t).occupant<0)return true;
  }
 }
 if(RING5_MODE!=3||IsQueen()||round_-ring_seen_>12||std::find(ring_.begin(),ring_.end(),t)==ring_.end())return false;
 for(int q=0;q<2;++q){auto it=seen_.find(q);if(it==seen_.end())continue;const auto&e=it->second;
  if(e.team==spawn_.team&&e.round==round_&&e.head>=0&&board_.Current(e.head)&&board_.at(e.head).occupant==q&&board_.at(e.head).occupant_head&&std::find(ring_.begin(),ring_.end(),e.head)!=ring_.end())return true;
 }
 return false;
}
}

namespace gm2 {
bot::Action StrategyAgent::Supply14Yield(bot::Action a) {
 if(SUPPLY14_MODE!=2 || round_<450 || round_>=499 || rescue_committed_ || a.resign || a.kind!=bot::Action::kMove || a.steps.empty())return a;
 auto it=seen_.find(queen_id_);if(it==seen_.end())return a;
 const auto&q=it->second;int qh=q.head;
 if(q.team!=spawn_.team || q.round!=round_ || qh<0 || q.length<2 || !board_.Current(qh) || board_.at(qh).occupant!=queen_id_ || !board_.at(qh).occupant_head)return a;
 std::vector<int> reserved{qh};
 for(int depth=0,begin=0;depth<3;++depth){int end=reserved.size();for(int i=begin;i<end;++i)for(int d=0;d<4;++d){int t=board_.Step(reserved[i],d);if(t<0 || !board_.Current(t) || std::find(reserved.begin(),reserved.end(),t)!=reserved.end())continue;if(board_.at(t).occupant>=0)continue;reserved.push_back(t);}begin=end;}
 auto blocked=[&](int t){return t>=0 && board_.at(t).pearl && std::find(reserved.begin(),reserved.end(),t)!=reserved.end();};
 int tile=head_,first=-1;
 for(int k=0;k<int(a.steps.size());++k){tile=board_.Step(tile,a.steps[k]);if(blocked(tile)){first=k;break;}}
 if(first<0)return a;
 if(first>0){a.steps.resize(first);rescue_committed_=true;reason_="feed14: yield prefix";return a;}
 const auto saved=body_;body_=std::deque<int32_t>(ring4_observed_body_.begin(),ring4_observed_body_.end());
 int best=-1;double value=-1e30;
 for(int d=0;d<4;++d){int t=board_.Step(head_,d);if(t<0 || !board_.Current(t) || board_.at(t).occupant>=0 || blocked(t) || QueenRisk(t)>0)continue;
  std::vector<int32_t> next(body_.begin(),body_.end());next.insert(next.begin(),t);if(!board_.at(t).pearl)next.pop_back();
  int room=0;if(EscapeLevel(next,EscapeCap(next.size()),&room)<1)continue;
  double score=room;if(score>value){value=score;best=d;}
 }
 body_=saved;
 if(best<0)return a; // Survival takes precedence when all exits eat reserved pearls.
 rescue_committed_=true;return Because("feed14: yield pearl",bot::Action::Move(best));
}
}
