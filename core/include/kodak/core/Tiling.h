#pragma once

#include <vector>

namespace kodak::core {

/// One tile along an axis: it covers [begin, end) and owns [ownBegin, ownEnd).
/// Owned ranges split each overlap in the middle and cover the whole length
/// exactly once, so something cut by one tile edge is kept, whole, by the
/// neighbour whose owned range holds its centre.
struct Tile {
    int begin = 0;
    int end = 0;
    int ownBegin = 0;
    int ownEnd = 0;

    friend bool operator==(const Tile&, const Tile&) = default;
};

/// Splits [0, length) into tiles of at most `maxTile` that overlap by at least
/// `overlap` (clamped to half a tile). One tile when it already fits.
std::vector<Tile> splitIntoTiles(int length, int maxTile, int overlap);

} // namespace kodak::core
