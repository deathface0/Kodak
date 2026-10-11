#include <doctest/doctest.h>

#include <kodak/core/TextValidation.h>

using namespace kodak::core;

TEST_CASE("Luhn accepts valid card numbers and rejects the rest") {
    CHECK(luhnValid(U"4111111111111111"));
    CHECK(luhnValid(U"79927398713"));
    CHECK_FALSE(luhnValid(U"79927398710"));
    CHECK_FALSE(luhnValid(U"4111111111111112"));
    CHECK_FALSE(luhnValid(U"4111a11111111111"));
    CHECK_FALSE(luhnValid(U""));
}

TEST_CASE("IBAN: remainder 1 modulo 97, Spanish ones have 24 characters") {
    CHECK(ibanValid(U"ES9121000418450200051332"));
    CHECK(ibanValid(U"es9121000418450200051332"));
    CHECK(ibanValid(U"DE89370400440532013000"));
    CHECK(ibanValid(U"GB82WEST12345698765432"));
    CHECK_FALSE(ibanValid(U"GB82WEST12345698765433")); // wrong check
    CHECK_FALSE(ibanValid(U"ES9221000418450200051332"));
    CHECK_FALSE(ibanValid(U"ES912100041845020005133")); // 23 characters
    CHECK_FALSE(ibanValid(U"ES91"));
    CHECK_FALSE(ibanValid(U"9921000418450200051332ES"));
}

TEST_CASE("DNI control letter") {
    CHECK(dniLetter(12345678) == U'Z');
    CHECK(dniLetter(0) == U'T');
    CHECK(dniLetter(22) == U'E');
    CHECK(dniValid(U"12345678Z"));
    CHECK(dniValid(U"12345678z"));
    CHECK_FALSE(dniValid(U"12345678A"));
    CHECK_FALSE(dniValid(U"1234567Z"));
}

TEST_CASE("NIE: X, Y and Z stand for 0, 1 and 2") {
    CHECK(nieValid(U"X1234567L"));
    CHECK(nieValid(U"Y1234567X"));
    CHECK(nieValid(U"Z1234567R"));
    CHECK(nieValid(U"x1234567l"));
    CHECK_FALSE(nieValid(U"X1234567A"));
    CHECK_FALSE(nieValid(U"W1234567L"));
}

TEST_CASE("IPv4: four octets from 0 to 255") {
    CHECK(ipv4Valid(U"192.168.1.20"));
    CHECK(ipv4Valid(U"0.0.0.0"));
    CHECK(ipv4Valid(U"255.255.255.255"));
    CHECK_FALSE(ipv4Valid(U"256.0.0.1"));
    CHECK_FALSE(ipv4Valid(U"1.2.3"));
    CHECK_FALSE(ipv4Valid(U"1.2.3.4.5"));
    CHECK_FALSE(ipv4Valid(U"1..2.3"));
    CHECK_FALSE(ipv4Valid(U"1234.1.1.1"));
}

TEST_CASE("Shannon entropy in bits per character") {
    CHECK(shannonEntropy(U"aaaa") == doctest::Approx(0.0));
    CHECK(shannonEntropy(U"aabb") == doctest::Approx(1.0));
    CHECK(shannonEntropy(U"abcd") == doctest::Approx(2.0));
    CHECK(shannonEntropy(U"") == doctest::Approx(0.0));
}
