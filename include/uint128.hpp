// uint128.hpp
// A minimal 128-bit unsigned integer, just enough to hold an IPv6 address
// as one number and print it. Only what ipnum needs is provided: storage,
// equality and conversion to decimal, hex and binary text. There is no
// arithmetic because the program never computes with the value, it only
// displays it. Implemented in uint128.cpp.

#pragma once

#include <cstdint>
#include <string>

namespace ipnum {

/// Unsigned 128-bit integer kept as two 64-bit halves.
/// Standard C++17 has no 128-bit type, and an IPv6 address needs one.
// value = high · 2^64 + low. For 2001:db8::1 the first 8 bytes of the
// address go into `high` and the last 8 into `low`.
// (GCC and Clang offer __int128 as an extension, but MSVC does not, and
// the project must build on Windows too.)
struct UInt128 {
    std::uint64_t high = 0;
    std::uint64_t low = 0;
};

// Equal when both halves are equal.
constexpr bool operator==(const UInt128& lhs, const UInt128& rhs) noexcept {
    return lhs.high == rhs.high && lhs.low == rhs.low;
}

constexpr bool operator!=(const UInt128& lhs, const UInt128& rhs) noexcept {
    return !(lhs == rhs);
}

/// Decimal representation without leading zeros, e.g. "340282366920938463463374607431768211455".
// The one non-trivial conversion: there is no printf format for 128 bits,
// so the digits are produced by repeated division by 10 (see uint128.cpp).
std::string to_decimal(const UInt128& value);

/// Exactly 32 upper-case hexadecimal digits, without a prefix.
// 128 bits / 4 bits per hex digit = 32 digits, zero-padded.
std::string to_hex(const UInt128& value);

/// Exactly 128 binary digits.
std::string to_binary(const UInt128& value);

}  // namespace ipnum