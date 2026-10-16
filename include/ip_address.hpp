#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

#include "uint128.hpp"
#include "parse_result.hpp"

namespace ipnum {

/// An IPv4 address, stored as its 32-bit numeric value.
/// The first octet is the most significant byte: 192.168.1.1 == 0xC0A80101.
class Ipv4Address {
public:
    constexpr Ipv4Address() noexcept = default;
    explicit constexpr Ipv4Address(std::uint32_t value) noexcept : value_(value) {}
    constexpr Ipv4Address(std::uint8_t a, std::uint8_t b, std::uint8_t c, std::uint8_t d) noexcept
        : value_((std::uint32_t{a} << 24) | (std::uint32_t{b} << 16) | (std::uint32_t{c} << 8) |
                 std::uint32_t{d}) {}

    /// Parses strict dotted-decimal notation: four octets, 0-255, no leading zeros.
    static ParseResult<Ipv4Address> parse(std::string_view text) noexcept;

    /// The address as a 32-bit number.
    constexpr std::uint32_t to_uint32() const noexcept { return value_; }

    /// The four octets, in the order they are written.
    std::array<std::uint8_t, 4> octets() const noexcept;

    /// Dotted-decimal notation.
    std::string to_string() const;

private:
    std::uint32_t value_ = 0;
};

constexpr bool operator==(const Ipv4Address& lhs, const Ipv4Address& rhs) noexcept {
    return lhs.to_uint32() == rhs.to_uint32();
}

constexpr bool operator!=(const Ipv4Address& lhs, const Ipv4Address& rhs) noexcept {
    return !(lhs == rhs);
}

/// An IPv6 address, stored as its 16 bytes in network order.
class Ipv6Address {
public:
    using Bytes = std::array<std::uint8_t, 16>;

    constexpr Ipv6Address() noexcept = default;
    explicit constexpr Ipv6Address(const Bytes& bytes) noexcept : bytes_(bytes) {}

    /// Parses RFC 4291 text: full form, "::" compression and an IPv4 tail
    /// such as "::ffff:192.0.2.1". A "%zone" suffix is accepted and ignored.
    static ParseResult<Ipv6Address> parse(std::string_view text) noexcept;

    constexpr const Bytes& bytes() const noexcept { return bytes_; }

    /// The eight 16-bit groups, in the order they are written.
    std::array<std::uint16_t, 8> groups() const noexcept;

    /// The address split into four 32-bit numbers, most significant first.
    std::array<std::uint32_t, 4> words() const noexcept;

    /// The address as one 128-bit number.
    UInt128 to_uint128() const noexcept;

    /// True for addresses in ::ffff:0:0/96, which carry an IPv4 address.
    bool is_ipv4_mapped() const noexcept;

    /// The IPv4 address inside an IPv4-mapped address, if this is one.
    std::optional<Ipv4Address> mapped_ipv4() const noexcept;

    /// Canonical text form as defined by RFC 5952.
    std::string to_string() const;

    /// All eight groups with four digits each, no compression.
    std::string to_expanded_string() const;

private:
    Bytes bytes_{};
};

constexpr bool operator==(const Ipv6Address& lhs, const Ipv6Address& rhs) noexcept {
    for (std::size_t i = 0; i < 16; ++i) {
        if (lhs.bytes()[i] != rhs.bytes()[i]) {
            return false;
        }
    }
    return true;
}

constexpr bool operator!=(const Ipv6Address& lhs, const Ipv6Address& rhs) noexcept {
    return !(lhs == rhs);
}

using IpAddress = std::variant<Ipv4Address, Ipv6Address>;

/// Parses either address family. Text containing ':' is treated as IPv6.
ParseResult<IpAddress> parse_ip(std::string_view text) noexcept;

}  // namespace ipnum
