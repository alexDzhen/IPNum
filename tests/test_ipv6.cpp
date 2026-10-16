// test_ipv6.cpp
// Unit tests for the IPv6 parser and formatter (ipv6_address.cpp), for
// parse_ip() which chooses between the two families, and for the error
// descriptions. Table-driven like test_ipv4.cpp; CAPTURE prints the entry
// being checked when a row fails.

#include <doctest.h>

#include <array>
#include <cstdint>
#include <string>

#include "../include/ip_address.hpp"

using ipnum::Ipv4Address;
using ipnum::Ipv6Address;
using ipnum::ParseError;
using ipnum::UInt128;

// text -> 128-bit number, given as the two 64-bit halves. The rows cover
// every place "::" can appear (start, middle, end, alone), the full form,
// upper and lower case, the all-zero and all-ones extremes.
TEST_CASE("IPv6 addresses convert to their 128-bit value") {
    struct Case {
        const char* text;
        std::uint64_t high;
        std::uint64_t low;
    };
    const Case cases[] = {
        {"::", 0x0u, 0x0u},
        {"::1", 0x0u, 0x1u},
        {"1::", 0x0001000000000000u, 0x0u},
        {"2001:db8::1", 0x20010DB800000000u, 0x1u},
        {"2001:0db8:0000:0000:0000:ff00:0042:8329", 0x20010DB800000000u, 0x0000FF0000428329u},
        {"2001:db8::ff00:42:8329", 0x20010DB800000000u, 0x0000FF0000428329u},
        {"2001:DB8::FF00:42:8329", 0x20010DB800000000u, 0x0000FF0000428329u},
        {"fe80::1", 0xFE80000000000000u, 0x1u},
        {"1:2:3:4:5:6:7:8", 0x0001000200030004u, 0x0005000600070008u},
        {"1:2:3:4:5:6:7::", 0x0001000200030004u, 0x0005000600070000u},
        {"::2:3:4:5:6:7:8", 0x0000000200030004u, 0x0005000600070008u},
        {"1:2:3::6:7:8", 0x0001000200030000u, 0x0000000600070008u},
        {"ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", 0xFFFFFFFFFFFFFFFFu, 0xFFFFFFFFFFFFFFFFu},
    };

    for (const Case& c : cases) {
        CAPTURE(c.text);
        const auto address = Ipv6Address::parse(c.text);
        REQUIRE(address.has_value());
        CHECK(address->to_uint128() == UInt128{c.high, c.low});
    }
}

// The two other views of the same address: four 32-bit words (what the
// program prints as the "32-bit" form) and eight 16-bit groups (what the
// text is made of).
TEST_CASE("an IPv6 address splits into four 32-bit words") {
    const auto address = Ipv6Address::parse("2001:db8::ff00:42:8329");
    REQUIRE(address.has_value());

    const auto words = address->words();
    CHECK(words[0] == 0x20010DB8u);
    CHECK(words[1] == 0x00000000u);
    CHECK(words[2] == 0x0000FF00u);
    CHECK(words[3] == 0x00428329u);

    const auto groups = address->groups();
    CHECK(groups[0] == 0x2001);
    CHECK(groups[1] == 0x0db8);
    CHECK(groups[5] == 0xff00);
    CHECK(groups[7] == 0x8329);
}

// The dotted-decimal tail. Three shapes:
//  - ::ffff:192.168.1.1, an IPv4-mapped address: is_ipv4_mapped() is true,
//    mapped_ipv4() gives the 32-bit value, and the hex spelling of the same
//    address (::ffff:c0a8:101) parses to the identical value;
//  - 64:ff9b::8.8.8.8 (the NAT64 prefix): a valid tail, but not "mapped";
//  - 1:2:3:4:5:6:1.2.3.4: six groups plus the tail, filling all 8 groups.
TEST_CASE("an IPv4 tail is accepted in IPv6 text") {
    const auto mapped = Ipv6Address::parse("::ffff:192.168.1.1");
    REQUIRE(mapped.has_value());
    CHECK(mapped->is_ipv4_mapped());
    REQUIRE(mapped->mapped_ipv4().has_value());
    CHECK(mapped->mapped_ipv4()->to_uint32() == 3232235777u);
    CHECK(mapped->words()[3] == 3232235777u);
    CHECK(*mapped == *Ipv6Address::parse("::ffff:c0a8:101"));

    const auto nat64 = Ipv6Address::parse("64:ff9b::8.8.8.8");
    REQUIRE(nat64.has_value());
    CHECK_FALSE(nat64->is_ipv4_mapped());
    CHECK_FALSE(nat64->mapped_ipv4().has_value());
    CHECK(nat64->words()[3] == 134744072u);

    const auto full = Ipv6Address::parse("1:2:3:4:5:6:1.2.3.4");
    REQUIRE(full.has_value());
    CHECK(full->groups()[6] == 0x0102);
    CHECK(full->groups()[7] == 0x0304);
}

