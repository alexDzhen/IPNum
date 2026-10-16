// ipv6_address.cpp
// The IPv6 parser and formatter, written by hand (no inet_pton), plus the
// conversions to 16-bit groups, 32-bit words and one 128-bit number, and
// the parse_ip() entry point that picks IPv4 or IPv6 from the text.
//
// IPv6 text (RFC 4291) is eight groups of up to four hex digits separated
// by colons, with two shortcuts that make parsing interesting:
//   - "::" stands for one or more all-zero groups, allowed once, so
//     2001:db8::1 is 2001:0db8:0000:0000:0000:0000:0000:0001;
//   - the last 32 bits may be written as an IPv4 address, e.g. ::ffff:192.168.1.1.
// A "%zone" suffix (fe80::1%eth0) names a network interface and is accepted
// but not part of the address.

#include "../include/ip_address.hpp"

namespace ipnum {

namespace {

constexpr int kGroupCount = 8;

// Value of a hex digit in either case, or -1 for anything else.
// Hand-written instead of std::isxdigit so it does not depend on the locale.
constexpr int hex_value(char c) noexcept {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

// Appends one 16-bit group as lowercase hex. With pad == false leading
// zeros are dropped (0x0db8 -> "db8", 0 -> "0"), which is the compressed
// form; with pad == true all four digits are written ("0db8", "0000"),
// which is the expanded form. The last digit (shift == 0) is always
// printed so a zero group never becomes an empty string.
void append_hex(std::string& out, std::uint16_t group, bool pad) {
    static const char kDigits[] = "0123456789abcdef";
    bool started = pad;
    for (int shift = 12; shift >= 0; shift -= 4) {
        const int digit = (group >> shift) & 0xF;
        if (digit != 0 || started || shift == 0) {
            out += kDigits[digit];
            started = true;
        }
    }
}

}  // namespace

// Single pass over the text, collecting groups into `groups` in the order
// written and remembering where a "::" was seen. Only after the whole text
// has been read do we know how many zero groups the "::" stands for, so
// the groups after it are moved to the end at that point.
ParseResult<Ipv6Address> Ipv6Address::parse(std::string_view text) noexcept {
    // Strip a "%zone" suffix; it is not part of the 128-bit value.
    const std::size_t zone_pos = text.find('%');
    if (zone_pos != std::string_view::npos) {
        if (zone_pos + 1 == text.size()) {
            return ParseError::EmptyZone;
        }
        text = text.substr(0, zone_pos);
    }
    if (text.empty()) {
        return ParseError::Empty;
    }

    std::array<std::uint16_t, kGroupCount> groups{};
    // How many groups have been written explicitly so far.
    int count = 0;
    // Position (in groups) where "::" appeared, or -1 if it did not.
    int compression_index = -1;
    std::size_t pos = 0;

    while (pos < text.size()) {
        // A colon here means we are at the start of the text or right after
        // a group separator was consumed, so it can only be the start of
        // "::". A lone ':' in that position (":1", "1:::2") is an error.
        if (text[pos] == ':') {
            const bool is_double = pos + 1 < text.size() && text[pos + 1] == ':';
            if (!is_double) {
                return ParseError::UnexpectedColon;
            }
            if (compression_index >= 0) {
                return ParseError::MultipleCompression;
            }
            compression_index = count;
            pos += 2;
            continue;
        }

        // Read a run of hex digits. Only the first four are accumulated
        // into the value; the length check below rejects longer runs, and
        // this keeps the accumulator from overflowing meanwhile.
        const std::size_t start = pos;
        std::uint32_t value = 0;
        while (pos < text.size() && hex_value(text[pos]) >= 0) {
            if (pos - start < 4) {
                value = (value << 4) | static_cast<std::uint32_t>(hex_value(text[pos]));
            }
            ++pos;
        }
        const std::size_t length = pos - start;

        // A dot after the digits means this "group" is really the start of
        // an embedded IPv4 address (::ffff:192.168.1.1). It occupies the
        // last 32 bits, i.e. two groups, so at most six groups may precede
        // it. The IPv4 parser does the rest and the text must end there.
        if (pos < text.size() && text[pos] == '.') {
            if (count > kGroupCount - 2) {
                return ParseError::TooManyGroups;
            }
            const auto ipv4 = Ipv4Address::parse(text.substr(start));
            if (!ipv4) {
                return ParseError::InvalidEmbeddedIpv4;
            }
            groups[static_cast<std::size_t>(count++)] =
                static_cast<std::uint16_t>(ipv4->to_uint32() >> 16);
            groups[static_cast<std::size_t>(count++)] =
                static_cast<std::uint16_t>(ipv4->to_uint32() & 0xFFFFu);
            pos = text.size();
            break;
        }

        // Validate the group just read.
        if (length == 0) {
            return ParseError::InvalidCharacter;
        }
        if (length > 4) {
            return ParseError::GroupTooLong;
        }
        if (count == kGroupCount) {
            return ParseError::TooManyGroups;
        }
        groups[static_cast<std::size_t>(count++)] = static_cast<std::uint16_t>(value);

        // After a group: end of text, or a colon.
        if (pos == text.size()) {
            break;
        }
        if (text[pos] != ':') {
            return ParseError::InvalidCharacter;
        }
        // A trailing single colon ("1:2:3:4:5:6:7:") is an error.
        if (pos + 1 == text.size()) {
            return ParseError::UnexpectedColon;
        }
        // Consume a single separator colon. If the next character is also
        // a colon this is "::", which is left for the top of the loop.
        if (text[pos + 1] != ':') {
            ++pos;
        }
    }

    if (compression_index < 0) {
        // No "::": exactly eight groups must have been written.
        if (count != kGroupCount) {
            return ParseError::TooFewGroups;
        }
    } else {
        // With "::": it must stand for at least one group, so eight
        // explicit groups plus "::" is too many.
        if (count == kGroupCount) {
            return ParseError::TooManyGroups;
        }
        // Expand the "::": move the `tail` groups that came after it to the
        // end of the array (back to front, so nothing is overwritten before
        // it is copied), then zero the gap in between.
        const int tail = count - compression_index;
        for (int i = 0; i < tail; ++i) {
            groups[static_cast<std::size_t>(kGroupCount - 1 - i)] =
                groups[static_cast<std::size_t>(count - 1 - i)];
        }
        for (int i = compression_index; i < kGroupCount - tail; ++i) {
            groups[static_cast<std::size_t>(i)] = 0;
        }
    }

    // Eight 16-bit groups -> 16 bytes, high byte first (network order).
    Bytes bytes{};
    for (std::size_t i = 0; i < groups.size(); ++i) {
        bytes[2 * i] = static_cast<std::uint8_t>(groups[i] >> 8);
        bytes[2 * i + 1] = static_cast<std::uint8_t>(groups[i] & 0xFFu);
    }
    return Ipv6Address{bytes};
}

// The 16 bytes as eight 16-bit groups (two bytes each).
std::array<std::uint16_t, 8> Ipv6Address::groups() const noexcept {
    std::array<std::uint16_t, 8> result{};
    for (std::size_t i = 0; i < result.size(); ++i) {
        result[i] = static_cast<std::uint16_t>((bytes_[2 * i] << 8) | bytes_[2 * i + 1]);
    }
    return result;
}

// The 16 bytes as four 32-bit numbers (four bytes each), the same shift-
// and-OR construction used for an IPv4 address, applied four times.
std::array<std::uint32_t, 4> Ipv6Address::words() const noexcept {
    std::array<std::uint32_t, 4> result{};
    for (std::size_t i = 0; i < result.size(); ++i) {
        result[i] = (std::uint32_t{bytes_[4 * i]} << 24) |
                    (std::uint32_t{bytes_[4 * i + 1]} << 16) |
                    (std::uint32_t{bytes_[4 * i + 2]} << 8) | std::uint32_t{bytes_[4 * i + 3]};
    }
    return result;
}

// The four 32-bit words combined into two 64-bit halves of a 128-bit number.
UInt128 Ipv6Address::to_uint128() const noexcept {
    const auto parts = words();
    UInt128 value;
    value.high = (std::uint64_t{parts[0]} << 32) | parts[1];
    value.low = (std::uint64_t{parts[2]} << 32) | parts[3];
    return value;
}

// ::ffff:a.b.c.d has 80 zero bits, then 16 one bits, then the IPv4 address.
bool Ipv6Address::is_ipv4_mapped() const noexcept {
    for (std::size_t i = 0; i < 10; ++i) {
        if (bytes_[i] != 0) {
            return false;
        }
    }
    return bytes_[10] == 0xFF && bytes_[11] == 0xFF;
}

// The last four bytes as an IPv4 address, for mapped addresses only.
std::optional<Ipv4Address> Ipv6Address::mapped_ipv4() const noexcept {
    if (!is_ipv4_mapped()) {
        return std::nullopt;
    }
    return Ipv4Address{bytes_[12], bytes_[13], bytes_[14], bytes_[15]};
}

// Canonical text as defined by RFC 5952: lowercase hex, no leading zeros,
// the longest run of two or more zero groups replaced by "::" (the first
// one if there is a tie), and mapped addresses written with their IPv4 tail.
std::string Ipv6Address::to_string() const {
    if (const auto ipv4 = mapped_ipv4()) {
        return "::ffff:" + ipv4->to_string();
    }

    const auto parts = groups();

    // Find the longest run of consecutive zero groups.
    int best_start = -1;
    int best_length = 0;
    for (int i = 0; i < kGroupCount;) {
        if (parts[static_cast<std::size_t>(i)] != 0) {
            ++i;
            continue;
        }
        int end = i;
        while (end < kGroupCount && parts[static_cast<std::size_t>(end)] == 0) {
            ++end;
        }
        // Strictly longer wins, so the leftmost run is kept on a tie.
        if (end - i > best_length) {
            best_start = i;
            best_length = end - i;
        }
        i = end;
    }
    // A single zero group is written as "0", never compressed.
    if (best_length < 2) {
        best_start = -1;
    }

    // Print the groups, substituting "::" for the chosen run. A separator
    // colon is added between groups unless the text already ends with one
    // (right after "::").
    std::string text;
    for (int i = 0; i < kGroupCount;) {
        if (i == best_start) {
            text += "::";
            i += best_length;
            continue;
        }
        if (!text.empty() && text.back() != ':') {
            text += ':';
        }
        append_hex(text, parts[static_cast<std::size_t>(i)], false);
        ++i;
    }
    return text;
}

// All eight groups, four digits each, no compression:
// "2001:0db8:0000:0000:0000:0000:0000:0001".
std::string Ipv6Address::to_expanded_string() const {
    const auto parts = groups();
    std::string text;
    for (std::size_t i = 0; i < parts.size(); ++i) {
        if (i != 0) {
            text += ':';
        }
        append_hex(text, parts[i], true);
    }
    return text;
}

// Chooses the parser from the text: every IPv6 address contains a colon
// and no IPv4 address does. The result is wrapped in the IpAddress variant
// so the caller can handle both families through one type; on failure the
// specific parser's error is passed through.
ParseResult<IpAddress> parse_ip(std::string_view text) noexcept {
    if (text.empty()) {
        return ParseError::Empty;
    }
    if (text.find(':') != std::string_view::npos) {
        const auto ipv6 = Ipv6Address::parse(text);
        if (!ipv6) {
            return ipv6.error();
        }
        return IpAddress{*ipv6};
    }
    const auto ipv4 = Ipv4Address::parse(text);
    if (!ipv4) {
        return ipv4.error();
    }
    return IpAddress{*ipv4};
}

}  // namespace ipnum