// parse_result.hpp
// How the parsers report success or failure. Instead of throwing an
// exception or returning a bare bool, every parse function returns a
// ParseResult: the parsed value when the text was valid, or a ParseError
// that says precisely what was wrong, so the user gets a message like
// "every octet must be between 0 and 255" instead of just "invalid".

#pragma once

#include <optional>
#include <string_view>
#include <utility>

namespace ipnum {

/// Reason why a piece of text is not a valid IP address.
enum class ParseError {
    None,                 // no error (parsing succeeded)
    Empty,                // ""
    InvalidCharacter,     // a character that cannot appear here, e.g. "1.2.3.4a"
    // IPv4 problems
    EmptyOctet,           // "1..3.4"
    LeadingZero,          // "01.2.3.4" (rejected: some tools read it as octal)
    OctetOutOfRange,      // "256.1.1.1"
    WrongOctetCount,      // "1.2.3" or "1.2.3.4.5"
    // IPv6 problems
    UnexpectedColon,      // ":1::" or "1:::2"
    MultipleCompression,  // "1::2::3" ("::" may appear only once)
    GroupTooLong,         // "12345::" (a group has at most four hex digits)
    TooManyGroups,        // "1:2:3:4:5:6:7:8:9"
    TooFewGroups,         // "1:2:3" without a "::"
    InvalidEmbeddedIpv4,  // "::ffff:1.2.3" (bad IPv4 tail)
    EmptyZone,            // "fe80::1%" (nothing after the '%')
};

/// Human-readable explanation of a parse error.
// Implemented in parse_result.cpp; used by main() for the error message.
std::string_view describe(ParseError error) noexcept;

/// Either a successfully parsed value or the reason parsing failed.
// A small template so the same wrapper serves Ipv4Address, Ipv6Address
// and the IpAddress variant. It is used like std::optional (if (result),
// *result, result->member) with the extra error() accessor.
template <typename T>
class ParseResult {
public:
    // Both constructors are deliberately not `explicit`: a parser can
    // simply `return Ipv4Address{value};` or `return ParseError::Empty;`
    // and the right ParseResult is built automatically.
    ParseResult(T value) : value_(std::move(value)) {}
    ParseResult(ParseError error) noexcept : error_(error) {}

    bool has_value() const noexcept { return value_.has_value(); }
    // Allows `if (result) { ... }`. `explicit` stops it from silently
    // converting to an int in arithmetic.
    explicit operator bool() const noexcept { return has_value(); }

    /// The parsed value. Only valid when has_value() is true.
    const T& value() const { return value_.value(); }
    const T& operator*() const { return *value_; }
    const T* operator->() const { return &*value_; }

    /// ParseError::None when parsing succeeded.
    ParseError error() const noexcept { return error_; }

private:
    std::optional<T> value_;
    ParseError error_ = ParseError::None;
};

}  // namespace ipnum