// "%eth0" names an interface, not part of the 128 bits: the address with
// and without it must be equal.
TEST_CASE("a zone suffix is accepted and ignored") {
    const auto with_zone = Ipv6Address::parse("fe80::1%eth0");
    const auto without_zone = Ipv6Address::parse("fe80::1");
    REQUIRE(with_zone.has_value());
    REQUIRE(without_zone.has_value());
    CHECK(*with_zone == *without_zone);
}

// The canonical text rules, one row each: leading zeros dropped, upper
// case lowered, "::" at the start/end/alone, a single zero group left as
// "0", the first of two equal zero runs compressed, the longer of two
// unequal runs compressed, no zeros -> no "::", and the mapped form with
// an IPv4 tail. Every canonical string must also parse back to the same
// address (round trip).
TEST_CASE("IPv6 text output follows RFC 5952") {
    struct Case {
        const char* input;
        const char* canonical;
    };
    const Case cases[] = {
        {"2001:0db8:0000:0000:0000:ff00:0042:8329", "2001:db8::ff00:42:8329"},
        {"2001:DB8:0:0:0:0:0:1", "2001:db8::1"},
        {"0:0:0:0:0:0:0:0", "::"},
        {"0:0:0:0:0:0:0:1", "::1"},
        {"1:0:0:0:0:0:0:0", "1::"},
        {"2001:db8:0:1:1:1:1:1", "2001:db8:0:1:1:1:1:1"},
        {"2001:0:0:1:0:0:0:1", "2001:0:0:1::1"},
        {"2001:db8:0:0:1:0:0:1", "2001:db8::1:0:0:1"},
        {"1:2:3:4:5:6:7:8", "1:2:3:4:5:6:7:8"},
        {"0:0:1:0:0:0:0:0", "0:0:1::"},
        {"::ffff:c0a8:101", "::ffff:192.168.1.1"},
    };

    for (const Case& c : cases) {
        CAPTURE(c.input);
        const auto address = Ipv6Address::parse(c.input);
        REQUIRE(address.has_value());
        CHECK(address->to_string() == c.canonical);

        const auto reparsed = Ipv6Address::parse(address->to_string());
        REQUIRE(reparsed.has_value());
        CHECK(*reparsed == *address);
    }
}

// The long form: no compression, every group padded to four digits.
TEST_CASE("the expanded form always has eight four-digit groups") {
    CHECK(Ipv6Address::parse("::1")->to_expanded_string() ==
          "0000:0000:0000:0000:0000:0000:0000:0001");
    CHECK(Ipv6Address::parse("2001:db8::ff00:42:8329")->to_expanded_string() ==
          "2001:0db8:0000:0000:0000:ff00:0042:8329");
}

