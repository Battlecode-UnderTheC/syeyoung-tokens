#pragma once
#include <algorithm>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace queen_survival {
enum class Result { Dead, Alive, Unknown };
struct State {
  std::vector<int> body, eaten, children;
  int slots = 0;
};
struct Environment {
  // -1 wall, -2 unobserved edge/portal.
  std::function<int(int,int)> step;
  // 0 known cell, 1 uncertain cell/another dragon; own ORIGINAL body is known.
  std::function<bool(int)> uncertain;
  std::function<bool(int)> pearl;
  std::function<bool(int,int,const std::vector<int>&)> pearl_uncertain;
  int max_action_steps = 12;
  bool slots_may_open = false;
  bool require_move_witness = false;
};
inline bool Contains(const std::vector<int>& v, int t) {
  return std::find(v.begin(),v.end(),t)!=v.end();
}
// Collision is checked BEFORE removing tails. Free steps are fixed at action start.
inline bool Advance(State& s, int tile, bool pearl, bool paid) {
  if (tile<0 || Contains(s.body,tile) || Contains(s.children,tile) ||
      (paid && s.body.size()<=2)) return false;
  s.body.insert(s.body.begin(),tile);
  if (pearl && !Contains(s.eaten,tile)) s.eaten.push_back(tile);
  else s.body.pop_back();
  if (paid) s.body.pop_back();
  return true;
}
class Search {
 public:
  Search(const Environment& env, int& budget):env_(env),budget_(budget) {}
  Result Run(const State& s, int turns) { horizon_=turns;return Turn(s,turns); }
  int expanded=0;
 private:
  const Environment& env_;int& budget_;int horizon_=0;
  std::unordered_map<std::string,Result> memo_;
  static Result Merge(Result a,Result b) {
    if(a==Result::Alive||b==Result::Alive)return Result::Alive;
    if(a==Result::Unknown||b==Result::Unknown)return Result::Unknown;
    return Result::Dead;
  }
  std::string Key(const State& s,int turns) const {
    std::string key;auto add=[&](int n){key.append(reinterpret_cast<const char*>(&n),sizeof(n));};
    add(turns);add(s.slots);add(s.body.size());for(int t:s.body)add(t);
    auto eaten=s.eaten;std::sort(eaten.begin(),eaten.end());add(eaten.size());for(int t:eaten)add(t);
    auto children=s.children;std::sort(children.begin(),children.end());add(children.size());for(int t:children)add(t);
    return key;
  }
  bool Spend(){if(budget_<=0)return false;--budget_;++expanded;return true;}
  Result Turn(const State& s,int turns) {
    if(turns<=0)return Result::Alive;
    if(!Spend())return Result::Unknown;
    auto key=Key(s,turns);auto found=memo_.find(key);if(found!=memo_.end())return found->second;
    Result result=Move(s,0,(s.body.size()+3)/4,turns);
    // Splitting is also a legal way to survive this turn. The child's body remains.
    if(result!=Result::Alive&&s.slots>0&&s.body.size()>=4){
      for(int n=2;n<=int(s.body.size())-2;++n){
        if(!Spend()){result=Merge(result,Result::Unknown);break;}
        State parent=s;parent.slots--;parent.children.insert(parent.children.end(),parent.body.end()-n,parent.body.end());
        parent.body.resize(parent.body.size()-n);
        auto split_result=Turn(parent,turns-1);
        if(env_.require_move_witness && split_result==Result::Alive)split_result=Result::Unknown;
        result=Merge(result,split_result);if(result==Result::Alive)break;
      }
    }
    if(result!=Result::Alive && s.slots<=0 && s.body.size()>=4 &&
       (env_.slots_may_open || !s.children.empty()))result=Merge(result,Result::Unknown);
    memo_.emplace(std::move(key),result);return result;
  }
  Result Move(const State& s,int used,int free,int turns) {
    if(used>=free&&s.body.size()<=2)return Result::Dead;
    if(used>=env_.max_action_steps)return Result::Unknown;
    Result result=Result::Dead;
    for(int d=0;d<4;++d){
      if(!Spend())return Merge(result,Result::Unknown);
      int tile=env_.step(s.body.front(),d);
      if(tile==-1)continue;
      if(tile<0){result=Merge(result,Result::Unknown);continue;}
      if(Contains(s.body,tile))continue;
      // Other dragons and split children may move before a later turn: never prove
      // death by treating their current bodies as permanently stationary obstacles.
      if(Contains(s.children,tile)||env_.uncertain(tile)||
         env_.pearl_uncertain(tile,horizon_-turns+1,s.eaten)){
        result=Merge(result,Result::Unknown);continue;
      }
      State next=s;if(!Advance(next,tile,env_.pearl(tile),used>=free))continue;
      // A move may end after ANY positive number of steps, not just the free limit.
      result=Merge(result,Turn(next,turns-1));if(result==Result::Alive)return result;
      result=Merge(result,Move(next,used+1,free,turns));if(result==Result::Alive)return result;
    }
    return result;
  }
};
} // namespace queen_survival
