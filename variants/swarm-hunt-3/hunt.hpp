// swarm-hunt-3 (from swarm-hunt-1): many small dragons that hunt enemy heads.
//
// The weapon is the lunge: a MOVE may take up to length-1 steps in one turn (every step
// after the first costs a tail segment), so a dragon of length L can reach an enemy head
// up to L-1 steps away before it can react, and a head-to-head there kills both. Opponents
// that only guard the tiles next to their heads never see it coming.
//
// Roles: dragons alive at round 0 are KEEPERS (the round-499 fallback: they split off
// hunters, never trade except for a longer enemy, keep out of enemy lunge range and grow
// late). Split children are HUNTERS: they gather, split, and lunge at enemy heads.
#pragma once

#include <cstdint>
#include <deque>
#include <unordered_map>
#include <vector>

#include "agent.hpp"
#include "board_memory.hpp"
#include "pathing.hpp"

namespace swh3 {

struct Params {
  int32_t split_min_length = 4;     // split at least this long ...
  int32_t split_until_round = 300;  // ... before this round
  int32_t keeper_split_until = 300; // keepers split before this round
  int32_t lunge = 1;                // 1: multi-step strikes at enemy heads in reach
  int32_t trade_rule = 0;           // hunters trade: 0 for an enemy at least as long; 1 any enemy; 2 any when units >= hunt_min_units
  int32_t trade_margin = 0;
  int32_t hunt_radius = 6;
  int32_t hunt_min_units = 3;
  int32_t keeper_guard = 2;         // keepers keep this many steps from enemy heads (1: adjacency only)
  int32_t crowd_room = 1;           // room checks don't count tiles next to ally heads
  int32_t ally_yield = 1;           // leave targets an ally head is nearer to
  int32_t avoid_heads = 1;          // hunters keep off tiles next to enemy heads they won't trade with
  // targets
  double pearl_decay = 0.97;
  double ripe_bed_value = 0.6;
  double explore_value = 0.25;
  double trap_cost = 3.0;
  int32_t trap_candidates = 8;
  int32_t room_margin = 3;
  int32_t room_min = 8;
  int32_t feed_from_round = 400;
  int32_t feed_distance = 2;
  int32_t ally_memory = 80;
  // spreading
  int32_t unit_reserve = 6;         // normal splits leave this many unit slots for trapped splits
  double crowd_penalty = 0.5;       // goal score /= 1 + penalty * ally segments near the goal
  int32_t split_room = 0;           // 0: off (2 halved splits and lost 13-27 on worldgen)
};

const Params& DefaultParams();

class HuntAgent : public bot::Agent {
 public:
  explicit HuntAgent(const rl::Spawn& spawn, const Params& params = DefaultParams());
  bot::Action Turn(const rl::Observation& o) override;
  const char* reason() const override { return reason_; }

 private:
  struct Sighting {
    int32_t head = -1;
    int32_t round = -1;
    int32_t length = 0;
    uint8_t team = 0;
  };
  enum class Cell { kFree, kWall, kUnknown, kBody, kEnemyHead, kAllyHead };
  struct Goal {
    int32_t tile = -1;
    bool trap = false;
  };

  void Remember(const rl::Observation& o);
  Cell Classify(int32_t tile) const;
  bool Occupied(int32_t tile) const;
  bool Mine(int32_t tile) const;
  bool WorthTrading(int32_t enemy_id) const;
  bool Exposed(int32_t tile) const;
  const Sighting* BiggestAlly() const;
  double TileValue(int32_t t, int32_t dist, int32_t* ready) const;
  int32_t RoomNeeded() const;
  Goal ChooseGoal(const strat::Paths& paths) const;
  bool Lunge(bot::Action* out);
  int32_t CrowdNear(int32_t t) const;   // ally segments within 2 steps of t
  int32_t TorusDistance(int32_t a, int32_t b) const;
  bool AllyCloser(int32_t t, int32_t dist) const;   // an ally head is nearer to t than we are
  bot::Action SafeStep(int preferred_dir, bool trap_ok);
  bot::Action Because(const char* why, bot::Action a) {
    reason_ = why;
    return a;
  }

  Params p_;
  rl::Spawn spawn_;
  strat::BoardMemory board_;
  bool keeper_ = false;
  bool first_turn_ = true;
  int32_t head_ = 0, length_ = 0, units_ = 1, round_ = 0;
  std::unordered_map<int32_t, Sighting> seen_;
  std::deque<int32_t> body_;
  std::vector<int32_t> ally_heads_;
  std::vector<uint8_t> crowd_;      // per tile: next to an ally's head this turn
  std::vector<uint8_t> ally_body_;  // per tile: an ally segment in view this turn
  std::vector<uint8_t> ally_tail_;  // per tile: the last visible segment of an ally (moves away)
  std::vector<int32_t> threat_;     // per tile: 1 if within an enemy head's guard distance this turn
  const char* reason_ = "";
};

}  // namespace swh3
