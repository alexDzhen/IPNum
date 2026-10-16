// format.cpp
// Turns a parsed address into the text ipnum prints, in whichever of the
// six output formats was requested. There is one renderer per address
// family, plus the small helpers that print a 32-bit number in hex and
// binary. Keeping all formatting here (and out of main) means the exact
// output can be unit-tested.

#include "../include/format.hpp"

namespace ipnum {

namespace {

// IPv4: the whole point of the program, the address as one 32-bit number.
// The one-line formats return early from the switch; the Text format
// falls through to build the multi-line report below.
std::string render_ipv4(const Ipv4Address& address, OutputFormat format) {
    const std::uint32_t value = address.to_uint32();
    switch (format) {
        case OutputFormat::Decimal:
        case OutputFormat::Words:
            // An IPv4 address is exactly one 32-bit word, so "words" and
            // "decimal" print the same thing.
            return std::to_string(value);
        case OutputFormat::Hex:
            return "0x" + to_hex(value);
        case OutputFormat::Binary:
            return to_binary(value);
        case OutputFormat::Json:
            // Numbers are emitted as JSON strings on purpose: JSON parsers
            // (e.g. JavaScript) do not handle 128-bit integers, and the
            // IPv4 output follows the same shape for consistency.
            return "{\"address\":\"" + address.to_string() + "\",\"version\":4,\"bits\":32," +
                   "\"decimal\":\"" + std::to_string(value) + "\",\"hex\":\"0x" + to_hex(value) +
                   "\",\"words\":[" + std::to_string(value) + "]}";
        case OutputFormat::Text:
            break;
    }

    // The human-readable report. The binary line is split into the four
    // octets with dots, which shows how 192.168.1.1 maps onto 32 bits.
    std::string text;
    text += "Address  : " + address.to_string() + "\n";
    text += "Version  : IPv4 (32-bit)\n";
    text += "Decimal  : " + std::to_string(value) + "\n";
    text += "Hex      : 0x" + to_hex(value) + "\n";
    text += "Binary   : " + to_binary(value, '.');
    return text;
}

// IPv6: 128 bits do not fit in a 32-bit number, so the address is shown
// both as one 128-bit number (via UInt128) and as four 32-bit words.
std::string render_ipv6(const Ipv6Address& address, OutputFormat format) {
    const UInt128 value = address.to_uint128();
    const auto words = address.words();

    // The four words joined into one string: comma-separated inside a JSON
    // array, space-separated for the "words" format.
    std::string word_list;
    for (std::size_t i = 0; i < words.size(); ++i) {
        if (i != 0) {
            word_list += format == OutputFormat::Json ? "," : " ";
        }
        word_list += std::to_string(words[i]);
    }

    switch (format) {
        case OutputFormat::Decimal:
            return to_decimal(value);
        case OutputFormat::Words:
            return word_list;
        case OutputFormat::Hex:
            return "0x" + to_hex(value);
        case OutputFormat::Binary:
            return to_binary(value);
        case OutputFormat::Json: {
            std::string json = "{\"address\":\"" + address.to_string() +
                               "\",\"version\":6,\"bits\":128," + "\"decimal\":\"" +
                               to_decimal(value) + "\",\"hex\":\"0x" + to_hex(value) +
                               "\",\"words\":[" + word_list + "]";
            // For ::ffff:a.b.c.d also give the embedded IPv4 address and
            // its plain 32-bit value.
            if (const auto ipv4 = address.mapped_ipv4()) {
                json += ",\"mapped_ipv4\":{\"address\":\"" + ipv4->to_string() +
                        "\",\"decimal\":\"" + std::to_string(ipv4->to_uint32()) + "\"}";
            }
            json += "}";
            return json;
        }
        case OutputFormat::Text:
            break;
    }

    // The human-readable report: compressed and expanded text forms, the
    // 128-bit number, then each 32-bit word in hex and decimal.
    std::string text;
    text += "Address  : " + address.to_string() + "\n";
    text += "Version  : IPv6 (128-bit)\n";
    text += "Expanded : " + address.to_expanded_string() + "\n";
    text += "Decimal  : " + to_decimal(value) + "\n";
    text += "Hex      : 0x" + to_hex(value) + "\n";
    text += "32-bit words, most significant first:";
    for (std::size_t i = 0; i < words.size(); ++i) {
        text += "\n  [" + std::to_string(i) + "] 0x" + to_hex(words[i]) + " = " +
                std::to_string(words[i]);
    }
    if (const auto ipv4 = address.mapped_ipv4()) {
        text += "\nMapped IPv4 : " + ipv4->to_string() + " = " + std::to_string(ipv4->to_uint32());
    }
    return text;
}

}  // namespace

// The names accepted by --format. Anything else gives nullopt and the
// command-line parser reports a usage error.
std::optional<OutputFormat> parse_output_format(std::string_view name) noexcept {
    if (name == "text") {
        return OutputFormat::Text;
    }
    if (name == "dec") {
        return OutputFormat::Decimal;
    }
    if (name == "hex") {
        return OutputFormat::Hex;
    }
    if (name == "bin") {
        return OutputFormat::Binary;
    }
    if (name == "words") {
        return OutputFormat::Words;
    }
    if (name == "json") {
        return OutputFormat::Json;
    }
    return std::nullopt;
}

// 32 bits as exactly 8 hex digits. Digit i (from the right) is bits
// 4i..4i+3 of the value, so the loop shifts by 4*i and masks with 0xF.
// Written into a pre-sized string from the right so the width is fixed
// and leading zeros are kept ("0A000001", not "A000001").
std::string to_hex(std::uint32_t value) {
    static const char kDigits[] = "0123456789ABCDEF";
    std::string text(8, '0');
    for (int i = 0; i < 8; ++i) {
        text[static_cast<std::size_t>(7 - i)] = kDigits[(value >> (4 * i)) & 0xF];
    }
    return text;
}

// 32 bits as 32 '0'/'1' characters, most significant bit first. When a
// separator is given it is inserted after every 8 bits (except at the
// very end), which marks the boundaries between the four octets.
std::string to_binary(std::uint32_t value, char separator) {
    std::string text;
    for (int bit = 31; bit >= 0; --bit) {
        text += ((value >> bit) & 1u) != 0 ? '1' : '0';
        if (separator != '\0' && bit != 0 && bit % 8 == 0) {
            text += separator;
        }
    }
    return text;
}

// Dispatches on which type the variant currently holds. std::get_if
// returns a pointer when the variant holds an Ipv4Address and nullptr
// otherwise; since there are only two alternatives, "not IPv4" means IPv6
// and std::get is safe.
std::string render(const IpAddress& address, OutputFormat format) {
    if (const auto* ipv4 = std::get_if<Ipv4Address>(&address)) {
        return render_ipv4(*ipv4, format);
    }
    return render_ipv6(std::get<Ipv6Address>(address), format);
}

}  // namespace ipnum