// Every way the input can be wrong, with the exact error expected: a bad
// or empty zone, stray single colons, more than one "::", groups of five
// digits, too many groups (also when "::" would have to stand for zero
// groups, and when an IPv4 tail pushes the count over), too few groups
// without "::", a broken IPv4 tail, and characters that do not belong
// (a non-hex letter, a /64 suffix, a trailing space, URL-style brackets).
TEST_CASE("malformed IPv6 addresses are rejected with a specific reason") {
    struct Case {
        const char* text;
        ParseError error;
    };
    const Case cases[] = {
        {"", ParseError::Empty},
        {"%eth0", ParseError::Empty},
        {"fe80::1%", ParseError::EmptyZone},
        {":", ParseError::UnexpectedColon},
        {":1:2:3:4:5:6:7", ParseError::UnexpectedColon},
        {"1:2:3:4:5:6:7:", ParseError::UnexpectedColon},
        {":::", ParseError::UnexpectedColon},
        {"1:::2", ParseError::UnexpectedColon},
        {"1::2::3", ParseError::MultipleCompression},
        {"::1::", ParseError::MultipleCompression},
        {"12345::", ParseError::GroupTooLong},
        {"::00001", ParseError::GroupTooLong},
        {"1:2:3:4:5:6:7:8:9", ParseError::TooManyGroups},
        {"1:2:3:4:5:6:7:8::", ParseError::TooManyGroups},
        {"::1:2:3:4:5:6:7:8", ParseError::TooManyGroups},
        {"1:2:3:4:5:6:7:1.2.3.4", ParseError::TooManyGroups},
        {"1:2:3:4:5:6:7", ParseError::TooFewGroups},
        {"1:2", ParseError::TooFewGroups},
        {"::ffff:1.2.3", ParseError::InvalidEmbeddedIpv4},
        {"::ffff:1.2.3.256", ParseError::InvalidEmbeddedIpv4},
        {"::1.2.3.4:5", ParseError::InvalidEmbeddedIpv4},
        {"::g", ParseError::InvalidCharacter},
        {"2001:db8::1/64", ParseError::InvalidCharacter},
        {"2001:db8::1 ", ParseError::InvalidCharacter},
        {"[::1]", ParseError::InvalidCharacter},
    };

    for (const Case& c : cases) {
        CAPTURE(c.text);
        const auto address = Ipv6Address::parse(c.text);
        CHECK_FALSE(address.has_value());
        CHECK(address.error() == c.error);
    }
}

// parse_ip() must put an IPv4 text into the Ipv4Address alternative of the
// variant and an IPv6 text into the Ipv6Address one (std::holds_alternative
// checks which, std::get extracts it), and must pass the specific parser's
// error through unchanged on failure.
TEST_CASE("parse_ip picks the address family from the text") {
    const auto ipv4 = ipnum::parse_ip("192.168.1.1");
    REQUIRE(ipv4.has_value());
    REQUIRE(std::holds_alternative<Ipv4Address>(*ipv4));
    CHECK(std::get<Ipv4Address>(*ipv4).to_uint32() == 3232235777u);

    const auto ipv6 = ipnum::parse_ip("2001:db8::1");
    REQUIRE(ipv6.has_value());
    REQUIRE(std::holds_alternative<Ipv6Address>(*ipv6));
    CHECK(std::get<Ipv6Address>(*ipv6).to_string() == "2001:db8::1");

    CHECK(ipnum::parse_ip("").error() == ParseError::Empty);
    CHECK(ipnum::parse_ip("example.com").error() == ParseError::InvalidCharacter);
    CHECK(ipnum::parse_ip("1.2.3").error() == ParseError::WrongOctetCount);
    CHECK(ipnum::parse_ip("1::2::3").error() == ParseError::MultipleCompression);
}

// Guards against adding a ParseError value and forgetting its text in
// describe(): every enumerator must have a real description, not the
// "unknown error" fallback.
TEST_CASE("every parse error has a description") {
    const ParseError errors[] = {
        ParseError::None,
        ParseError::Empty,
        ParseError::InvalidCharacter,
        ParseError::EmptyOctet,
        ParseError::LeadingZero,
        ParseError::OctetOutOfRange,
        ParseError::WrongOctetCount,
        ParseError::UnexpectedColon,
        ParseError::MultipleCompression,
        ParseError::GroupTooLong,
        ParseError::TooManyGroups,
        ParseError::TooFewGroups,
        ParseError::InvalidEmbeddedIpv4,
        ParseError::EmptyZone,
    };
    for (const ParseError error : errors) {
        const std::string description{ipnum::describe(error)};
        CHECK_FALSE(description.empty());
        CHECK(description != "unknown error");
    }
}