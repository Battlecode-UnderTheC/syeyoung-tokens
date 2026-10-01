// engine/src/config.cc (MapReader) and the edge helpers of engine/src/helpers.cc,
// resolved into the Board's lookup tables. Error messages are the engine's.
#include <algorithm>
#include <map>
#include <optional>
#include <sstream>

#include "dragons/core.hpp"

#define DRAGONS_ASSERT(cond, msg)                  \
  do {                                             \
    if (!(cond)) {                                 \
      std::ostringstream err_;                     \
      err_ << msg;                                 \
      throw std::runtime_error(err_.str());        \
    }                                              \
  } while (0)

namespace dragons {
namespace {

enum class Orientation : uint8_t { Horizontal, Vertical };
enum class EdgeKind : uint8_t { Empty, Kelp, Portal };

struct EdgeIndex {
  Orientation o;
  int x, y;
  bool operator==(const EdgeIndex& e) const { return o == e.o && x == e.x && y == e.y; }
};

struct Edge {
  EdgeKind kind = EdgeKind::Empty;
  int portal_id = -1;
  EdgeIndex partner{Orientation::Horizontal, -1, -1};
};

struct Point {
  int x, y;
  bool operator==(const Point& p) const { return x == p.x && y == p.y; }
};

class Reader {
 public:
  int w = 0, h = 0, unit_limit = kDefaultUnitLimit;
  Symmetry symmetry = Symmetry::None;
  std::vector<uint8_t> spawns;
  std::vector<int32_t> min_gap, max_gap;
  std::vector<Edge> hedges, vedges;   // [y * w + x]
  std::vector<std::vector<Point>> bodies;
  std::vector<uint8_t> teams;
  int declared_tiles = -1, declared_edges = -1, declared_dragons = -1, tiles_read = 0, edges_read = 0;
  std::map<int, std::vector<EdgeIndex>> portal_ends;
  std::vector<int> occupied;          // dragon index per tile, -1

  Point Wrap(Point p) const { return {((p.x % w) + w) % w, ((p.y % h) + h) % h}; }

  // engine: EdgeOnTileSide
  EdgeIndex EdgeOnTileSide(Point tile, int dir) const {
    const Point from = Wrap(tile);
    switch (dir) {
      case 0: return {Orientation::Horizontal, from.x, from.y};
      case 2: return {Orientation::Horizontal, from.x, Wrap({from.x, from.y + 1}).y};
      case 3: return {Orientation::Vertical, from.x, from.y};
      default: return {Orientation::Vertical, Wrap({from.x + 1, from.y}).x, from.y};
    }
  }
  Edge& EdgeAt(EdgeIndex e) { return e.o == Orientation::Horizontal ? hedges[e.y * w + e.x] : vedges[e.y * w + e.x]; }
  const Edge& EdgeAt(EdgeIndex e) const { return e.o == Orientation::Horizontal ? hedges[e.y * w + e.x] : vedges[e.y * w + e.x]; }

  // engine: EdgeIndexOf
  std::optional<EdgeIndex> EdgeIndexOf(int index) const {
    const int stride = w + 1, column = index % stride, row = index / stride;
    if (index < 0 || row > 2 * h) return std::nullopt;
    if (row % 2 == 0) {
      if (column == w || row == 2 * h) return std::nullopt;
      return EdgeIndex{Orientation::Horizontal, column, row / 2};
    }
    if (column == w) return std::nullopt;
    return EdgeIndex{Orientation::Vertical, column, (row - 1) / 2};
  }

  // engine: TileAfterCrossing / TileAfterStep
  Point TileAfterCrossing(EdgeIndex e, int heading) const {
    if (e.o == Orientation::Horizontal) return Wrap({e.x, heading == 2 ? e.y : e.y - 1});
    return Wrap({heading == 1 ? e.x : e.x - 1, e.y});
  }
  std::optional<Point> TileAfterStep(Point from, int dir) const {
    const Edge& b = EdgeAt(EdgeOnTileSide(from, dir));
    switch (b.kind) {
      case EdgeKind::Kelp: return std::nullopt;
      case EdgeKind::Portal: return TileAfterCrossing(b.partner, dir);
      default: return Wrap({from.x + kDx[dir], from.y + kDy[dir]});
    }
  }
  std::optional<int> DirectionOfStepBetween(Point from, Point to) const {
    for (int d = 0; d < 4; ++d) {
      const auto t = TileAfterStep(from, d);
      if (t && *t == to) return d;
    }
    return std::nullopt;
  }

