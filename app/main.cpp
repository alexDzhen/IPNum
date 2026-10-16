// main.cpp
// Entry point of ipnum. Ties the modules together: parse the command line
// (cli), then for every address either given as an argument or read from
// standard input, parse it (ip_address) and print it in the chosen format
// (format). Also provides an interactive prompt when run without arguments
// in a terminal.

#include <cstdio>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

// isatty() lives in a different header and under a different name on
// Windows (_isatty) and on POSIX systems (isatty).
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include "../include/cli.hpp"
#include "../include/ip_address.hpp"
#include "format.hpp"

namespace {

// True when standard input is a keyboard (a terminal) rather than a file
// or a pipe. Decides whether to show the interactive prompt: with
// `echo 10.0.0.1 | ipnum` nobody is typing, so no prompt is printed.
bool stdin_is_terminal() {
#ifdef _WIN32
    return _isatty(_fileno(stdin)) != 0;
#else
    return isatty(STDIN_FILENO) != 0;
#endif
}

// Removes spaces, tabs and line endings from both ends of the text, so a
// line read from a file with Windows "\r\n" endings or a pasted address
// with a trailing space still parses. string_view means no copy is made.
std::string_view trim(std::string_view text) {
    static const char kWhitespace[] = " \t\r\n";
    const std::size_t first = text.find_first_not_of(kWhitespace);
    if (first == std::string_view::npos) {
        return {};
    }
    const std::size_t last = text.find_last_not_of(kWhitespace);
    return text.substr(first, last - first + 1);
}

// Converts addresses one at a time and remembers whether any of them was
// invalid, which decides the exit code at the end.
class Converter {
public:
    explicit Converter(ipnum::OutputFormat format) : format_(format) {}

    // Parses one address and prints it. Invalid input goes to standard
    // error with the reason, so it does not get mixed into piped output.
    void convert(std::string_view input) {
        const auto address = ipnum::parse_ip(input);
        if (!address) {
            std::cerr << "ipnum: '" << input
                      << "' is not a valid IP address: " << ipnum::describe(address.error())
                      << "\n";
            all_valid_ = false;
            return;
        }
        // The multi-line text report gets a blank line between addresses;
        // the one-line formats (dec, hex, json...) do not, so they stay
        // easy to process with other tools.
        if (format_ == ipnum::OutputFormat::Text && printed_any_) {
            std::cout << "\n";
        }
        std::cout << ipnum::render(*address, format_) << "\n";
        printed_any_ = true;
    }

    bool all_valid() const { return all_valid_; }

private:
    ipnum::OutputFormat format_;
    bool printed_any_ = false;
    bool all_valid_ = true;
};

// Reads addresses from standard input, one per line, until end of input.
// In a terminal it behaves as a small interactive tool: shows a prompt and
// stops at an empty line. When input is piped in, blank lines are simply
// skipped and reading continues to the end of the data.
void convert_standard_input(Converter& converter) {
    const bool interactive = stdin_is_terminal();
    if (interactive) {
        std::cout << "Enter an IPv4 or IPv6 address per line. An empty line quits.\n";
    }

    std::string line;
    while (true) {
        if (interactive) {
            std::cout << "> " << std::flush;
        }
        // getline fails at end of input (Ctrl+D on Linux, Ctrl+Z on Windows,
        // or the end of a piped file).
        if (!std::getline(std::cin, line)) {
            break;
        }
        const std::string_view input = trim(line);
        if (input.empty()) {
            if (interactive) {
                break;
            }
            continue;
        }
        converter.convert(input);
    }
}

// Parses the arguments and dispatches: help, version, or conversion.
// A bad command line is reported with the conventional exit code 2.
int run(const std::vector<std::string>& args) {
    ipnum::CliOptions options;
    try {
        options = ipnum::parse_cli(args);
    } catch (const ipnum::UsageError& error) {
        std::cerr << "ipnum: " << error.what() << "\nTry 'ipnum --help' for more information.\n";
        return 2;
    }

    if (options.show_help) {
        std::cout << ipnum::usage_text();
        return 0;
    }
    if (options.show_version) {
        // IPNUM_VERSION is defined by CMake from the project version.
        std::cout << "ipnum " << IPNUM_VERSION << "\n";
        return 0;
    }

    // Addresses on the command line are converted directly; with none
    // given, they are read from standard input instead.
    Converter converter{options.format};
    if (options.addresses.empty()) {
        convert_standard_input(converter);
    } else {
        for (const std::string& address : options.addresses) {
            converter.convert(trim(address));
        }
    }
    // Exit code 1 if at least one address was invalid, so scripts can
    // detect bad input.
    return converter.all_valid() ? 0 : 1;
}

}  // namespace

// argv[0] is the program name, so the arguments proper are argv[1..argc).
// Any unexpected exception ends up here as a one-line message.
int main(int argc, char* argv[]) {
    try {
        return run(std::vector<std::string>(argv + 1, argv + argc));
    } catch (const std::exception& error) {
        std::cerr << "ipnum: " << error.what() << "\n";
        return 1;
    }
}