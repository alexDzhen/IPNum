// test_system_crosscheck.cpp
// Compares our hand-written parsers against the operating system's own
// parser, inet_pton, on hundreds of thousands of random strings. The idea:
// inet_pton has been used by every network program for decades, so if our
// parser accepts exactly the same strings and produces exactly the same
// bytes, it is almost certainly right. Random input also reaches odd corner
// cases nobody would think to write by hand.
//
// The file compiles to nothing unless the C library is glibc (Linux),
// because inet_pton on other systems is slightly more or less strict
// (e.g. about leading zeros) and the comparison would not be fair.
// REQUIRE instead of CHECK is used inside the loops so that the first
// disagreement stops the test, rather than flooding the output with
// thousands of failures.

#include <doctest.h>

#include <cstdlib>

#if defined(__GLIBC__)

#include <arpa/inet.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <random>
#include <string>

#include "../include/ip_address.hpp"

namespace {

// The system's IPv4 parser. inet_pton fills s_addr in network byte order;
// ntohl turns that into the host-order number our parser produces.
bool system_parse_ipv4(const std::string& text, std::uint32_t& value) {
    in_addr address{};
    if (inet_pton(AF_INET, text.c_str(), &address) != 1) {
        return false;
    }
    value = ntohl(address.s_addr);
    return true;
}

// The system's IPv6 parser; the 16 bytes are already in network order.
bool system_parse_ipv6(const std::string& text, std::array<std::uint8_t, 16>& bytes) {
    in6_addr address{};
    if (inet_pton(AF_INET6, text.c_str(), &address) != 1) {
        return false;
    }
    std::memcpy(bytes.data(), address.s6_addr, bytes.size());
    return true;
}

// A random string of up to max_length characters drawn from `alphabet`.
// Repeating a character in the alphabet (".." or "::::::") makes it more
// likely to appear, which steers the strings towards address-like shapes.
std::string random_text(std::mt19937& rng, const std::string& alphabet, std::size_t max_length) {
    std::uniform_int_distribution<std::size_t> length_dist(0, max_length);
    std::uniform_int_distribution<std::size_t> char_dist(0, alphabet.size() - 1);
    std::string text(length_dist(rng), ' ');
    for (char& c : text) {
        c = alphabet[char_dist(rng)];
    }
    return text;
}

}  // namespace

// 200 000 random strings of digits and dots. For each one, both parsers
// must agree on whether it is valid, and when it is, on the value. The
// seed is fixed so a failure can be reproduced exactly. The final CHECK
// makes sure the random strings were not all invalid, which would make
// the test meaningless.
TEST_CASE("IPv4 parsing agrees with the operating system on random input") {
    std::mt19937 rng(20260101u);
    const std::string alphabet = "0123456789..";

    int accepted = 0;
    for (int i = 0; i < 200000; ++i) {
        const std::string text = random_text(rng, alphabet, 15);
        std::uint32_t expected = 0;
        const bool system_ok = system_parse_ipv4(text, expected);
        const auto ours = ipnum::Ipv4Address::parse(text);

        CAPTURE(text);
        REQUIRE(ours.has_value() == system_ok);
        if (system_ok) {
            REQUIRE(ours->to_uint32() == expected);
            ++accepted;
        }
    }
    CHECK(accepted > 0);
}

// The other direction: 20 000 random 32-bit numbers, printed by our
// to_string(), must be accepted by the system parser and give the same
// number back from both parsers. This checks the formatter as well.
TEST_CASE("IPv4 parsing agrees with the operating system on random valid addresses") {
    std::mt19937 rng(7u);
    std::uniform_int_distribution<std::uint32_t> value_dist;

    for (int i = 0; i < 20000; ++i) {
        const std::uint32_t value = value_dist(rng);
        const std::string text = ipnum::Ipv4Address{value}.to_string();
        std::uint32_t expected = 0;

        CAPTURE(text);
        REQUIRE(system_parse_ipv4(text, expected));
        REQUIRE(expected == value);
        REQUIRE(ipnum::Ipv4Address::parse(text)->to_uint32() == value);
    }
}

// Same as the first test for IPv6: 400 000 random strings of hex digits
// (both cases), colons and dots, so that "::" compression and IPv4 tails
// come up often. Compared byte for byte.
TEST_CASE("IPv6 parsing agrees with the operating system on random input") {
    std::mt19937 rng(42u);
    const std::string alphabet = "0123456789abcdefABCDEF::::::..";

    int accepted = 0;
    for (int i = 0; i < 400000; ++i) {
        const std::string text = random_text(rng, alphabet, 24);
        std::array<std::uint8_t, 16> expected{};
        const bool system_ok = system_parse_ipv6(text, expected);
        const auto ours = ipnum::Ipv6Address::parse(text);

        CAPTURE(text);
        REQUIRE(ours.has_value() == system_ok);
        if (system_ok) {
            REQUIRE(ours->bytes() == expected);
            ++accepted;
        }
    }
    CHECK(accepted > 0);
}

// Random valid IPv6 addresses, printed in both the compressed and the
// expanded form, must be accepted by the system and parse back to the
// same bytes with both parsers. About a third of the groups are forced to
// zero so that "::" compression is exercised heavily, since that is the
// trickiest part of the formatter.
TEST_CASE("IPv6 parsing agrees with the operating system on random valid addresses") {
    std::mt19937 rng(99u);
    std::uniform_int_distribution<int> byte_dist(0, 255);
    std::uniform_int_distribution<int> zero_dist(0, 2);

    for (int i = 0; i < 20000; ++i) {
        ipnum::Ipv6Address::Bytes bytes{};
        for (std::size_t group = 0; group < 8; ++group) {
            if (zero_dist(rng) == 0) {
                continue;
            }
            bytes[2 * group] = static_cast<std::uint8_t>(byte_dist(rng));
            bytes[2 * group + 1] = static_cast<std::uint8_t>(byte_dist(rng));
        }
        const ipnum::Ipv6Address address{bytes};

        for (const std::string& text : {address.to_string(), address.to_expanded_string()}) {
            std::array<std::uint8_t, 16> expected{};
            CAPTURE(text);
            REQUIRE(system_parse_ipv6(text, expected));
            REQUIRE(expected == bytes);

            const auto ours = ipnum::Ipv6Address::parse(text);
            REQUIRE(ours.has_value());
            REQUIRE(ours->bytes() == bytes);
        }
    }
}

#endif