  Point Mirror(Point p) const {
    switch (symmetry) {
      case Symmetry::X: return {p.x, h - 1 - p.y};
      case Symmetry::Y: return {w - 1 - p.x, p.y};
      case Symmetry::XY: return {w - 1 - p.x, h - 1 - p.y};
      default: return p;
    }
  }

  bool InBounds(Point p) const { return p.x >= 0 && p.x < w && p.y >= 0 && p.y < h; }

  void ReadLine(const std::string& line) {
    std::istringstream fields(line);
    std::string keyword;
    fields >> keyword;
    if (keyword == "MAP") {
      DRAGONS_ASSERT(w == 0, "MAP appears twice: " << line);
      DRAGONS_ASSERT(fields >> w >> h, "MAP needs a width and a height: " << line);
      DRAGONS_ASSERT(w >= kVision && h >= kVision,
                     "map must be at least " << kVision << "x" << kVision << ", got " << w << "x" << h);
      DRAGONS_ASSERT(w <= kMaxMapSide && h <= kMaxMapSide,
                     "map must be at most " << kMaxMapSide << "x" << kMaxMapSide << ", got " << w << "x" << h);
      spawns.assign(w * h, 0);
      min_gap.assign(w * h, 0);
      max_gap.assign(w * h, 0);
      hedges.assign(w * h, Edge{});
      vedges.assign(w * h, Edge{});
      occupied.assign(w * h, -1);
      return;
    }
    DRAGONS_ASSERT(w > 0, "MAP must come before " << keyword);
    if (keyword == "UNIT_LIMIT") {
      DRAGONS_ASSERT((fields >> unit_limit) && unit_limit > 0, "UNIT_LIMIT needs a positive count: " << line);
    } else if (keyword == "SYMMETRY") {
      std::string value, extra;
      DRAGONS_ASSERT((fields >> value) && !(fields >> extra), "SYMMETRY needs one of x, y, xy: " << line);
      if (value == "x") symmetry = Symmetry::X;
      else if (value == "y") symmetry = Symmetry::Y;
      else if (value == "xy") symmetry = Symmetry::XY;
      else DRAGONS_ASSERT(false, "SYMMETRY must be x, y or xy: " << line);
    } else if (keyword == "TILE_COUNT") {
      DRAGONS_ASSERT(fields >> declared_tiles, "TILE_COUNT needs a count: " << line);
    } else if (keyword == "EDGE_COUNT") {
      DRAGONS_ASSERT(fields >> declared_edges, "EDGE_COUNT needs a count: " << line);
    } else if (keyword == "DRAGON_COUNT") {
      DRAGONS_ASSERT(fields >> declared_dragons, "DRAGON_COUNT needs a count: " << line);
    } else if (keyword == "TILE") {
      Point at;
      int lo = 0, hi = 0;
      DRAGONS_ASSERT(fields >> at.x >> at.y >> lo >> hi, "TILE needs x y minGap maxGap: " << line);
      DRAGONS_ASSERT(InBounds(at), "TILE out of bounds: " << line);
      DRAGONS_ASSERT(hi >= 0 && lo >= 0 && (hi == 0 || lo <= hi), "TILE has bad gaps: " << line);
      const int t = at.y * w + at.x;
      spawns[t] = hi > 0;
      min_gap[t] = lo;
      max_gap[t] = hi;
      ++tiles_read;
    } else if (keyword == "EDGE") {
      int file_index = 0, kind = 0, portal_id = -1;
      DRAGONS_ASSERT(fields >> file_index >> kind >> portal_id, "EDGE needs index kind portalId: " << line);
      DRAGONS_ASSERT(file_index >= 0 && file_index < (2 * h + 1) * (w + 1), "EDGE index is off the map: " << line);
      const auto index = EdgeIndexOf(file_index);
      if (!index) {   // the editor's wrapped bottom row and right column: skipped silently
        ++edges_read;
        return;
      }
      Edge& edge = EdgeAt(*index);
      switch (kind) {
        case 0: edge.kind = EdgeKind::Empty; break;
        case 1: edge.kind = EdgeKind::Kelp; break;
        case 2:
          DRAGONS_ASSERT(portal_id >= 0, "portal EDGE needs a portal id: " << line);
          edge.portal_id = edge.kind == EdgeKind::Portal ? std::min(edge.portal_id, portal_id) : portal_id;
          edge.kind = EdgeKind::Portal;
          portal_ends[portal_id].push_back(*index);
          break;
        default: DRAGONS_ASSERT(false, "EDGE has unknown kind: " << line);
      }
      ++edges_read;
    } else if (keyword == "DRAGON") {
      int team = 0, count = 0;
      DRAGONS_ASSERT(fields >> team >> count, "DRAGON needs team and segment count: " << line);
      DRAGONS_ASSERT(team == 0 || team == 1, "DRAGON team must be 0 or 1: " << line);
      DRAGONS_ASSERT(count >= kMinDragonLength, "DRAGON is too short: " << line);
      // unswbc 1.2.3: each DRAGON line's team differs from the line before it
      DRAGONS_ASSERT(teams.empty() || teams.back() != team,
                     "DRAGON teams must alternate, so dragons 0 and 1 are on different teams: " << line);
      std::vector<Point> body;
      for (int i = 0; i < count; ++i) {
        Point s;
        DRAGONS_ASSERT(fields >> s.x >> s.y, "DRAGON ran out of segments: " << line);
        DRAGONS_ASSERT(InBounds(s), "DRAGON segment out of bounds: " << line);
        DRAGONS_ASSERT(occupied[s.y * w + s.x] < 0, "DRAGON overlaps another dragon: " << line);
        // The engine only checks against earlier dragons; a body folded onto
        // itself would break the occupancy grid, so it is refused here.
        DRAGONS_ASSERT(std::find(body.begin(), body.end(), s) == body.end(),
                       "DRAGON overlaps itself (not supported by this simulator): " << line);
        body.push_back(s);
      }
      for (const Point& s : body) occupied[s.y * w + s.x] = static_cast<int>(bodies.size());
      bodies.push_back(std::move(body));
      teams.push_back(static_cast<uint8_t>(team));
    } else if (keyword == "MAP_NAME") {
    } else {
      DRAGONS_ASSERT(false, "unknown map line: " << line);
    }
  }

