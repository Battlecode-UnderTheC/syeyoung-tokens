#include "giant.hpp"

#include <algorithm>
#include <cmath>

#include "plugin.hpp"

namespace giant {

using strat::kBlocked;
using strat::kUnknown;

namespace {
constexpr int32_t kFar = 1 << 20;
}

GiantAgent::GiantAgent(const rl::Spawn& spawn, Params params)
    : p_(params), spawn_(spawn), board_(spawn.width, spawn.height) {}

void GiantAgent::Remember(const rl::Observation& o) {
  board_.Update(o);
  head_ = board_.Tile(o.head_x(), o.head_y());
  length_ = o.length;
  units_ = o.unit_count;
  round_ = o.round;
  if (first_turn_) {
    first_turn_ = false;
    for (const rl::Segment& s : o.segments)
      if (s.id == spawn_.id) body_.push_back(board_.Tile(s.x, s.y));
    blacklist_until_.assign(board_.size(), -1);
  }
  if (body_.empty() || body_.front() != head_) body_.push_front(head_);
  while (static_cast<int32_t>(body_.size()) > length_) body_.pop_back();

  const int32_t n = board_.size();
  own_idx_.assign(n, -1);
  for (int32_t i = static_cast<int32_t>(body_.size()) - 1; i >= 0; --i) own_idx_[body_[i]] = i;
  other_.assign(n, 0);
  enemy_heads_.clear();
  ally_heads_.clear();
  seen_len_.clear();
  head_of_.clear();
  for (const rl::Segment& s : o.segments) {
    if (s.id == spawn_.id) continue;
    const int32_t t = board_.Tile(s.x, s.y);
    other_[t] = 1;
    ++seen_len_[s.id];
    if (s.head) {
      head_of_[s.id] = t;
      (s.team == spawn_.team ? ally_heads_ : enemy_heads_).push_back(t);
    }
  }
}

// Steps each tile is from the nearest enemy head, through tiles free of dragons.
void GiantAgent::ComputeThreats() {
  enemy_dist_.assign(board_.size(), kFar);
  std::deque<int32_t> q;
  for (const int32_t h : enemy_heads_) {
    enemy_dist_[h] = 0;
    q.push_back(h);
  }
  while (!q.empty()) {
    const int32_t t = q.front();
    q.pop_front();
    if (enemy_dist_[t] >= p_.threat_radius) continue;
    for (int d = 0; d < 4; ++d) {
      const int32_t n = board_.Step(t, d);
      if (n < 0 || enemy_dist_[n] != kFar) continue;
      enemy_dist_[n] = enemy_dist_[t] + 1;
      if (other_[n] || own_idx_[n] >= 0) continue;   // reached, but not passed through
      q.push_back(n);
    }
  }
}

// Tiles reachable after stepping onto `to`, knowing our own segments move away in time:
// segment i of today's body is gone once enough further steps are taken.
int32_t GiantAgent::TimeRoom(int32_t to, bool grow, int32_t limit) const {
  const int32_t new_len = length_ + (grow ? 1 : 0);
  std::vector<int32_t> dist(board_.size(), -1);
  std::deque<int32_t> q{to};
  dist[to] = 0;
  int32_t count = 1;
  while (!q.empty() && count < limit) {
    const int32_t t = q.front();
    q.pop_front();
    for (int d = 0; d < 4; ++d) {
      const int32_t n = board_.Step(t, d);
      if (n == kUnknown) return limit;
      if (n < 0 || dist[n] >= 0 || other_[n]) continue;
      if (enemy_dist_[n] <= 1) continue;
      const int32_t i = own_idx_[n];
      if (i >= 0 && dist[t] + 1 < new_len - i - 1) continue;
      dist[n] = dist[t] + 1;
      ++count;
      q.push_back(n);
    }
  }
  return std::min(count, limit);
}

int32_t GiantAgent::ChooseGoal(const std::vector<int32_t>& dist) const {
  int32_t best = -1;
  double best_score = 0;
  for (int32_t t = 0; t < board_.size(); ++t) {
    const int32_t d = dist[t];
    if (d <= 0 || blacklist_until_[t] > round_) continue;
    const strat::TileMemory& m = board_.at(t);
    double score = 0;
    if (m.seen_round >= 0) {
      const int32_t age = round_ - m.seen_round;
      int32_t ready = d;
      double value = 0;
      if (m.pearl) {
        value = std::pow(p_.pearl_decay, age);
      } else if (m.countdown >= 0) {
        const int32_t due = m.countdown - age;
        if (due > 0) {
          ready = std::max(d, due);
          value = 1.0;
        } else {
          value = age == 0 ? 1.0 : p_.ripe_bed_value;
        }
      }
      if (enemy_dist_[t] <= 2) value *= 0.3;
      score = value / (ready + 1);
    }
    for (int dir = 0; dir < 4; ++dir)
      if (board_.Step(t, dir) == kUnknown) {
        score = std::max(score, p_.explore_value / (d + 1));
        break;
      }
    if (score > best_score) {
      best_score = score;
      best = t;
    }
  }
  return best;
}

bot::Action GiantAgent::Turn(const rl::Observation& o) {
  Remember(o);
  ComputeThreats();

  // a stub with allies left: any head-to-head is a good trade
  if (length_ <= p_.stub_length && units_ >= 2)
    for (int d = 0; d < 4; ++d) {
      const int32_t to = board_.Step(head_, d);
      if (to >= 0 && std::find(enemy_heads_.begin(), enemy_heads_.end(), to) != enemy_heads_.end())
        return Because("stub: trade", bot::Action::Move(d));
    }

  // late: a small dragon gives its body to a bigger ally in view
  int32_t feed_target = -1;
  if (round_ >= p_.feed_from_round && length_ <= p_.feed_max_length && units_ >= 2) {
    int32_t best_len = length_;
    for (const auto& [id, t] : head_of_) {
      const strat::TileMemory& m = board_.at(t);
      if (m.occupant_team != spawn_.team) continue;
      const int32_t len = seen_len_[id];
      if (len > best_len) {
        best_len = len;
        feed_target = t;
      }
    }
  }

  // paths from the head, knowing our tail moves away
  const int32_t n = board_.size();
  std::vector<int32_t> dist(n, -1);
  std::vector<int8_t> first(n, -1);
  {
    std::deque<int32_t> q{head_};
    dist[head_] = 0;
    while (!q.empty()) {
      const int32_t t = q.front();
      q.pop_front();
      for (int d = 0; d < 4; ++d) {
        const int32_t m = board_.Step(t, d);
        if (m < 0 || dist[m] >= 0) continue;
        if (m == feed_target) {
          dist[m] = dist[t] + 1;
          first[m] = static_cast<int8_t>(t == head_ ? d : first[t]);
          continue;
        }
        if (other_[m] || enemy_dist_[m] <= 1) continue;
        const int32_t i = own_idx_[m];
        if (i >= 0 && dist[t] + 1 < length_ - i) continue;
        dist[m] = dist[t] + 1;
        first[m] = static_cast<int8_t>(t == head_ ? d : first[t]);
        q.push_back(m);
      }
    }
  }
  if (feed_target >= 0 && dist[feed_target] >= 0 && dist[feed_target] <= 2)
    return Because("feed ally", bot::Action::Resign());

  int32_t goal = feed_target >= 0 && dist[feed_target] > 0 ? feed_target : ChooseGoal(dist);
  const int preferred = goal >= 0 ? first[goal] : -1;

  struct Cand {
    int d;
    int32_t to, room, edist;
    bool lethal;
  };
  std::vector<Cand> cands;
  const int32_t need = std::max(length_ + p_.room_margin, p_.room_min);
  bool unknown_step[4] = {false, false, false, false};
  for (int d = 0; d < 4; ++d) {
    const int32_t to = board_.Step(head_, d);
    if (to == kUnknown) unknown_step[d] = true;
    if (to < 0 || own_idx_[to] >= 0 || other_[to]) continue;
    const bool grow = board_.Current(to) && board_.at(to).pearl;
    const int32_t edist = enemy_dist_[to];
    cands.push_back({d, to, TimeRoom(to, grow, need), edist, edist <= 1});
  }

  const Cand* best = nullptr;
  bool evade = false;
  for (const Cand& c : cands)
    if (c.edist < p_.evade_distance) evade = true;
  auto capped = [&](const Cand& c) { return std::min(c.edist, p_.evade_distance + 1); };
  for (const Cand& c : cands) {
    if (c.lethal || c.room < need) continue;
    if (!evade && c.d == preferred) return Because("toward goal", bot::Action::Move(c.d));
    if (!best) {
      best = &c;
      continue;
    }
    if (evade) {
      if (capped(c) != capped(*best)) {
        if (capped(c) > capped(*best)) best = &c;
        continue;
      }
      if ((c.d == preferred) != (best->d == preferred)) {
        if (c.d == preferred) best = &c;
        continue;
      }
    }
    if (c.room > best->room) best = &c;
  }
  if (best) {
    if (!evade && goal >= 0 && preferred >= 0) blacklist_until_[goal] = round_ + 20;
    return Because(evade ? "evade" : preferred >= 0 ? "detour" : "roomiest", bot::Action::Move(best->d));
  }

  // no step is both out of reach and roomy
  const Cand* roomiest = nullptr;
  for (const Cand& c : cands)
    if (!c.lethal && (!roomiest || c.room > roomiest->room)) roomiest = &c;
  const bool can_split = length_ >= 5 && units_ < spawn_.unit_limit;
  if (roomiest && (roomiest->room * 2 >= need || !can_split))
    return Because("tight: roomiest", bot::Action::Move(roomiest->d));
  if (can_split) return Because("trapped: split", bot::Action::Split(length_ - 2));
  if (!cands.empty()) {
    const Cand* c = &cands[0];
    for (const Cand& x : cands)
      if (x.room > c->room) c = &x;
    return Because("cornered", bot::Action::Move(c->d));
  }
  for (int d = 0; d < 4; ++d) {
    const int32_t to = board_.Step(head_, d);
    if (to >= 0 && std::find(enemy_heads_.begin(), enemy_heads_.end(), to) != enemy_heads_.end())
      return Because("trapped: trade", bot::Action::Move(d));
  }
  for (int d = 0; d < 4; ++d)
    if (unknown_step[d]) return Because("trapped: unknown portal", bot::Action::Move(d));
  if (length_ >= 4 && units_ < spawn_.unit_limit) return Because("trapped: split", bot::Action::Split(length_ - 2));
  return Because("trapped: no move", bot::Action::Resign());
}

}  // namespace giant

LEAGUE_VARIANT(giant::GiantAgent)
