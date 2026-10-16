// parse_result.cpp
// The one function in this module that needs a .cpp file: the text for
// each ParseError, used in the message main() prints for invalid input:
//   ipnum: '300.1.1.1' is not a valid IP address: every octet must be between 0 and 255
// The ParseResult template itself is entirely in the header.

#include "../include/parse_result.hpp"

namespace ipnum {

// Returns a string_view to a string literal, which lives for the whole
// program, so no allocation and no dangling pointer. Written so a user
// who does not know the RFCs can still tell what to fix.
std::string_view describe(ParseError error) noexcept {
    switch (error) {
        case ParseError::None:
            return "no error";
        case ParseError::Empty:
            return "the address is empty";
        case ParseError::InvalidCharacter:
            return "the address contains a character that is not allowed here";
        // IPv4
        case ParseError::EmptyOctet:
            return "an octet is missing between the dots";
        case ParseError::LeadingZero:
            return "octets must not have leading zeros";
        case ParseError::OctetOutOfRange:
            return "every octet must be between 0 and 255";
        case ParseError::WrongOctetCount:
            return "an IPv4 address has exactly four octets";
        // IPv6
        case ParseError::UnexpectedColon:
            return "a ':' is in a place where a group was expected";
        case ParseError::MultipleCompression:
            return "'::' may appear only once";
        case ParseError::GroupTooLong:
            return "a group may have at most four hexadecimal digits";
        case ParseError::TooManyGroups:
            return "an IPv6 address has at most eight groups";
        case ParseError::TooFewGroups:
            return "an IPv6 address needs eight groups or a '::'";
        case ParseError::InvalidEmbeddedIpv4:
            return "the IPv4 part at the end of the address is not valid";
        case ParseError::EmptyZone:
            return "the zone after '%' is empty";
    }
    // Every enumerator is handled above; this only runs if an invalid
    // value is cast to ParseError, and it keeps the compiler from warning
    // about a missing return.
    return "unknown error";
}

}  // namespace ipnum