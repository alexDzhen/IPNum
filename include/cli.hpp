// cli.hpp
// Declares the command-line interface of ipnum: the struct holding the
// parsed options, the parser that fills it, and the --help text.
// Implemented in cli.cpp.

#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include "format.hpp"

namespace ipnum {

/// Thrown when the command line cannot be understood.
// main() catches it, prints the message plus a hint to try --help, and
// exits with code 2.
class UsageError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Everything the user can ask for, with defaults.
struct CliOptions {
    // -f / --format: how each address is printed (text report, dec, hex...).
    OutputFormat format = OutputFormat::Text;
    // -h / -V: print and exit without converting anything.
    bool show_help = false;
    bool show_version = false;
    // The addresses given as arguments, in order. Empty means "read them
    // from standard input instead".
    std::vector<std::string> addresses;
};

/// Parses the arguments that follow the program name.
// Takes a vector of strings rather than argc/argv so the tests can call it
// with literal lists like {"--format", "dec", "10.0.0.1"}.
CliOptions parse_cli(const std::vector<std::string>& args);

/// The text printed for --help.
std::string usage_text();

}  // namespace ipnum