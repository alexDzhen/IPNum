# ipnum

[![CI](https://github.com/alexDzhen/ipnum/actions/workflows/ci.yml/badge.svg)](https://github.com/alexDzhen/ipnum/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![Platforms](https://img.shields.io/badge/platforms-Linux%20%7C%20Windows-lightgrey.svg)

A small command-line tool that converts an IPv4 or IPv6 address to its numeric value.

*Български: [README.bg.md](README.bg.md)*

```console
$ ipnum 192.168.1.1
Address  : 192.168.1.1
Version  : IPv4 (32-bit)
Decimal  : 3232235777
Hex      : 0xC0A80101
Binary   : 11000000.10101000.00000001.00000001
```

## Features

- IPv4 to a 32-bit number, in decimal, hexadecimal and binary.
- IPv6 to a 128-bit number and to four 32-bit numbers.
- Its own parser, written from scratch: no `inet_pton`, no third-party libraries.
- Clear messages that say why an address is invalid.
- Script-friendly output (`dec`, `hex`, `bin`, `words`, `json`) and meaningful exit codes.
- Interactive mode, or addresses piped in on standard input.
- Builds with one command on Linux and Windows (GCC, Clang, MSVC, MinGW).

## Quick start

You need CMake 3.20 or newer and a C++17 compiler.

**Linux**

```sh
git clone https://github.com/alexDzhen/ipnum.git
cd ipnum
./scripts/build.sh
./build/ipnum 192.168.1.1
```

**Windows**

Install Visual Studio 2022 with the "Desktop development with C++" workload and run
the commands in "Developer PowerShell for VS 2022", where `cmake` is already available.

```powershell
git clone https://github.com/alexDzhen/ipnum.git
cd ipnum
powershell -ExecutionPolicy Bypass -File scripts\build.ps1
.\build\Release\ipnum.exe 192.168.1.1
```

Both scripts run the same three commands, which you can also type yourself:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Ready-made binaries for both systems are attached to every
[release](https://github.com/alexDzhen/ipnum/releases). The Linux binary is built
on GitHub's current Ubuntu image and needs a distribution at least that recent. On
older systems, build from source.

## Usage

```text
ipnum [options] [address...]

  -f, --format <name>  text (default), dec, hex, bin, words or json
  -h, --help           Show help and exit
  -V, --version        Show the version and exit
```

One or more addresses on the command line:

```console
$ ipnum --format dec 10.0.0.1 8.8.8.8
167772161
134744072
```

An IPv6 address:

```console
$ ipnum 2001:db8::ff00:42:8329
Address  : 2001:db8::ff00:42:8329
Version  : IPv6 (128-bit)
Expanded : 2001:0db8:0000:0000:0000:ff00:0042:8329
Decimal  : 42540766411282592856904265327123268393
Hex      : 0x20010DB8000000000000FF0000428329
32-bit words, most significant first:
  [0] 0x20010DB8 = 536939960
  [1] 0x00000000 = 0
  [2] 0x0000FF00 = 65280
  [3] 0x00428329 = 4358953
```

No arguments starts interactive mode. Input can also be piped in:

```console
$ printf '10.0.0.1\n172.16.0.1\n' | ipnum --format hex
0x0A000001
0xAC100001
```

JSON, one object per address:

```console
$ ipnum --format json 8.8.8.8
{"address":"8.8.8.8","version":4,"bits":32,"decimal":"134744072","hex":"0x08080808","words":[134744072]}
```

Invalid input is reported on standard error and sets the exit code:

```console
$ ipnum 300.1.1.1
ipnum: '300.1.1.1' is not a valid IP address: every octet must be between 0 and 255
$ echo $?
1
```

| Exit code | Meaning                              |
|-----------|--------------------------------------|
| 0         | Every address was converted          |
| 1         | At least one address was invalid     |
| 2         | The command line was wrong           |

## How the conversion works

### IPv4

An IPv4 address is four bytes written in decimal. The first byte is the most
significant one, so the number is:

```text
a.b.c.d  =  a·2^24 + b·2^16 + c·2^8 + d

192.168.1.1 = 192·16777216 + 168·65536 + 1·256 + 1 = 3232235777 = 0xC0A80101
```

In code this is one shift and one OR per octet: `value = (value << 8) | octet`.

The parser is strict. It accepts exactly four octets from 0 to 255 and rejects
leading zeros (`01.2.3.4`), because some tools read those as octal.

### IPv6

An IPv6 address is 128 bits long, four times wider than a 32-bit number, so it
cannot be stored in one. `ipnum` therefore shows it in two ways:

- as a single 128-bit number, in decimal and hexadecimal;
- as four 32-bit numbers ("words"), most significant first.

C++17 has no 128-bit integer type, so the value is kept in two 64-bit halves
([`UInt128`](include/uint128.hpp)) and printed in decimal with long division by 10.

The parser handles everything RFC 4291 allows:

| Form                | Example                                   |
|---------------------|-------------------------------------------|
| Full                | `2001:0db8:0000:0000:0000:ff00:0042:8329` |
| Compressed          | `2001:db8::ff00:42:8329`, `::1`, `::`     |
| IPv4 tail           | `::ffff:192.168.1.1`, `64:ff9b::8.8.8.8`  |
| With a zone suffix  | `fe80::1%eth0` (the zone is ignored)      |

For an IPv4-mapped address (`::ffff:a.b.c.d`) the report also shows the 32-bit
value of the IPv4 address inside it. Addresses are printed back in the canonical
form defined by RFC 5952.

## Install and package

```sh
sudo cmake --install build                 # Linux: installs to /usr/local/bin
cmake --install build --config Release --prefix C:\Tools\ipnum   # Windows
```

To produce a distributable archive (`.tar.gz` on Linux, `.zip` on Windows):

```sh
cpack --config build/CPackConfig.cmake -C Release -B dist
```

Pushing a tag such as `v1.0.0` runs the release workflow, which builds, tests and
attaches both archives to a GitHub release.

## Tests

The test suite uses [doctest](https://github.com/doctest/doctest), which is included
in `third_party/`, so nothing is downloaded during the build.

- Unit tests for the IPv4 parser, the IPv6 parser, 128-bit arithmetic, the
  formatters and the command-line parser.
- Command-line tests that run the real binary.
- On Linux, a cross-check that feeds several hundred thousand random strings to both
  `ipnum` and the system's `inet_pton` and requires identical results.

```sh
ctest --test-dir build -C Release --output-on-failure
```

## Project layout

```text
ipnum/
├── app/main.cpp            Program entry point
├── include/                Public headers
├── src/                    Implementation
├── tests/                  Unit and command-line tests
├── third_party/doctest/    Test framework (single header)
├── scripts/                One-command build scripts
└── .github/workflows/      CI and release pipelines
```

The parsing and formatting code lives in a static library (`ipnum_core`) that has
no dependencies beyond the C++ standard library. The executable and the tests both
link against it.

## License

[MIT](LICENSE)
