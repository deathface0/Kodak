#include <doctest/doctest.h>

#include <kodak/core/Tiling.h>

#include <algorithm>
#include <vector>

using kodak::core::splitIntoTiles;
using kodak::core::Tile;

TEST_CASE("one tile when the length fits") {
    CHECK(splitIntoTiles(300, 400, 100) == std::vector<Tile>{{0, 300, 0, 300}});
    CHECK(splitIntoTiles(400, 400, 100) == std::vector<Tile>{{0, 400, 0, 400}});
    CHECK(splitIntoTiles(0, 400, 100).empty());
}

TEST_CASE("tiles overlap and split each overlap in the middle") {
    CHECK(splitIntoTiles(900, 400, 200) ==
          std::vector<Tile>{
              {0, 400, 0, 300}, {200, 600, 300, 500}, {400, 800, 500, 650}, {500, 900, 650, 900}});
}

TEST_CASE("tiles respect the maximum and own every position exactly once") {
    struct Case {
        int length;
        int maxTile;
        int overlap;
    };
    for (const Case c : {Case{900, 400, 200}, Case{1000, 400, 100}, Case{10000, 2048, 256},
                         Case{401, 400, 256}, Case{5, 1, 0}, Case{7000, 2048, 5000}}) {
        INFO("length ", c.length, ", max ", c.maxTile, ", overlap ", c.overlap);
        const std::vector<Tile> tiles = splitIntoTiles(c.length, c.maxTile, c.overlap);
        REQUIRE_FALSE(tiles.empty());
        CHECK(tiles.front().begin == 0);
        CHECK(tiles.back().end == c.length);
        CHECK(tiles.front().ownBegin == 0);
        CHECK(tiles.back().ownEnd == c.length);
        for (std::size_t i = 0; i < tiles.size(); ++i) {
            CHECK(tiles[i].end - tiles[i].begin <= c.maxTile);
            CHECK(tiles[i].begin <= tiles[i].ownBegin);
            CHECK(tiles[i].ownBegin < tiles[i].ownEnd);
            CHECK(tiles[i].ownEnd <= tiles[i].end);
            if (i > 0) {
                CHECK(tiles[i].ownBegin == tiles[i - 1].ownEnd);
                CHECK(tiles[i - 1].end - tiles[i].begin >= std::min(c.overlap, c.maxTile / 2));
            }
        }
    }
}
