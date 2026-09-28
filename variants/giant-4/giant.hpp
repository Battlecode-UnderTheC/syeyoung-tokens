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
#include <string>
#include <unordered_map>
#include <vector>

#include "agent.hpp"
#include "board_memory.hpp"

namespace giant4 {

struct Params {
  double pearl_decay = 0.97;     // per round since a pearl was seen
  double ripe_bed_value = 0.6;   // a bed that ran out of countdown out of view
  double explore_value = 0.25;   // the edge of the known map
  int32_t room_margin = 6;       // a step must leave length + this much room (time-aware)
  int32_t room_min = 10;
  int32_t threat_radius = 6;     // enemy heads' reach is computed this far
  int32_t evade_distance = 3;    // closer than this, evading beats the goal
  int32_t stub_length = 3;       // this short (and not alone): trade with any enemy head
  int32_t feed_from_round = 340; // surge: every dragon (giants too) walks to the king and dies next to it (380 measured worse here)
  int32_t feed_max_length = 1000;
  // numbers: the giant splits off helpers early, helpers split too
  int32_t giant_split_until = 100;
  int32_t giant_child_length = 8;  // a child born this long carries on as a giant
  int32_t helper_split_until = 350;  // 400 (late-surge) measured worse here
  int32_t split_min_length = 4;
  // helpers fight: trade with enemy heads at least as long, hunt them this close
  int32_t hunt_radius = 6;
  int32_t hunt_min_units = 3;
  int32_t helper_room_margin = 3;
  int32_t helper_room_min = 8;
  int32_t outnumbered_factor = 2;   // outnumbered (by enemies seen lately), a helper trades only for this many times its length
  int32_t enemy_memory = 60;
  // gossip: every dragon pings the biggest giant it knows of (head tile, round, length)
  int32_t king_ttl = 40;            // rounds a report stays believable
  double king_decay = 0.5;          // reports compare by length less this per round of age
  double demote_ratio = 2;          // a giant this many times shorter than the king becomes a helper
  int32_t feed_early_length = 0;    // a helper this long feeds the giant in view any time after the giant stops splitting (0: off)
  int32_t feed_distance = 3;        // a helper this close (torus steps) to the giant's head dies there
  int32_t intercept_ahead = 2;      // and walks to this many steps in front of it
  double body_time_factor = 1.5;    // room check: own segment frees after (steps until it would) * this (eating stalls the tail)
  int32_t unknown_room = 4;         // room check: an unexplored edge is worth this many tiles (0: a sure way out)
  int32_t strict_early = 1;         // 1: a giant avoids every head even while still splitting
  int32_t anchor_radius = 0;        // late, the king keeps its goals this close to where it was at feed_from_round (0: off)
  int32_t avoid_giant_len = 6;      // helpers keep off a head of an ally this long (or the king) until feeding (0: off)
  int32_t heir_by_sonar = 0;        // a child is a giant only if its giant parent pinged it (else: by its length)
  int32_t promote_length = 10;       // a helper this long, done splitting, becomes a giant (0: never)
  // economy from the gather-split-trade line (g6-5)
  double share_factor = 0.1;        // a target a visible ally head is nearer to is worth this much
  int32_t lunge_reach = 6;          // helpers: a MOVE of up to length-1 steps onto a tradeable enemy head
  int32_t cap_feed_length = 0;      // at the unit cap, a helper this short dies next to a longer ally
  int32_t cap_feed_distance = 2;
  // late-surge endgame (from gather-split-trade-31)
  int32_t champ_freeze = 330;       // the king stops growth-splitting from here
  int32_t giant_feed = 1;           // giants other than the king feed it too (from feed_from_round)
  int32_t giant_feed_fresh = 20;    // ... when the king report is at most this old
  int32_t feed_clear_tenths = 8;    // feed only when our length < this/10 of the king's (in view)
  int32_t trust_visible = 1;        // a king whose whole body is in view has exactly its visible length
  int32_t tight_factor = 1000;         // no roomy step: move anyway if room * this >= need, else split out
};

class GiantAgent : public bot::Agent {
 public:
  explicit GiantAgent(const rl::Spawn& spawn, Params params = {});
  bot::Action Turn(const rl::Observation& o) override;
  const char* reason() const override {
    why_ = std::string(giant_ ? "G " : "h ") + reason_;
    return why_.c_str();
  }

 private:
  void Remember(const rl::Observation& o);
  bot::Action Decide();
  void ComputeThreats();
  int32_t TimeRoom(int32_t to, bool grow, int32_t limit) const;
  int32_t ChooseGoal(const std::vector<int32_t>& dist, int32_t anchor) const;
  int32_t Torus(int32_t a, int32_t b) const;
  bool Lunge(bot::Action* out) const;
  int32_t anchor_ = -1;                       // the king's home late in the game
  bot::Action Because(const char* why, bot::Action a) {
    reason_ = why;
    return a;
  }

  Params p_;
  rl::Spawn spawn_;
  strat::BoardMemory board_;
  bool first_turn_ = true;
  bool demoted_ = false;                      // a giant turned helper (never splits)
  bool strict_ = false;                       // a giant done splitting: avoids every enemy head
  bool giant_ = false;                        // alive at round 0; else a helper
  bool Tradeable(int32_t enemy_head) const;
  int32_t EnemiesSeen() const;
  std::unordered_map<int32_t, int32_t> enemy_seen_;   // enemy id -> last round seen
  struct King {
    int32_t id = -1, tile = -1, round = -1, length = 0;
  };
  King king_;                                 // the biggest giant we heard of
  void Consider(const King& k);
  uint64_t Encode(const King& k) const;
  uint64_t HeirWord() const;
  bool Decode(uint64_t w, King* k) const;
  bool KingKnown() const { return king_.id >= 0 && round_ - king_.round <= p_.king_ttl; }   // a helper gains by a head-to-head with it
  int32_t head_ = 0, length_ = 0, units_ = 1, round_ = 0;
  std::deque<int32_t> body_;                  // own tiles, head first
  // this turn
  std::vector<int32_t> own_idx_;              // index in body_ of each tile, -1 none
  std::vector<uint8_t> other_;                // another dragon's segment (seen now)
  std::vector<int32_t> enemy_dist_;           // steps from the nearest enemy head (big: far)
  std::vector<int32_t> enemy_heads_, ally_heads_, trade_heads_;   // threats, allies, trades we want
  std::unordered_map<int32_t, int32_t> seen_len_;   // visible segments by dragon id
  std::unordered_map<int32_t, bool> cut_;           // its last segment in view is on the window's border
  std::unordered_map<int32_t, int32_t> head_of_;    // head tile by dragon id (this turn)
  std::unordered_map<int32_t, int> head_facing_;
  std::vector<int32_t> head_id_;              // dragon id whose head is on the tile, -1
  std::vector<int32_t> blacklist_until_;      // goal tile -> round it is ignored until
  const char* reason_ = "";
  mutable std::string why_;
};

}  // namespace giant4
