#include <doctest/doctest.h>

#include <kodak/core/Finding.h>

#include <vector>

using kodak::core::Finding;
using kodak::core::FindingKind;
using kodak::core::mergeOverlapping;
using kodak::core::Point;
using kodak::core::Rect;
using kodak::core::toggleAt;
using kodak::core::topmostAt;

TEST_CASE("topmostAt picks the last finding containing the point") {
    const std::vector<Finding> findings{{Rect{0, 0, 10, 10}}, {Rect{5, 5, 10, 10}}, {Rect{20, 20, 2, 2}}};

    CHECK(topmostAt(findings, Point{2, 2}) == 0u);
    CHECK(topmostAt(findings, Point{7, 7}) == 1u); // inside both: the one drawn on top
    CHECK(topmostAt(findings, Point{21, 21}) == 2u);
}

TEST_CASE("topmostAt finds nothing outside every finding") {
    const std::vector<Finding> findings{{Rect{0, 0, 10, 10}}};
    CHECK_FALSE(topmostAt(findings, Point{10, 10}).has_value());
    CHECK_FALSE(topmostAt({}, Point{0, 0}).has_value());
}

TEST_CASE("overlapping findings merge into one, also through a chain") {
    const std::vector<Finding> findings{{Rect{0, 0, 10, 10}, FindingKind::Email},
                                        {Rect{5, 5, 20, 20}, FindingKind::Card},
                                        {Rect{24, 24, 5, 5}, FindingKind::Phone},
                                        {Rect{50, 50, 2, 2}, FindingKind::Ip}};

    const auto merged = mergeOverlapping(findings);
    REQUIRE(merged.size() == 2);
    CHECK(merged[0].area == Rect{0, 0, 29, 29});
    CHECK(merged[0].kind == FindingKind::Card); // the larger area wins
    CHECK(merged[1].area == Rect{50, 50, 2, 2});
    CHECK(merged[1].kind == FindingKind::Ip);
}

TEST_CASE("findings that only touch stay apart") {
    const std::vector<Finding> findings{{Rect{0, 0, 10, 10}}, {Rect{10, 0, 10, 10}}};
    CHECK(mergeOverlapping(findings).size() == 2);
}

TEST_CASE("toggleAt flips the topmost finding under the point") {
    std::vector<Finding> findings{{Rect{0, 0, 10, 10}}, {Rect{5, 5, 10, 10}}};

    CHECK(toggleAt(findings, Point{7, 7}));
    CHECK(findings[0].enabled);
    CHECK_FALSE(findings[1].enabled);

    CHECK(toggleAt(findings, Point{7, 7}));
    CHECK(findings[1].enabled);
}

TEST_CASE("toggleAt outside every finding changes nothing") {
    std::vector<Finding> findings{{Rect{0, 0, 10, 10}}};
    CHECK_FALSE(toggleAt(findings, Point{20, 20}));
    CHECK(findings[0].enabled);
}