  void Finish() {
    DRAGONS_ASSERT(w > 0, "map has no MAP line");
    DRAGONS_ASSERT(declared_tiles == tiles_read, "TILE_COUNT is " << declared_tiles << " but read " << tiles_read);
    DRAGONS_ASSERT(declared_edges == edges_read, "EDGE_COUNT is " << declared_edges << " but read " << edges_read);
    DRAGONS_ASSERT(declared_dragons == static_cast<int>(bodies.size()),
                   "DRAGON_COUNT is " << declared_dragons << " but read " << bodies.size());
    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; ++x) {
        const Point m = Mirror({x, y});
        const int a = y * w + x, b = m.y * w + m.x;
        const bool same = spawns[a] == spawns[b] && (!spawns[a] || (min_gap[a] == min_gap[b] && max_gap[a] == max_gap[b]));
        DRAGONS_ASSERT(same, "pearl bed at (" << x << ", " << y << ") doesn't match its mirror at (" << m.x << ", " << m.y << ")");
      }
    }
    std::vector<EdgeIndex> seen;
    for (const auto& [pid, ends] : portal_ends) {
      for (const EdgeIndex& end : ends) {
        DRAGONS_ASSERT(std::find(seen.begin(), seen.end(), end) == seen.end(),
                       "portal " << pid << " reuses an edge of another portal at (" << end.x << ", " << end.y << ")");
        seen.push_back(end);
        DRAGONS_ASSERT(EdgeAt(end).kind == EdgeKind::Portal,
                       "portal " << pid << " end at (" << end.x << ", " << end.y << ") was overwritten");
      }
      if (ends.size() == 2)
        DRAGONS_ASSERT(ends[0].o == ends[1].o, "portal " << pid << " joins a horizontal edge to a vertical one");
    }
    for (const auto& [pid, ends] : portal_ends) {
      if (ends.size() != 2) continue;
      Edge& from = EdgeAt(ends[0]);
      Edge& to = EdgeAt(ends[1]);
      from.partner = ends[1];
      to.partner = ends[0];
      from.portal_id = to.portal_id = std::min(from.portal_id, to.portal_id);
    }
    for (const auto& [pid, ends] : portal_ends) {
      if (ends.size() == 2) continue;
      for (const EdgeIndex& end : ends)
        DRAGONS_ASSERT(EdgeAt(end).partner.x >= 0,
                       "portal " << pid << " has " << ends.size() << " end(s) and joins nothing at "
                                 << (end.o == Orientation::Horizontal ? "top of " : "left of ") << "(" << end.x << ", "
                                 << end.y << ")");
    }
    for (size_t d = 0; d < bodies.size(); ++d) {
      for (size_t i = 1; i < bodies[d].size(); ++i)
        DRAGONS_ASSERT(DirectionOfStepBetween(bodies[d][i], bodies[d][i - 1]),
                       "dragon " << d << " segment " << i << " is not adjacent to the one before it");
    }
  }

