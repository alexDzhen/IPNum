// test_cli.cpp
// Unit tests for the command-line parser (cli.cpp), written with doctest.
// Each TEST_CASE is an independent test; CHECK records a failure and keeps
// going, REQUIRE stops the test case (used before indexing a vector),
// CHECK_THROWS_AS passes only if the expression throws the given exception
// type. parse_cli() is called directly with lists of arguments.

#include <doctest.h>

#include <string>
#include <vector>

#include "../include/cli.hpp"

using ipnum::OutputFormat;
using ipnum::UsageError;

// Plain arguments become addresses, kept in the order given, and the
// defaults (text format, no help/version) are untouched.
TEST_CASE("addresses are collected in order") {
    const auto options = ipnum::parse_cli({"192.168.1.1", "::1"});
    CHECK(options.format == OutputFormat::Text);
    CHECK_FALSE(options.show_help);
    CHECK_FALSE(options.show_version);
    REQUIRE(options.addresses.size() == 2);
    CHECK(options.addresses[0] == "192.168.1.1");
    CHECK(options.addresses[1] == "::1");
}

// An empty command line is valid: main() then reads from standard input.
TEST_CASE("no arguments means read from standard input") {
    const auto options = ipnum::parse_cli({});
    CHECK(options.addresses.empty());
    CHECK(options.format == OutputFormat::Text);
}

// "-f dec", "--format hex", "--format=json", and the option placed after
// the address rather than before it.
TEST_CASE("the format option accepts every spelling") {
    CHECK(ipnum::parse_cli({"-f", "dec", "1.2.3.4"}).format == OutputFormat::Decimal);
    CHECK(ipnum::parse_cli({"--format", "hex", "1.2.3.4"}).format == OutputFormat::Hex);
    CHECK(ipnum::parse_cli({"--format=json", "1.2.3.4"}).format == OutputFormat::Json);
    CHECK(ipnum::parse_cli({"1.2.3.4", "--format", "bin"}).format == OutputFormat::Binary);
}

TEST_CASE("help and version flags are recognised") {
    CHECK(ipnum::parse_cli({"-h"}).show_help);
    CHECK(ipnum::parse_cli({"--help"}).show_help);
    CHECK(ipnum::parse_cli({"-V"}).show_version);
    CHECK(ipnum::parse_cli({"--version"}).show_version);
}

// After "--" everything is an address, even text that looks like an option.
TEST_CASE("a double dash ends option parsing") {
    const auto options = ipnum::parse_cli({"--", "--format"});
    REQUIRE(options.addresses.size() == 1);
    CHECK(options.addresses[0] == "--format");
}

// Unknown options, --format with no value after it, and a format name that
// does not exist (in both spellings).
TEST_CASE("bad command lines raise a usage error") {
    CHECK_THROWS_AS(ipnum::parse_cli({"--bogus"}), UsageError);
    CHECK_THROWS_AS(ipnum::parse_cli({"-x"}), UsageError);
    CHECK_THROWS_AS(ipnum::parse_cli({"--format"}), UsageError);
    CHECK_THROWS_AS(ipnum::parse_cli({"-f"}), UsageError);
    CHECK_THROWS_AS(ipnum::parse_cli({"--format", "xml"}), UsageError);
    CHECK_THROWS_AS(ipnum::parse_cli({"--format=xml"}), UsageError);
}

// Guards against adding an option and forgetting to document it.
TEST_CASE("the usage text mentions every option") {
    const std::string usage = ipnum::usage_text();
    CHECK(usage.find("--format") != std::string::npos);
    CHECK(usage.find("--help") != std::string::npos);
    CHECK(usage.find("--version") != std::string::npos);
}