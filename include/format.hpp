// format.hpp
// Declares the output side of ipnum: the available output formats, the
// 32-bit number helpers and render(), which turns a parsed address into
// the text the program prints. Implemented in format.cpp.

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "ip_address.hpp"

namespace ipnum {

/// How a converted address is printed.
// Text is the multi-line human-readable report; the others print one
// value per line for use in scripts.
enum class OutputFormat {
    Text,     // full report: address, version, decimal, hex, binary...
    Decimal,  // the number in base 10 only
    Hex,      // the number in base 16 only, "0x" prefix
    Binary,   // the number in base 2 only
    Words,    // the 32-bit parts in decimal (one for IPv4, four for IPv6)
    Json,     // one JSON object per address
};

/// Maps "text", "dec", "hex", "bin", "words" or "json" to a format.
// Used by the command-line parser for the --format option. Returns nothing
// for an unknown name so the caller can report a usage error.
std::optional<OutputFormat> parse_output_format(std::string_view name) noexcept;

/// Eight upper-case hexadecimal digits, without a prefix.
// e.g. 0xC0A80101 -> "C0A80101". Fixed width, so values line up.
std::string to_hex(std::uint32_t value);

/// 32 binary digits. A non-zero separator is inserted between the four bytes.
// e.g. to_binary(0xC0A80101, '.') -> "11000000.10101000.00000001.00000001",
// which shows how each octet of 192.168.1.1 maps onto 8 bits.
std::string to_binary(std::uint32_t value, char separator = '\0');

/// Renders an address in the requested format. The result has no trailing newline.
// Works for both families: an IPv4 address is one 32-bit number, an IPv6
// address is one 128-bit number and also four 32-bit words.
std::string render(const IpAddress& address, OutputFormat format);

}  // namespace ipnum