  static int32_t Code(const Edge& e) {
    switch (e.kind) {
      case EdgeKind::Kelp: return kEdgeKelp;
      case EdgeKind::Portal: return e.portal_id;
      default: return kEdgeEmpty;
    }
  }

  std::shared_ptr<Board> Build(const std::string& text) const {
    auto b = std::make_shared<Board>();
    b->width = w;
    b->height = h;
    b->unit_limit = unit_limit;
    b->symmetry = symmetry;
    b->spawns = spawns;
    b->min_gap = min_gap;
    b->max_gap = max_gap;
    b->text = text;
    b->step_to.assign(static_cast<size_t>(w) * h * 4, -1);
    b->north_code.assign(w * h, kEdgeEmpty);
    b->west_code.assign(w * h, kEdgeEmpty);
    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; ++x) {
        const int t = y * w + x;
        for (int d = 0; d < 4; ++d) {
          const auto to = TileAfterStep({x, y}, d);
          b->step_to[t * 4 + d] = to ? to->y * w + to->x : -1;
        }
        b->north_code[t] = Code(hedges[t]);
        b->west_code[t] = Code(vedges[t]);
      }
    }
    b->block_of.assign(w * h, 0);
    b->block_beds.assign(kBlocks * kBlocks, 0);
    b->block_kelp.assign(kBlocks * kBlocks, 0);
    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; ++x) {
        const int t = y * w + x;
        const int blk = (y * kBlocks / h) * kBlocks + x * kBlocks / w;
        b->block_of[t] = static_cast<uint8_t>(blk);
        b->block_beds[blk] += spawns[t];
        b->block_kelp[blk] += (b->north_code[t] == kEdgeKelp) + (b->west_code[t] == kEdgeKelp);
      }
    }
    // engine: InitPearlCountdowns / PearlTick visit beds row-major, skipping a
    // bed whose mirror comes first (the mirror owns the shared countdown)
    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; ++x) {
        const Point m = Mirror({x, y});
        const int bed = y * w + x, mirror = m.y * w + m.x;
        if (mirror < bed || !spawns[bed]) continue;
        b->pearl_beds.push_back(bed);
        b->pearl_beds.push_back(mirror);
      }
    }
    for (size_t d = 0; d < bodies.size(); ++d) {
      Dragon dragon;
      dragon.id = static_cast<int32_t>(d);
      dragon.team = teams[d];
      dragon.length = static_cast<int32_t>(bodies[d].size());
      uint32_t cap = 8;
      while (cap < static_cast<uint32_t>(dragon.length) + 2) cap <<= 1;
      dragon.ring.assign(cap, -1);
      dragon.mask = cap - 1;
      dragon.front = static_cast<uint32_t>(dragon.length - 1);
      for (int s = 0; s < dragon.length; ++s) dragon.ring[(dragon.front - s) & dragon.mask] = bodies[d][s].y * w + bodies[d][s].x;
      dragon.facing = static_cast<uint8_t>(*DirectionOfStepBetween(bodies[d][1], bodies[d][0]));
      b->dragons.push_back(std::move(dragon));
    }
    return b;
  }
};

}  // namespace

std::shared_ptr<const Board> LoadBoard(const std::string& map_text) {
  Reader r;
  std::istringstream lines(map_text);
  std::string line;
  while (std::getline(lines, line)) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    if (line.empty()) continue;
    if (line == "END") break;
    r.ReadLine(line);
  }
  r.Finish();
  return r.Build(map_text);
}

}  // namespace dragons
