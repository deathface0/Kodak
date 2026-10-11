#include "TextTestSupport.h"

#include <kodak/core/Preview.h>

using kodak::core::FindingKind;
using kodak::core::maskPreview;

TEST_CASE("each kind of data is masked in its own way") {
    CHECK(maskPreview(FindingKind::Email, U"juan.perez@gmail.com") == U"ju***@gmail.com");
    CHECK(maskPreview(FindingKind::Card, U"4532 0151 1283 0366") == U"**** 0366");
    CHECK(maskPreview(FindingKind::Iban, U"ES91 2100 0418 4502 0005 1332") == U"ES91 **** 1332");
    CHECK(maskPreview(FindingKind::DniNie, U"12345678Z") == U"*****678Z");
    CHECK(maskPreview(FindingKind::DniNie, U"X-1234567-L") == U"*****567L");
    CHECK(maskPreview(FindingKind::Phone, U"+34 612 345 678") == U"*** *** 678");
    CHECK(maskPreview(FindingKind::Ip, U"192.168.1.20") == U"192.168.*.*");
    CHECK(maskPreview(FindingKind::Secret, U"ghp_a1B2c3D4e5F6g7H8i9J0k1L2m3N4o5P6q7R8") ==
          U"ghp_*** (40 characters)");
    CHECK(maskPreview(FindingKind::ContextValue, U"hunter2") == U"h*** (7 characters)");
    CHECK(maskPreview(FindingKind::UserTerm, U"Jos\u00E9 P\u00E9rez") == U"J*** (10 characters)");
}

TEST_CASE("text too short to mask safely is hidden completely") {
    CHECK(maskPreview(FindingKind::Email, U"a@b") == U"a***@b");
    CHECK(maskPreview(FindingKind::Email, U"@mail.com") == U"***");
    CHECK(maskPreview(FindingKind::Card, U"1234") == U"***");
    CHECK(maskPreview(FindingKind::Phone, U"123") == U"***");
    CHECK(maskPreview(FindingKind::Iban, U"ES91") == U"***");
    CHECK(maskPreview(FindingKind::Ip, U"10.0") == U"***");
    CHECK(maskPreview(FindingKind::ContextValue, U"abc") == U"***");
    CHECK(maskPreview(FindingKind::Secret, U"short") == U"***");
}

TEST_CASE("manual boxes have no preview") { CHECK(maskPreview(FindingKind::Manual, U"anything").empty()); }
