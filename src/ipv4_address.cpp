// ipv4_address.cpp
// The IPv4 parser and formatter. The parser is written by hand (no
// inet_pton / inet_addr) so that every kind of mistake gets its own error
// and so that the conversion to a 32-bit number is visible in the code:
// each octet is shifted in with  value = (value << 8) | octet.

#include "../include/ip_address.hpp"

namespace ipnum {

namespace {

// Only ASCII '0'..'9'. Deliberately not std::isdigit, which depends on the
// locale and accepts other characters on some systems.
constexpr bool is_digit(char c) noexcept {
    return c >= '0' && c <= '9';
}

}  // namespace

// Parses "a.b.c.d" strictly: exactly four octets, each 0-255, written in
// plain decimal with no leading zeros, no spaces and nothing else around.
// Walks the text once, one octet per loop iteration, building the 32-bit
// value as it goes. Returns a ParseError the moment something is wrong.
ParseResult<Ipv4Address> Ipv4Address::parse(std::string_view text) noexcept {
    if (text.empty()) {
        return ParseError::Empty;
    }

    std::uint32_t value = 0;
    int octet_count = 0;
    std::size_t pos = 0;

    while (true) {
        // Collect the run of digits that forms the next octet.
        const std::size_t start = pos;
        while (pos < text.size() && is_digit(text[pos])) {
            ++pos;
        }
        const std::size_t length = pos - start;

        // No digits where an octet should be. If we stopped at a dot or
        // at the end ("1..3.4", "1.2.3.") the octet is missing; otherwise
        // we hit a character that does not belong ("a.b.c.d", " 1.2.3.4").
        if (length == 0) {
            const bool at_separator = pos == text.size() || text[pos] == '.';
            return at_separator ? ParseError::EmptyOctet : ParseError::InvalidCharacter;
        }
        // "01" is rejected: some tools read a leading zero as octal, so the
        // same text would mean different numbers in different programs.
        if (length > 1 && text[start] == '0') {
            return ParseError::LeadingZero;
        }
        // More than three digits cannot be 0-255. Checked before converting
        // so a very long digit string cannot overflow the 32-bit octet.
        if (length > 3) {
            return ParseError::OctetOutOfRange;
        }

        // Decimal text to number, digit by digit.
        std::uint32_t octet = 0;
        for (std::size_t i = start; i < pos; ++i) {
            octet = octet * 10 + static_cast<std::uint32_t>(text[i] - '0');
        }
        if (octet > 255) {
            return ParseError::OctetOutOfRange;
        }

        // The conversion itself: make room for 8 new bits and put the
        // octet in them. After four octets the first one has been shifted
        // to the top, which is why 192.168.1.1 becomes 0xC0A80101.
        value = (value << 8) | octet;
        ++octet_count;

        // After the octet: either the end of the text, or a dot leading to
        // the next octet.
        if (pos == text.size()) {
            break;
        }
        if (text[pos] != '.') {
            return ParseError::InvalidCharacter;
        }
        // A dot after the fourth octet means a fifth is coming: too many.
        if (octet_count == 4) {
            return ParseError::WrongOctetCount;
        }
        ++pos;
    }

    // Fewer than four octets ("1.2.3").
    if (octet_count != 4) {
        return ParseError::WrongOctetCount;
    }
    return Ipv4Address{value};
}

// The reverse of the shifts in parse(): pull each byte back out of the
// 32-bit number, most significant first.
std::array<std::uint8_t, 4> Ipv4Address::octets() const noexcept {
    return {
        static_cast<std::uint8_t>(value_ >> 24),
        static_cast<std::uint8_t>(value_ >> 16),
        static_cast<std::uint8_t>(value_ >> 8),
        static_cast<std::uint8_t>(value_),
    };
}

// Dotted-decimal text, e.g. "192.168.1.1".
std::string Ipv4Address::to_string() const {
    const auto parts = octets();
    std::string text;
    for (std::size_t i = 0; i < parts.size(); ++i) {
        if (i != 0) {
            text += '.';
        }
        text += std::to_string(parts[i]);
    }
    return text;
}

}  // namespace ipnum