// giant-1: grow few, long dragons and keep them alive.
//
// Every dragon runs the same logic (no roles need agreeing on): gather pearls, never split
// by choice, keep the head out of reach of every enemy head (a head-to-head kills both),
// and never step where the body could box itself in (a flood fill that knows its own tail
// moves away). Cornered, a dragon splits off almost all of itself: the child walks out
// from the tail, the two-segment stub stays behind as bait or a trade.
#pragma once

#include <cstdint>
#include <deque>
#include <unordered_map>
#include <vector>

#include "agent.hpp"
#include "board_memory.hpp"

namespace giant {

struct Params {
  double pearl_decay = 0.97;     // per round since a pearl was seen
  double ripe_bed_value = 0.6;   // a bed that ran out of countdown out of view
  double explore_value = 0.25;   // the edge of the known map
  int32_t room_margin = 6;       // a step must leave length + this much room (time-aware)
  int32_t room_min = 10;
  int32_t threat_radius = 6;     // enemy heads' reach is computed this far
  int32_t evade_distance = 3;    // closer than this, evading beats the goal
  int32_t stub_length = 3;       // this short (and not alone): trade with any enemy head
  int32_t feed_from_round = 430; // small dragons walk to a bigger ally and die next to it
  int32_t feed_max_length = 6;
};

class GiantAgent : public bot::Agent {
 public:
  explicit GiantAgent(const rl::Spawn& spawn, Params params = {});
  bot::Action Turn(const rl::Observation& o) override;
  const char* reason() const override { return reason_; }

 private:
  void Remember(const rl::Observation& o);
  void ComputeThreats();
  int32_t TimeRoom(int32_t to, bool grow, int32_t limit) const;
  int32_t ChooseGoal(const std::vector<int32_t>& dist) const;
  bot::Action Because(const char* why, bot::Action a) {
    reason_ = why;
    return a;
  }

  Params p_;
  rl::Spawn spawn_;
  strat::BoardMemory board_;
  bool first_turn_ = true;
  int32_t head_ = 0, length_ = 0, units_ = 1, round_ = 0;
  std::deque<int32_t> body_;                  // own tiles, head first
  // this turn
  std::vector<int32_t> own_idx_;              // index in body_ of each tile, -1 none
  std::vector<uint8_t> other_;                // another dragon's segment (seen now)
  std::vector<int32_t> enemy_dist_;           // steps from the nearest enemy head (big: far)
  std::vector<int32_t> enemy_heads_, ally_heads_;
  std::unordered_map<int32_t, int32_t> seen_len_;   // visible segments by dragon id
  std::unordered_map<int32_t, int32_t> head_of_;    // head tile by dragon id (this turn)
  std::vector<int32_t> blacklist_until_;      // goal tile -> round it is ignored until
  const char* reason_ = "";
};

}  // namespace giant
