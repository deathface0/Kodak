#include "TextTestSupport.h"

using kodak::core::FindingKind;
using kodak::test::contains;
using kodak::test::found;
using kodak::test::Texts;

TEST_CASE("e-mail addresses") {
    CHECK(found(U"Contact juan.perez@gmail.com today", FindingKind::Email) == Texts{U"juan.perez@gmail.com"});
    CHECK(found(U"mail: a.b-c+tag@sub.example.co.uk.", FindingKind::Email) ==
          Texts{U"a.b-c+tag@sub.example.co.uk"});
    CHECK(found(U"user@localhost", FindingKind::Email).empty());
    CHECK(found(U"user@domain.c", FindingKind::Email).empty());
    CHECK(found(U"@example.com", FindingKind::Email).empty());
    CHECK(found(U"name@.com", FindingKind::Email).empty());
}

TEST_CASE("card numbers pass Luhn, with or without separators") {
    CHECK(found(U"Card 4111 1111 1111 1111 exp", FindingKind::Card) == Texts{U"4111 1111 1111 1111"});
    CHECK(found(U"4111-1111-1111-1111", FindingKind::Card) == Texts{U"4111-1111-1111-1111"});
    CHECK(found(U"4111111111111111", FindingKind::Card) == Texts{U"4111111111111111"});
    CHECK(found(U"4111 1111 1111 1111 12/25", FindingKind::Card) == Texts{U"4111 1111 1111 1111"});
}

TEST_CASE("card candidates that fail Luhn, are too short or sit inside a word are ignored") {
    CHECK(found(U"4111 1111 1111 1112", FindingKind::Card).empty());
    CHECK(found(U"4111 1111 1111", FindingKind::Card).empty());
    CHECK(found(U"ref4111111111111111", FindingKind::Card).empty());
}

TEST_CASE("a card misread by the OCR is still found, with the original text range") {
    CHECK(found(U"4111 l111 1111 1111", FindingKind::Card) == Texts{U"4111 l111 1111 1111"});
}

TEST_CASE("IBANs with valid check digits") {
    CHECK(found(U"IBAN: ES91 2100 0418 4502 0005 1332 ok", FindingKind::Iban) ==
          Texts{U"ES91 2100 0418 4502 0005 1332"});
    CHECK(found(U"ES9121000418450200051332", FindingKind::Iban) == Texts{U"ES9121000418450200051332"});
    CHECK(found(U"DE89 3704 0044 0532 0130 00", FindingKind::Iban) == Texts{U"DE89 3704 0044 0532 0130 00"});
    CHECK(found(U"GB82 WEST 1234 5698 7654 32", FindingKind::Iban) == Texts{U"GB82 WEST 1234 5698 7654 32"});
    CHECK(found(U"ES91 2100 0418 4502 0005 1332 EUR", FindingKind::Iban) ==
          Texts{U"ES91 2100 0418 4502 0005 1332"});
    CHECK(found(U"ES92 2100 0418 4502 0005 1332", FindingKind::Iban).empty());
}

TEST_CASE("DNI and NIE with their control letter") {
    CHECK(found(U"DNI 12345678Z", FindingKind::DniNie) == Texts{U"12345678Z"});
    CHECK(found(U"DNI: 12345678-Z.", FindingKind::DniNie) == Texts{U"12345678-Z"});
    CHECK(found(U"NIE X1234567L", FindingKind::DniNie) == Texts{U"X1234567L"});
    CHECK(found(U"12345678A", FindingKind::DniNie).empty());
    CHECK(found(U"123456789Z", FindingKind::DniNie).empty());
    CHECK(found(U"12345678 Zaragoza", FindingKind::DniNie).empty());
}

TEST_CASE("Spanish phone numbers") {
    CHECK(found(U"Tel 612 345 678", FindingKind::Phone) == Texts{U"612 345 678"});
    CHECK(found(U"+34 612 345 678", FindingKind::Phone) == Texts{U"+34 612 345 678"});
    CHECK(found(U"+34612345678", FindingKind::Phone) == Texts{U"+34612345678"});
    CHECK(found(U"0034 612345678", FindingKind::Phone) == Texts{U"0034 612345678"});
    CHECK(found(U"612.345.678", FindingKind::Phone) == Texts{U"612.345.678"});
    CHECK(found(U"912-34-56-78", FindingKind::Phone) == Texts{U"912-34-56-78"});
}

