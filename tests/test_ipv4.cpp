// test_ipv4.cpp
// Unit tests for the IPv4 parser and formatter (ipv4_address.cpp).
// Table-driven: each test holds an array of (input, expected) pairs and
// loops over it. CAPTURE(c.text) makes doctest print which entry was being
// checked when something fails, so a failure in a 20-row table is easy to
// locate.

#include <doctest.h>

#include <cstdint>
#include <string>

#include "../include/ip_address.hpp"

using ipnum::Ipv4Address;
using ipnum::ParseError;

// The core requirement of the task: text -> 32-bit number. The expected
// values are the well-known ones (a·2^24 + b·2^16 + c·2^8 + d), including
// the extremes 0.0.0.0 and 255.255.255.255. Each address must also print
// back exactly as written.
TEST_CASE("IPv4 addresses convert to their 32-bit value") {
    struct Case {
        const char* text;
        std::uint32_t value;
    };
    const Case cases[] = {
        {"0.0.0.0", 0u},
        {"0.0.0.1", 1u},
        {"0.0.1.0", 256u},
        {"1.0.0.0", 16777216u},
        {"10.0.0.1", 167772161u},
        {"127.0.0.1", 2130706433u},
        {"172.16.254.1", 2886794753u},
        {"192.168.1.1", 3232235777u},
        {"8.8.8.8", 134744072u},
        {"255.255.255.255", 4294967295u},
    };

    for (const Case& c : cases) {
        CAPTURE(c.text);
        const auto address = Ipv4Address::parse(c.text);
        REQUIRE(address.has_value());
        CHECK(address.error() == ParseError::None);
        CHECK(address->to_uint32() == c.value);
        CHECK(address->to_string() == c.text);
    }
}

// Byte order: 192 must end up in the top 8 bits (0xC0 of 0xC0A80101), and
// octets() must give the bytes back in the written order.
TEST_CASE("the first octet is the most significant byte") {
    const auto address = Ipv4Address::parse("192.168.1.1");
    REQUIRE(address.has_value());
    CHECK(address->to_uint32() == 0xC0A80101u);

    const auto octets = address->octets();
    CHECK(octets[0] == 192);
    CHECK(octets[1] == 168);
    CHECK(octets[2] == 1);
    CHECK(octets[3] == 1);
}

// The three constructors agree with each other: from a number, from four
// octets, and the default (0.0.0.0).
TEST_CASE("IPv4 addresses can be built from a number or from octets") {
    CHECK(Ipv4Address{3232235777u}.to_string() == "192.168.1.1");
    CHECK(Ipv4Address{192, 168, 1, 1}.to_uint32() == 3232235777u);
    CHECK(Ipv4Address{192, 168, 1, 1} == Ipv4Address{0xC0A80101u});
    CHECK(Ipv4Address{10, 0, 0, 1} != Ipv4Address{10, 0, 0, 2});
    CHECK(Ipv4Address{}.to_uint32() == 0u);
}

// number -> text -> number must give the original number back. The samples
// sit on byte boundaries (255/256, 65535/65536, ...) and at the signed and
// unsigned 32-bit limits, where off-by-one and sign errors would show.
TEST_CASE("every 32-bit value survives a round trip through text") {
    const std::uint32_t samples[] = {0u,          1u,          255u,        256u,
                                     65535u,      65536u,      16777215u,   16777216u,
                                     2147483647u, 2147483648u, 3735928559u, 4294967295u};
    for (const std::uint32_t value : samples) {
        const std::string text = Ipv4Address{value}.to_string();
        CAPTURE(text);
        const auto parsed = Ipv4Address::parse(text);
        REQUIRE(parsed.has_value());
        CHECK(parsed->to_uint32() == value);
    }
}

// Every way the input can be wrong, and the exact error expected for each:
// wrong number of octets, values above 255 (including ones long enough to
// overflow a naive parser), leading zeros, missing octets, and characters
// that do not belong (letters, spaces, commas, signs, a /24 suffix, a hex
// prefix). The parser must not only reject these but name the right reason.
TEST_CASE("malformed IPv4 addresses are rejected with a specific reason") {
    struct Case {
        const char* text;
        ParseError error;
    };
    const Case cases[] = {
        {"", ParseError::Empty},
        {"1.2.3", ParseError::WrongOctetCount},
        {"1", ParseError::WrongOctetCount},
        {"1.2.3.4.5", ParseError::WrongOctetCount},
        {"1.2.3.4.", ParseError::WrongOctetCount},
        {"256.1.1.1", ParseError::OctetOutOfRange},
        {"1.1.1.999", ParseError::OctetOutOfRange},
        {"1.1.1.1000", ParseError::OctetOutOfRange},
        {"1.1.1.4294967297", ParseError::OctetOutOfRange},
        {"01.2.3.4", ParseError::LeadingZero},
        {"1.2.3.00", ParseError::LeadingZero},
        {"1..3.4", ParseError::EmptyOctet},
        {".1.2.3", ParseError::EmptyOctet},
        {"1.2.3.", ParseError::EmptyOctet},
        {"a.b.c.d", ParseError::InvalidCharacter},
        {"1.2.3.4a", ParseError::InvalidCharacter},
        {" 1.2.3.4", ParseError::InvalidCharacter},
        {"1.2.3.4 ", ParseError::InvalidCharacter},
        {"1,2,3,4", ParseError::InvalidCharacter},
        {"-1.2.3.4", ParseError::InvalidCharacter},
        {"+1.2.3.4", ParseError::InvalidCharacter},
        {"1.2.3.4/24", ParseError::InvalidCharacter},
        {"0x7f.0.0.1", ParseError::InvalidCharacter},
    };

    for (const Case& c : cases) {
        CAPTURE(c.text);
        const auto address = Ipv4Address::parse(c.text);
        CHECK_FALSE(address.has_value());
        CHECK(address.error() == c.error);
    }
}