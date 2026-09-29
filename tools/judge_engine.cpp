// The engine as the judge seeds it, for the tools only (sim, match, check).
//
// unswbc 1.2.x (the judge, `unswbc run --seed S`; the wasm engine's
// Game::Game(GameState, unsigned long long seed, ...)) draws pearl gaps from
// std::mt19937_64 seeded with the match seed. The shared port
// (shared/engine/dragons, of unswbc 1.0.0) uses a 32-bit std::mt19937 at the fixed
// kPearlRngSeed, so no pearl after the first draw matches any judge game. The ABI
// header (core.hpp) cannot change, so this file recompiles game.cpp unchanged with
// two substitutions: the member's type becomes std::mt19937_64 and Reset seeds it
// from tools::PearlSeed() (the calling thread's seed, set by tools::Play). Every
// tool links this in place of shared's game.o (it comes first on the link line, so
// the archive's copy is never pulled in); variant plugins keep their own engine.
//
// Both engines are an array of 2496 bytes plus an index, so Game's layout is the
// same either way and code compiled against the header (observation.cpp) reads it
// unchanged.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <memory>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

static_assert(sizeof(std::mt19937_64) == sizeof(std::mt19937) && alignof(std::mt19937_64) == alignof(std::mt19937),
              "Game's layout would change with the judge's RNG");

namespace tools {
uint64_t PearlSeed();
}

#define mt19937 mt19937_64
#define seed(x) seed(::tools::PearlSeed())
#include "../shared/engine/dragons/game.cpp"
#undef seed
#undef mt19937

namespace tools {
namespace {
thread_local uint64_t g_pearl_seed = dragons::kPearlRngSeed;
}
uint64_t PearlSeed() { return g_pearl_seed; }
void SetPearlSeed(uint64_t s) { g_pearl_seed = s; }
}  // namespace tools
