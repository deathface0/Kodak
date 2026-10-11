#include "kodak/core/Tiling.h"

#include <algorithm>
#include <cstddef>

namespace kodak::core {

std::vector<Tile> splitIntoTiles(int length, int maxTile, int overlap) {
    if (length <= 0) return {};
    maxTile = std::max(maxTile, 1);
    if (length <= maxTile) return {{0, length, 0, length}};
    overlap = std::clamp(overlap, 0, maxTile / 2);

    std::vector<Tile> tiles;
    for (int begin = 0;; begin += maxTile - overlap) {
        if (begin + maxTile >= length) {
            // The last tile ends at the edge and keeps its full size.
            tiles.push_back({length - maxTile, length, 0, 0});
            break;
        }
        tiles.push_back({begin, begin + maxTile, 0, 0});
    }

    for (std::size_t i = 0; i < tiles.size(); ++i) {
        tiles[i].ownBegin = i == 0 ? 0 : (tiles[i].begin + tiles[i - 1].end) / 2;
        tiles[i].ownEnd = i + 1 == tiles.size() ? length : (tiles[i + 1].begin + tiles[i].end) / 2;
    }
    return tiles;
}

} // namespace kodak::core
