// test_uint128.cpp
// Unit tests for the 128-bit number (uint128.cpp). UInt128{high, low}
// means high · 2^64 + low. The expected decimal strings were computed
// independently (they are the well-known powers-of-two limits and the
// value of the RFC documentation address), so they check the long
// division rather than just restating what the code does.

#include <doctest.h>

#include <cstdint>
#include <string>

#include "../include/uint128.hpp"

using ipnum::UInt128;

// Decimal output across the interesting boundaries: zero (must be "0", not
// ""), one digit, two digits, the 32-bit limit and one past it, the 64-bit
// limit (all of `low`) and one past it (`high` = 1, which is 2^64), a real
// IPv6 address, and the largest 128-bit value (39 digits).
TEST_CASE("128-bit numbers print in decimal") {
    CHECK(ipnum::to_decimal(UInt128{0, 0}) == "0");
    CHECK(ipnum::to_decimal(UInt128{0, 1}) == "1");
    CHECK(ipnum::to_decimal(UInt128{0, 9}) == "9");
    CHECK(ipnum::to_decimal(UInt128{0, 10}) == "10");
    CHECK(ipnum::to_decimal(UInt128{0, 4294967295u}) == "4294967295");
    CHECK(ipnum::to_decimal(UInt128{0, 4294967296u}) == "4294967296");
    CHECK(ipnum::to_decimal(UInt128{0, 0xFFFFFFFFFFFFFFFFu}) == "18446744073709551615");
    CHECK(ipnum::to_decimal(UInt128{1, 0}) == "18446744073709551616");
    CHECK(ipnum::to_decimal(UInt128{0x20010DB800000000u, 0x0000FF0000428329u}) ==
          "42540766411282592856904265327123268393");
    CHECK(ipnum::to_decimal(UInt128{0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu}) ==
          "340282366920938463463374607431768211455");
}

// Always exactly 32 hex digits: zero-padded for small values, and the two
// halves placed in the right order (high first).
TEST_CASE("128-bit numbers print in hexadecimal with a fixed width") {
    CHECK(ipnum::to_hex(UInt128{0, 0}) == "00000000000000000000000000000000");
    CHECK(ipnum::to_hex(UInt128{0, 1}) == "00000000000000000000000000000001");
    CHECK(ipnum::to_hex(UInt128{0x20010DB800000000u, 0x0000FF0000428329u}) ==
          "20010DB8000000000000FF0000428329");
    CHECK(ipnum::to_hex(UInt128{0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu}) ==
          "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");
}

// Always exactly 128 characters. "5" is 101 at the very end; the top bit
// of `high` (2^127) is a single '1' at the very start.
TEST_CASE("128-bit numbers print in binary with a fixed width") {
    CHECK(ipnum::to_binary(UInt128{0, 0}) == std::string(128, '0'));
    CHECK(ipnum::to_binary(UInt128{0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu}) ==
          std::string(128, '1'));
    CHECK(ipnum::to_binary(UInt128{0, 5}) == std::string(125, '0') + "101");
    CHECK(ipnum::to_binary(UInt128{0x8000000000000000u, 0}) == "1" + std::string(127, '0'));
}

// Equality must look at both halves, not just one of them.
TEST_CASE("128-bit numbers compare by both halves") {
    CHECK(UInt128{1, 2} == UInt128{1, 2});
    CHECK(UInt128{1, 2} != UInt128{2, 1});
    CHECK(UInt128{0, 2} != UInt128{1, 2});
}