TEST_CASE("numbers that are not phones") {
    CHECK(found(U"512 345 678", FindingKind::Phone).empty());    // no Spanish number starts with 5
    CHECK(found(U"1612345678", FindingKind::Phone).empty());     // 10 digits
    CHECK(found(U"order 61234567", FindingKind::Phone).empty()); // 8 digits
}

TEST_CASE("IPv4 addresses") {
    CHECK(found(U"server 192.168.1.20.", FindingKind::Ip) == Texts{U"192.168.1.20"});
    CHECK(found(U"10.0.0.1:8080", FindingKind::Ip) == Texts{U"10.0.0.1"});
    CHECK(found(U"256.1.1.1", FindingKind::Ip).empty());
    CHECK(found(U"version 1.2.3", FindingKind::Ip).empty());
    CHECK(found(U"1.2.3.4.5", FindingKind::Ip).empty());
    CHECK(found(U"v1.2.3.4", FindingKind::Ip).empty());
}

TEST_CASE("secrets with a known format") {
    CHECK(contains(found(U"aws AKIAIOSFODNN7EXAMPLE", FindingKind::Secret), U"AKIAIOSFODNN7EXAMPLE"));
    CHECK(contains(found(U"ghp_a1B2c3D4e5F6g7H8i9J0k1L2m3N4o5P6q7R8", FindingKind::Secret),
                   U"ghp_a1B2c3D4e5F6g7H8i9J0k1L2m3N4o5P6q7R8"));
    CHECK(contains(found(U"key=AIzaSyD-abcdefghijklmnopqrstuvwxyz01234", FindingKind::Secret),
                   U"AIzaSyD-abcdefghijklmnopqrstuvwxyz01234"));
    CHECK(contains(found(U"sk_live_4eC39HqLyjWDarjtT1zdp7dc", FindingKind::Secret),
                   U"sk_live_4eC39HqLyjWDarjtT1zdp7dc"));
    CHECK(contains(
        found(U"Authorization: Bearer "
              U"eyJhbGciOiJIUzI1NiJ9.eyJzdWIiOiIxMjM0In0.dozjgNryP4J3jVmNHl0w5N_XgL0n3I9PlFUP0THsR8U",
              FindingKind::Secret),
        U"eyJhbGciOiJIUzI1NiJ9.eyJzdWIiOiIxMjM0In0.dozjgNryP4J3jVmNHl0w5N_XgL0n3I9PlFUP0THsR8U"));
    CHECK(contains(found(U"-----BEGIN RSA PRIVATE KEY-----", FindingKind::Secret),
                   U"-----BEGIN RSA PRIVATE KEY-----"));
}

TEST_CASE("almost-secrets are not flagged") {
    CHECK(found(U"AKIAIOSFODNN7EXAMPL", FindingKind::Secret).empty());   // one character short
    CHECK(found(U"AAKIAIOSFODNN7EXAMPLE", FindingKind::Secret).empty()); // inside a longer word
    CHECK(found(U"-----BEGIN PUBLIC KEY-----", FindingKind::Secret).empty());
}

TEST_CASE("random-looking tokens are flagged as secrets") {
    CHECK(found(U"pass Zq8vR2mN5tX1pL9wK3sB7dF4 end", FindingKind::Secret) ==
          Texts{U"Zq8vR2mN5tX1pL9wK3sB7dF4"});
    CHECK(found(U"9f8Kq2LmZx7Vb3Nc4Rt6Yh1Wd5Sg0Pj", FindingKind::Secret) ==
          Texts{U"9f8Kq2LmZx7Vb3Nc4Rt6Yh1Wd5Sg0Pj"});
    CHECK(found(U"dGhpcyBpcyBhIHNlY3JldCBrZXkgMTIz==", FindingKind::Secret) ==
          Texts{U"dGhpcyBpcyBhIHNlY3JldCBrZXkgMTIz=="});
}

TEST_CASE("ordinary long words, identifiers, hashes and paths are not") {
    CHECK(found(U"ThisIsAVeryLongIdentifierName", FindingKind::Secret).empty());            // no digit
    CHECK(found(U"getUserById2FactorAuthentication", FindingKind::Secret).empty());         // readable
    CHECK(found(U"123e4567-e89b-12d3-a456-426614174000", FindingKind::Secret).empty());     // UUID
    CHECK(found(U"da39a3ee5e6b4b0d3255bfef95601890afd80709", FindingKind::Secret).empty()); // SHA-1
    CHECK(found(U"C:/Users/Usuario/Desktop/Kodak/build2", FindingKind::Secret).empty());
    CHECK(found(U"Short1Aa", FindingKind::Secret).empty());
}

