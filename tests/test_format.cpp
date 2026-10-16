// test_format.cpp
// Unit tests for the output formatting (format.cpp): the 32-bit hex and
// binary helpers, the --format name lookup, and the exact text produced
// for an IPv4, an IPv6 and an IPv4-mapped address in every format.
// Comparing whole strings character for character pins down the output
// format so it cannot change by accident.

#include <doctest.h>

#include <string>

#include "../include/format.hpp"
#include "../include/ip_address.hpp"

using ipnum::IpAddress;
using ipnum::OutputFormat;

namespace {

// Parses an address that is known to be valid, failing the test loudly if
// it is not, so the render tests can be written with plain text literals.
IpAddress address_of(const char* text) {
    const auto address = ipnum::parse_ip(text);
    REQUIRE(address.has_value());
    return *address;
}

}  // namespace

// Zero, a typical value and all ones must all come out at exactly 8 hex
// or 32 binary digits; the optional separator splits the binary into octets.
TEST_CASE("32-bit helpers use a fixed width") {
    CHECK(ipnum::to_hex(0u) == "00000000");
    CHECK(ipnum::to_hex(0xC0A80101u) == "C0A80101");
    CHECK(ipnum::to_hex(0xFFFFFFFFu) == "FFFFFFFF");
    CHECK(ipnum::to_binary(0xC0A80101u) == "11000000101010000000000100000001");
    CHECK(ipnum::to_binary(0xC0A80101u, '.') == "11000000.10101000.00000001.00000001");
    CHECK(ipnum::to_binary(0u) == "00000000000000000000000000000000");
}

// Every valid name, plus an unknown name and the empty string.
TEST_CASE("format names map to formats") {
    CHECK(ipnum::parse_output_format("text") == OutputFormat::Text);
    CHECK(ipnum::parse_output_format("dec") == OutputFormat::Decimal);
    CHECK(ipnum::parse_output_format("hex") == OutputFormat::Hex);
    CHECK(ipnum::parse_output_format("bin") == OutputFormat::Binary);
    CHECK(ipnum::parse_output_format("words") == OutputFormat::Words);
    CHECK(ipnum::parse_output_format("json") == OutputFormat::Json);
    CHECK_FALSE(ipnum::parse_output_format("xml").has_value());
    CHECK_FALSE(ipnum::parse_output_format("").has_value());
}

// 192.168.1.1 = 192·2^24 + 168·2^16 + 1·2^8 + 1 = 3232235777 = 0xC0A80101.
// "words" equals "dec" for IPv4 because the address is a single 32-bit word.
TEST_CASE("an IPv4 address renders in every format") {
    const IpAddress address = address_of("192.168.1.1");

    CHECK(ipnum::render(address, OutputFormat::Decimal) == "3232235777");
    CHECK(ipnum::render(address, OutputFormat::Words) == "3232235777");
    CHECK(ipnum::render(address, OutputFormat::Hex) == "0xC0A80101");
    CHECK(ipnum::render(address, OutputFormat::Binary) == "11000000101010000000000100000001");
    CHECK(ipnum::render(address, OutputFormat::Json) ==
          "{\"address\":\"192.168.1.1\",\"version\":4,\"bits\":32,\"decimal\":\"3232235777\","
          "\"hex\":\"0xC0A80101\",\"words\":[3232235777]}");
    CHECK(ipnum::render(address, OutputFormat::Text) ==
          "Address  : 192.168.1.1\n"
          "Version  : IPv4 (32-bit)\n"
          "Decimal  : 3232235777\n"
          "Hex      : 0xC0A80101\n"
          "Binary   : 11000000.10101000.00000001.00000001");
}

// The RFC documentation address 2001:db8::ff00:42:8329. Its four 32-bit
// words are 0x20010DB8, 0, 0x0000FF00 and 0x00428329; the 128-bit decimal
// value is the 38-digit number below. The binary form is only checked for
// length (128 characters), since the full string would be unreadable here.
TEST_CASE("an IPv6 address renders in every format") {
    const IpAddress address = address_of("2001:db8::ff00:42:8329");

    CHECK(ipnum::render(address, OutputFormat::Decimal) ==
          "42540766411282592856904265327123268393");
    CHECK(ipnum::render(address, OutputFormat::Words) == "536939960 0 65280 4358953");
    CHECK(ipnum::render(address, OutputFormat::Hex) == "0x20010DB8000000000000FF0000428329");
    CHECK(ipnum::render(address, OutputFormat::Binary).size() == 128);
    CHECK(ipnum::render(address, OutputFormat::Json) ==
          "{\"address\":\"2001:db8::ff00:42:8329\",\"version\":6,\"bits\":128,"
          "\"decimal\":\"42540766411282592856904265327123268393\","
          "\"hex\":\"0x20010DB8000000000000FF0000428329\",\"words\":[536939960,0,65280,4358953]}");
    CHECK(ipnum::render(address, OutputFormat::Text) ==
          "Address  : 2001:db8::ff00:42:8329\n"
          "Version  : IPv6 (128-bit)\n"
          "Expanded : 2001:0db8:0000:0000:0000:ff00:0042:8329\n"
          "Decimal  : 42540766411282592856904265327123268393\n"
          "Hex      : 0x20010DB8000000000000FF0000428329\n"
          "32-bit words, most significant first:\n"
          "  [0] 0x20010DB8 = 536939960\n"
          "  [1] 0x00000000 = 0\n"
          "  [2] 0x0000FF00 = 65280\n"
          "  [3] 0x00428329 = 4358953");
}

// For ::ffff:a.b.c.d the report and the JSON get an extra part with the
// embedded IPv4 address and its own 32-bit value.
TEST_CASE("an IPv4-mapped IPv6 address also reports its 32-bit value") {
    const IpAddress address = address_of("::ffff:192.168.1.1");

    const std::string text = ipnum::render(address, OutputFormat::Text);
    CHECK(text.find("Mapped IPv4 : 192.168.1.1 = 3232235777") != std::string::npos);

    const std::string json = ipnum::render(address, OutputFormat::Json);
    CHECK(json.find("\"mapped_ipv4\":{\"address\":\"192.168.1.1\",\"decimal\":\"3232235777\"}") !=
          std::string::npos);
}