// engine/src/game.cc, actions.cc, pearls.cc, sonar.cc and scoring.cc.
#include <algorithm>
#include <cstdio>
#include <functional>
#include <tuple>

#include "dragons/core.hpp"

namespace dragons {

void Game::Reset(std::shared_ptr<const Board> board) {
  board_ = std::move(board);
  const Board& b = *board_;
  const int n = b.width * b.height;
  dragons_ = b.dragons;
  cells_.assign(n, Cell{});
  block_pearls_.assign(kBlocks * kBlocks, 0);
  block_body_.assign(2 * kBlocks * kBlocks, 0);
  bucket_head_.assign(kMaxRounds, -1);
  bucket_next_.assign(b.pearl_beds.size() / 2, -1);
  deaths_.clear();
  alive_[0] = alive_[1] = 0;
  for (const Dragon& d : dragons_) {
    ++alive_[d.team];
    LayBody(d.id);
  }
  rng_.seed(kPearlRngSeed);
  result_ = Result{};
  done_ = false;
  // engine: Game::Run -- InitPearlCountdowns (before round 0's tick), then round 0
  for (uint32_t i = 0; 2 * i < b.pearl_beds.size(); ++i) SetCountdown(i, DrawGap(b.pearl_beds[2 * i]), -1);
  round_ = 0;
  BeginRound();
  Advance();
}

int32_t Game::DrawGap(int32_t bed) {
  const Board& b = *board_;
  const auto span = static_cast<uint32_t>(b.max_gap[bed] - b.min_gap[bed] + 1);
  return b.min_gap[bed] + static_cast<int32_t>(rng_() % span);
}

// A countdown set to `gap` in round r (-1: before the game) reads gap - (now - r)
// and fires at the first tick where that is <= 0.
void Game::SetCountdown(uint32_t order, int32_t gap, int32_t from_round) {
  const Board& b = *board_;
  cells_[b.pearl_beds[2 * order]].pearl_base = gap + from_round;
  cells_[b.pearl_beds[2 * order + 1]].pearl_base = gap + from_round;
  const int64_t due = static_cast<int64_t>(from_round) + std::max(gap, 1);
  if (due >= kMaxRounds) return;   // never fires within a game
  bucket_next_[order] = bucket_head_[due];
  bucket_head_[due] = static_cast<int32_t>(order);
}

// engine: PearlTick, at the start of every round: the beds due now, in row-major order
void Game::BeginRound() {
  const Board& b = *board_;
  due_now_.clear();
  for (int32_t o = bucket_head_[round_]; o >= 0; o = bucket_next_[o]) due_now_.push_back(static_cast<uint32_t>(o));
  bucket_head_[round_] = -1;
  std::sort(due_now_.begin(), due_now_.end());
  for (const uint32_t order : due_now_) {
    const int32_t bed = b.pearl_beds[2 * order], mirror = b.pearl_beds[2 * order + 1];
    if (!cells_[bed].pearl && cells_[bed].occ < 0) SetPearl(bed, 1);                      // TrySpawnPearl
    if (mirror != bed && !cells_[mirror].pearl && cells_[mirror].occ < 0) SetPearl(mirror, 1);
    SetCountdown(order, DrawGap(bed), round_);
  }
  actor_ = 0;
}

// The next living dragon in index order; a round ends when none is left, and
// the game when ResultAfterRound says so.
void Game::Advance() {
  while (true) {
    while (actor_ < static_cast<int32_t>(dragons_.size()) && !dragons_[actor_].alive) ++actor_;
    if (actor_ < static_cast<int32_t>(dragons_.size())) return;
    result_ = ResultAfterRound(false);
    if (result_.terminated) {
      actor_ = -1;
      done_ = true;
      return;
    }
    ++round_;
    BeginRound();
  }
}

void Game::Act(uint8_t kind, const uint8_t* steps, int nsteps, int32_t split, const Sonar& sonar,
               int32_t protocol) {
  const int32_t d = actor_;
  {
    Dragon& g = dragons_[d];
    g.inbox.clear();
    std::fill(g.echo, g.echo + kEchoes, 0);
    ++g.turns;
    if (protocol >= 0) g.protocol = protocol;   // before a split, so the child inherits it
  }
  if (kind == kMove && nsteps > 0) Move(d, steps, nsteps);
  else if (kind == kSplit) Split(d, split);
  else Kill(d, kNoValidAction);
  if (dragons_[d].alive) {
    const int facing = dragons_[d].facing;
    for (int dir = 0; dir < 4; ++dir) {
      uint64_t word;
      if (sonar.mask >> dir & 1) word = sonar.word[dir];
      else if (sonar.bare && dir == facing) word = static_cast<uint32_t>(sonar.bare_word);
      else continue;
      const uint8_t hit = CastSonar(d, dir, word);
      if (hit != kEchoNothing) ++dragons_[d].echo[hit];
    }
  }
  ++actor_;
  Advance();
}

void Game::PushFront(int32_t d, int32_t tile) {
  Dragon& g = dragons_[d];
  if (static_cast<uint32_t>(g.length) + 1 > g.mask) {
    // grow the ring, keeping every segment at its logical position
    const uint32_t cap = (g.mask + 1) * 2;
    std::vector<int32_t> ring(cap, -1);
    for (int s = 0; s < g.length; ++s) ring[(g.front - s) & (cap - 1)] = g.ring[(g.front - s) & g.mask];
    g.ring.swap(ring);
    g.mask = cap - 1;
  }
  const int32_t old_head = g.Head();
  ++g.front;
  ++g.length;
  g.ring[g.front & g.mask] = tile;
  cells_[tile].occ = d;
  cells_[tile].pos = g.front;
  ++block_body_[g.team * kBlocks * kBlocks + board_->block_of[tile]];
  if (g.length > 1) cells_[old_head].dir = static_cast<uint8_t>(DirectionOfStep(old_head, tile));
}

// occupancy, logical positions and segment facings of a whole body
void Game::LayBody(int32_t d) {
  const Dragon& g = dragons_[d];
  for (int s = 0; s < g.length; ++s) {
    Cell& c = cells_[g.Seg(s)];
    if (c.occ < 0) ++block_body_[g.team * kBlocks * kBlocks + board_->block_of[g.Seg(s)]];
    c.occ = d;
    c.pos = g.front - static_cast<uint32_t>(s);
    if (s > 0) c.dir = static_cast<uint8_t>(DirectionOfStep(g.Seg(s), g.Seg(s - 1)));
  }
}

void Game::PopBack(int32_t d) {
  Dragon& g = dragons_[d];
  cells_[g.Tail()].occ = -1;
  --block_body_[g.team * kBlocks * kBlocks + board_->block_of[g.Tail()]];
  --g.length;
}

// engine: Move (unswbc 1.2.3). The first ceil(L/4) steps are free, L being the
// length when the move starts (pearls eaten on the way do not raise it); every
// later step pays a segment, and a step that cannot be paid for (length 2) is
// the "no valid action" death, after the steps before it were taken.
void Game::Move(int32_t d, const uint8_t* steps, int n) {
  const int free_steps = (dragons_[d].length + 3) >> 2;
  for (int i = 0; i < n; ++i) {
    const bool must_pay = i >= free_steps;
    if (must_pay && dragons_[d].length <= kMinDragonLength) {
      Kill(d, kNoValidAction);
      return;
    }
    Step(d, steps[i], must_pay);
    if (!dragons_[d].alive) return;
  }
}

// engine: Step
void Game::Step(int32_t d, int dir, bool must_pay) {
  Dragon& g = dragons_[d];
  g.facing = static_cast<uint8_t>(dir);
  const int32_t dest = board_->step_to[g.Head() * 4 + dir];
  if (dest < 0) {
    Kill(d, kHitWall);
    return;
  }
  const int32_t other = cells_[dest].occ;
  if (other == d) {
    Kill(d, kHitSelf);
    return;
  }
  if (other >= 0) {
    if (dragons_[other].Head() == dest) {
      Kill(other, kHitHeadToHead);
      Kill(d, kHitHeadToHead);
      return;
    }
    Kill(d, kHitOtherBody, other);
    return;
  }
  PushFront(d, dest);
  if (cells_[dest].pearl) SetPearl(dest, 0);
  else PopBack(d);
  if (must_pay) PopBack(d);
}

// engine: Split
void Game::Split(int32_t d, int32_t child_segments) {
  const int32_t parent_length = dragons_[d].length;
  if (child_segments < kMinDragonLength || child_segments > parent_length - kMinDragonLength) {
    Kill(d, kNoValidAction);
    return;
  }
  if (alive_[dragons_[d].team] >= board_->unit_limit) {
    Kill(d, kNoValidAction);
    return;
  }
  Dragon child;
  child.id = static_cast<int32_t>(dragons_.size());
  child.team = dragons_[d].team;
  child.length = child_segments;
  uint32_t cap = 8;
  while (cap < static_cast<uint32_t>(child_segments) + 2) cap <<= 1;
  child.ring.assign(cap, -1);
  child.mask = cap - 1;
  child.front = static_cast<uint32_t>(child_segments - 1);
  // the parent's tail, reversed: the child's head is the parent's last segment
  {
    const Dragon& p = dragons_[d];
    for (int i = 0; i < child_segments; ++i) child.ring[(child.front - i) & child.mask] = p.Seg(parent_length - 1 - i);
  }
  dragons_[d].length = parent_length - child_segments;
  const int facing = DirectionOfStep(child.Seg(1), child.Seg(0));
  if (facing < 0) throw std::runtime_error("child segments are not adjacent");
  child.facing = static_cast<uint8_t>(facing);
  child.protocol = dragons_[d].protocol;
  ++alive_[child.team];
  dragons_.push_back(std::move(child));
  LayBody(dragons_.back().id);
}

// engine: Kill -- a pearl on every second segment from the head
void Game::Kill(int32_t d, uint8_t reason, int32_t by) {
  Dragon& g = dragons_[d];
  deaths_.push_back(Death{g.id, g.team, reason, round_, by >= 0 ? dragons_[by].id : -1});
  for (int s = 0; s < g.length; ++s) {
    const int32_t tile = g.Seg(s);
    if (s % 2 == 0) SetPearl(tile, 1);
    --block_body_[g.team * kBlocks * kBlocks + board_->block_of[tile]];
    cells_[tile].occ = -1;
  }
  g.alive = false;
  g.length = 0;
  g.inbox.clear();
  --alive_[g.team];
}

// engine: CastSonar (1.0.0). A ping opposite the facing starts at the tail and
// runs the way the tail points (from the second-to-last segment to the last);
// any other starts at the head in `dir`. It goes through portals for up to
// width + height steps, into the first living dragon (itself included), and
// reports what stopped it.
uint8_t Game::CastSonar(int32_t d, int dir, uint64_t value) {
  const Board& b = *board_;
  const Dragon& g = dragons_[d];
  int32_t at = g.Head();
  if (dir == (g.facing + 2) % 4 && g.length > 1) {
    at = g.Tail();
    const int out = DirectionOfStep(g.Seg(g.length - 2), at);
    if (out >= 0) dir = out;
  }
  const int max_steps = b.width + b.height;
  int32_t hit = -1;
  for (int step = 0; step < max_steps && hit < 0; ++step) {
    const int32_t next = b.step_to[at * 4 + dir];
    if (next < 0) return kEchoKelp;
    at = next;
    hit = cells_[at].occ;
  }
  if (hit < 0) return kEchoNothing;
  Dragon& h = dragons_[hit];
  // a word wider than uint32 reaches only a dragon that declared protocol 3+
  if (value <= 0xFFFFFFFFull || h.echoes()) h.inbox.push_back(value);
  const bool head = h.Head() == at;
  if (h.team == g.team) return head ? kEchoAllyHead : kEchoAlly;
  return head ? kEchoEnemyHead : kEchoEnemy;
}

// engine: ResultAfterRound (unswbc 1.2.3): at the round limit the longer queen
// wins, then the longest living dragon, then the greater total length.
Result Game::ResultAfterRound(bool final_round) const {
  Result r;
  for (const Dragon& g : dragons_) {
    if (!g.alive) continue;
    Standing& s = r.team[g.team];
    s.dragons += 1;
    s.longest = std::max(s.longest, g.length);
    s.total += g.length;
    if (g.id <= 1) s.queen = g.length;   // dragons 0 and 1 are the queens
  }
  const bool a_out = r.team[0].dragons == 0, b_out = r.team[1].dragons == 0;
  if (a_out || b_out) {
    r.terminated = true;
    r.end_reason = 0;
    if (a_out != b_out) r.winner = a_out ? 1 : 0;
    return r;
  }
  if (!(final_round || round_ + 1 >= kMaxRounds)) return r;
  r.terminated = true;
  r.end_reason = 1;
  const auto rank = [](const Standing& s) { return std::tuple{s.queen, s.longest, s.total}; };
  if (rank(r.team[0]) > rank(r.team[1])) r.winner = 0;
  else if (rank(r.team[1]) > rank(r.team[0])) r.winner = 1;
  return r;
}

// ---- the judge's text --------------------------------------------------------

std::string Game::InitBlock(int32_t d) const {
  const Dragon& g = dragons_[d];
  char buf[128];
  std::snprintf(buf, sizeof buf, "ID %d\nTEAM %c\nMAP %d %d\nUNIT_LIMIT %d\n", g.id, g.team ? 'B' : 'A',
                board_->width, board_->height, board_->unit_limit);
  return buf;
}

// engine: BuildRoundBlock, literally: every segment of every living dragon is
// tested for vision, as the engine does, independent of the fast observer
std::string Game::RoundBlock() const {
  const Board& b = *board_;
  const Dragon& me = dragons_[actor_];
  const int hx = me.Head() % b.width, hy = me.Head() / b.width;
  std::string out;
  auto num = [&](long long v) { out += std::to_string(v); };
  out += "ROUND "; num(round_);
  out += "\nDIR "; out += kDirChar[me.facing];
  out += "\nLENGTH "; num(me.length);
  out += "\nUNIT_COUNT "; num(alive_[me.team]);
  out += "\nNUM_MSGS "; num(static_cast<long long>(me.inbox.size()));
  out += '\n';
  for (uint64_t m : me.inbox) { out += std::to_string(m); out += '\n'; }
  if (me.echoes()) {
    out += "ECHOES";
    for (int f = 0; f < kEchoes; ++f) { out += ' '; num(me.echo[f]); }
    out += '\n';
  }
  for (int row = 0; row < kVision; ++row) {
    for (int col = 0; col < kVision; ++col) {
      const int32_t t = b.Wrap(hx + col - kVisionRadius, hy + row - kVisionRadius);
      num(t % b.width); out += ' '; num(t / b.width);
      out += cells_[t].pearl ? " 1 " : " 0 ";
      num(next_pearl(t));
      out += '\n';
    }
  }
  auto within = [](int from, int to, int size) {
    const int off = ((to - from) % size + size) % size;
    return off <= kVisionRadius || off >= size - kVisionRadius;
  };
  std::string bodies;
  int count = 0;
  for (const Dragon& o : dragons_) {
    if (!o.alive) continue;
    for (int s = 0; s < o.length; ++s) {
      const int32_t t = o.Seg(s);
      const int x = t % b.width, y = t / b.width;
      if (!within(hx, x, b.width) || !within(hy, y, b.height)) continue;
      const int facing = s == 0 ? o.facing : DirectionOfStep(t, o.Seg(s - 1));
      if (facing < 0) throw std::runtime_error("dragon segments are not adjacent");
      bodies += o.team ? 'B' : 'A';
      bodies += ' ' + std::to_string(o.id) + ' ' + std::to_string(x) + ' ' + std::to_string(y) + ' ';
      bodies += kDirChar[facing];
      bodies += s == 0 ? " 1\n" : " 0\n";
      ++count;
    }
  }
  out += "DRAGON_BODIES "; num(count); out += '\n';
  out += bodies;
  auto edge = [&](int32_t code) {
    if (code == kEdgeKelp) out += 'w';
    else if (code == kEdgeEmpty) out += '.';
    else num(code);
  };
  for (int row = 0; row <= kVision; ++row) {
    for (int col = 0; col < kVision; ++col) {
      // the south side of the last row is the north side of the row below it
      const int32_t t = b.Wrap(hx + col - kVisionRadius, hy + row - kVisionRadius);
      if (col > 0) out += ' ';
      edge(b.north_code[t]);
    }
    out += '\n';
  }
  for (int row = 0; row < kVision; ++row) {
    for (int col = 0; col <= kVision; ++col) {
      const int32_t t = b.Wrap(hx + col - kVisionRadius, hy + row - kVisionRadius);
      if (col > 0) out += ' ';
      edge(b.west_code[t]);
    }
    out += '\n';
  }
  return out;
}

}  // namespace dragons