TEST_CASE("the user's own terms, ignoring case and accents") {
    CHECK(found(U"Firmado: Jos\u00E9 P\u00E9rez Garc\u00EDa", FindingKind::UserTerm, {U"jose perez"}) ==
          Texts{U"Jos\u00E9 P\u00E9rez"});
    CHECK(found(U"Banana Ana", FindingKind::UserTerm, {U"Ana"}) == Texts{U"Ana"});
    CHECK(found(U"Calle Mayor 5", FindingKind::UserTerm, {U"  calle mayor  "}) == Texts{U"Calle Mayor"});
    CHECK(found(U"Jose Perez", FindingKind::UserTerm).empty()); // no terms configured
}

TEST_CASE("values after a label are redacted to the end of the line") {
    CHECK(found(U"Contrase\u00F1a: hunter2", FindingKind::ContextValue) == Texts{U"hunter2"});
    CHECK(found(U"password=Sup3r S3cret  ", FindingKind::ContextValue) == Texts{U"Sup3r S3cret"});
    CHECK(found(U"PIN : 1234", FindingKind::ContextValue) == Texts{U"1234"});
    CHECK(found(U"API key: abc-123", FindingKind::ContextValue) == Texts{U"abc-123"});
    CHECK(found(U"Clave = 9876", FindingKind::ContextValue) == Texts{U"9876"});
    CHECK(found(U"CVV:123", FindingKind::ContextValue) == Texts{U"123"});
}

TEST_CASE("short password labels count too") {
    // As the Windows OCR read a real search box: the trailing 1 is the text cursor.
    CHECK(found(U"Q pass: lacasaroja121", FindingKind::ContextValue) == Texts{U"lacasaroja121"});
    CHECK(found(U"passwd=hunter2", FindingKind::ContextValue) == Texts{U"hunter2"});
    CHECK(found(U"PWD: 1234abcd", FindingKind::ContextValue) == Texts{U"1234abcd"});
    CHECK(found(U"Client secret: s3cr3t-value", FindingKind::ContextValue) == Texts{U"s3cr3t-value"});
    // "password" is not also matched as "pass" followed by "word".
    CHECK(found(U"password: abc", FindingKind::ContextValue) == Texts{U"abc"});
}

TEST_CASE("labels without a value or inside other words are ignored") {
    CHECK(found(U"pinned: yes", FindingKind::ContextValue).empty());
    CHECK(found(U"Forgot your password?", FindingKind::ContextValue).empty());
    CHECK(found(U"password:   ", FindingKind::ContextValue).empty());
    CHECK(found(U"passport: valid", FindingKind::ContextValue).empty());
    CHECK(found(U"Mountain pass is open", FindingKind::ContextValue).empty());
}

TEST_CASE("one line with every kind of data") {
    const std::u32string_view line =
        U"juan@mail.com 4111111111111111 ES9121000418450200051332 12345678Z 612345678 10.0.0.1";
    CHECK(found(line, FindingKind::Email) == Texts{U"juan@mail.com"});
    CHECK(found(line, FindingKind::Card) == Texts{U"4111111111111111"});
    CHECK(found(line, FindingKind::Iban) == Texts{U"ES9121000418450200051332"});
    CHECK(found(line, FindingKind::DniNie) == Texts{U"12345678Z"});
    CHECK(found(line, FindingKind::Phone) == Texts{U"612345678"});
    CHECK(found(line, FindingKind::Ip) == Texts{U"10.0.0.1"});
}

TEST_CASE("disabled kinds have no rules") {
    using kodak::core::defaultRules;
    auto count = [](const std::set<FindingKind>& disabled, Texts terms = {}) {
        int matches = 0;
        for (const auto& rule : defaultRules(std::move(terms), disabled))
            matches += static_cast<int>(rule(U"juan@mail.com Zq8vR2mN5tX1pL9wK3sB7dF4 Ana").size());
        return matches;
    };
    CHECK(count({}, {U"Ana"}) == 3);
    CHECK(count({FindingKind::Email}, {U"Ana"}) == 2);
    CHECK(count({FindingKind::Secret}, {U"Ana"}) == 2);   // the entropy rule goes too
    CHECK(count({FindingKind::UserTerm}, {U"Ana"}) == 2); // terms configured but turned off
}
