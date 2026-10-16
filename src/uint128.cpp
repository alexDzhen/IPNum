// uint128.cpp
// Printing a 128-bit number. Hex and binary are easy because each digit
// maps to a fixed group of bits. Decimal is the interesting one: base 10
// does not line up with bits, so the digits have to be produced by actual
// division, and no built-in type is wide enough to divide 128 bits at once.

#include "../include/uint128.hpp"

#include <algorithm>
#include <array>

namespace ipnum {

// Long division by 10, the same way it is done by hand on paper.
// The 128-bit number is split into four 32-bit "limbs" (most significant
// first). Each pass of the outer loop divides the whole number by 10:
// the limbs are processed left to right, each one together with the
// remainder carried from the previous limb, exactly like carrying a
// remainder from one digit to the next in schoolbook division. Because a
// limb is 32 bits and the carried remainder is less than 10, the combined
// value always fits in 64 bits, which the CPU can divide directly.
// The remainder of each full pass is the next decimal digit, produced from
// the least significant end; the loop stops when the quotient is zero.
std::string to_decimal(const UInt128& value) {
    std::array<std::uint32_t, 4> limbs{
        static_cast<std::uint32_t>(value.high >> 32),
        static_cast<std::uint32_t>(value.high & 0xFFFFFFFFu),
        static_cast<std::uint32_t>(value.low >> 32),
        static_cast<std::uint32_t>(value.low & 0xFFFFFFFFu),
    };

    std::string digits;
    // Starts true so that the loop runs at least once and the value 0
    // produces "0" rather than an empty string.
    bool quotient_is_nonzero = true;
    while (quotient_is_nonzero) {
        std::uint64_t remainder = 0;
        quotient_is_nonzero = false;
        for (std::uint32_t& limb : limbs) {
            const std::uint64_t current = (remainder << 32) | limb;
            limb = static_cast<std::uint32_t>(current / 10);
            remainder = current % 10;
            quotient_is_nonzero = quotient_is_nonzero || limb != 0;
        }
        digits.push_back(static_cast<char>('0' + remainder));
    }

    // Digits came out least significant first; put them in reading order.
    std::reverse(digits.begin(), digits.end());
    return digits;
}

// 128 bits -> 32 hex digits, 16 from each half. Digit i (from the right of
// each half) is bits 4i..4i+3, so shift by 4*i and mask with 0xF; the string
// is pre-filled with '0' and written from the right so the width stays
// fixed and leading zeros are kept.
std::string to_hex(const UInt128& value) {
    static const char kDigits[] = "0123456789ABCDEF";
    std::string text(32, '0');
    for (int i = 0; i < 16; ++i) {
        text[static_cast<std::size_t>(15 - i)] = kDigits[(value.high >> (4 * i)) & 0xF];
        text[static_cast<std::size_t>(31 - i)] = kDigits[(value.low >> (4 * i)) & 0xF];
    }
    return text;
}

// 128 bits -> 128 characters. The string starts as all '0'; every set bit
// flips its character to '1'. Bit i of `high` is character 63-i, bit i of
// `low` is character 127-i, so the most significant bit comes first.
std::string to_binary(const UInt128& value) {
    std::string text(128, '0');
    for (int i = 0; i < 64; ++i) {
        if ((value.high >> i) & 1u) {
            text[static_cast<std::size_t>(63 - i)] = '1';
        }
        if ((value.low >> i) & 1u) {
            text[static_cast<std::size_t>(127 - i)] = '1';
        }
    }
    return text;
}

}  // namespace ipnum