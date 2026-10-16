# Changelog

All notable changes to this project are documented in this file.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/)
and the project uses [Semantic Versioning](https://semver.org/).

## [1.0.0] - 2026-10-13

### Added

- IPv4 parsing and conversion to a 32-bit number.
- IPv6 parsing (full form, `::` compression, IPv4 tail, zone suffix) and conversion
  to a 128-bit number and to four 32-bit numbers.
- Output formats: text report, decimal, hexadecimal, binary, 32-bit words and JSON.
- Interactive mode and reading addresses from standard input.
- Unit tests, command-line tests and a cross-check against the operating system parser.
- CMake build for Linux and Windows, CI and release workflows.
