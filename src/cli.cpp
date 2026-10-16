// cli.cpp
// Parses the command line into a CliOptions struct and provides the --help
// text. ipnum has only one real option (--format), so the parser is a
// simple loop rather than a table. Anything wrong on the command line is
// reported by throwing UsageError, which main() turns into a message and
// exit code 2.
//
// Supported forms:  -f dec   --format dec   --format=dec   and "--" to end options.

#include "../include/cli.hpp"

namespace ipnum {

namespace {

// Turns the text after --format into an OutputFormat, or throws a usage
// error listing the valid names.
OutputFormat require_format(const std::string& name) {
    const auto format = parse_output_format(name);
    if (!format) {
        throw UsageError("unknown format '" + name +
                         "' (expected text, dec, hex, bin, words or json)");
    }
    return *format;
}

}  // namespace

// Walks the arguments (everything after the program name) once, from left
// to right. Anything that is not a recognised option is collected as an
// address to convert; any number of them is allowed, including none.
CliOptions parse_cli(const std::vector<std::string>& args) {
    static const std::string kFormatPrefix = "--format=";

    CliOptions options;
    bool options_ended = false;

    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];

        // Not an option: an address (or anything after "--").
        if (options_ended || arg.empty() || arg[0] != '-') {
            options.addresses.push_back(arg);
            continue;
        }
        // "--" means "no more options"; whatever follows is an address even
        // if it starts with '-'.
        if (arg == "--") {
            options_ended = true;
            continue;
        }
        if (arg == "-h" || arg == "--help") {
            options.show_help = true;
            continue;
        }
        if (arg == "-V" || arg == "--version") {
            options.show_version = true;
            continue;
        }
        // "-f dec" / "--format dec": the value is the next argument. The ++i
        // consumes it so the loop does not treat it as an address.
        if (arg == "-f" || arg == "--format") {
            if (i + 1 >= args.size()) {
                throw UsageError("option '" + arg + "' needs a value");
            }
            options.format = require_format(args[++i]);
            continue;
        }
        // "--format=dec": the value is attached after the '='.
        if (arg.compare(0, kFormatPrefix.size(), kFormatPrefix) == 0) {
            options.format = require_format(arg.substr(kFormatPrefix.size()));
            continue;
        }
        throw UsageError("unknown option '" + arg + "'");
    }

    return options;
}

// The text shown by --help. Adjacent string literals are joined by the
// compiler into one string, which keeps the layout readable in the source.
std::string usage_text() {
    return "Usage: ipnum [options] [address...]\n"
           "\n"
           "Converts IPv4 and IPv6 addresses to their numeric value.\n"
           "An IPv4 address becomes one 32-bit number. An IPv6 address is 128 bits wide,\n"
           "so it is shown as one 128-bit number and as four 32-bit numbers.\n"
           "With no address arguments, addresses are read from standard input, one per line.\n"
           "\n"
           "Options:\n"
           "  -f, --format <name>  Output format (default: text)\n"
           "                         text   full report\n"
           "                         dec    decimal number only\n"
           "                         hex    hexadecimal number only\n"
           "                         bin    binary number only\n"
           "                         words  32-bit numbers separated by spaces\n"
           "                         json   one JSON object per address\n"
           "  -h, --help           Show this help and exit\n"
           "  -V, --version        Show the version and exit\n"
           "\n"
           "Examples:\n"
           "  ipnum 192.168.1.1\n"
           "  ipnum --format dec 10.0.0.1 8.8.8.8\n"
           "  ipnum 2001:db8::1\n"
           "\n"
           "Exit status: 0 on success, 1 if an address is invalid, 2 on a usage error.\n";
}

}  // namespace ipnum