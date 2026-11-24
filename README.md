# Bubu Chess Engine

**Bubu** is an open-source chess engine project from Indonesia, written from
scratch in C++20. The project aims to grow into a correct, efficient, and
well-documented UCI engine through small, verifiable development steps.

[![CI](https://github.com/hmuazzam/Bubu/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/hmuazzam/Bubu/actions/workflows/ci.yml)
[![License: GPL-3.0](https://img.shields.io/badge/License-GPL--3.0-blue.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C.svg)](https://en.cppreference.com/w/cpp/20.html)
[![Build: Make](https://img.shields.io/badge/build-Makefile-427819.svg)](Makefile)
[![Development status](https://img.shields.io/badge/status-early%20development-orange.svg)](#project-status)
[![GitHub stars](https://img.shields.io/github/stars/hmuazzam/Bubu)](https://github.com/hmuazzam/Bubu/stargazers)
[![Open issues](https://img.shields.io/github/issues/hmuazzam/Bubu)](https://github.com/hmuazzam/Bubu/issues)

> **Project status:** early development (M0, repository setup). Bubu is not yet
> a playable chess engine and does not implement the UCI protocol.

## Project goals

Bubu is being developed as a classical chess engine first, with no neural
network in the initial release. The long-term MVP goals are:

- A legal move generator validated against established perft positions.
- Reliable UCI support for common chess interfaces and tournament tools.
- Deterministic search benchmarks and repeatable strength testing.
- Original, documented implementation, with no copied engine code or
  evaluation tables.
- A measured strength target of **at least 2300 Elo** in engine-vs-engine
  testing. This is a goal, not a current rating or a claim of achieved strength.

After the classical MVP, the project may explore self-play tuning and NNUE.
See the development plan and design notes in [`docs/`](docs/).

## Project status

The repository currently contains a minimal C++20 executable, a Makefile, a CI
workflow, and initial project documentation. Running `./bubu` prints the
project name. The test target is currently a smoke check, and the bench target
is a placeholder until search is implemented.

| Capability | Status |
| --- | --- |
| C++20 release build | Available |
| Debug build with AddressSanitizer and UndefinedBehaviorSanitizer | Available |
| CI builds with GCC and Clang | Configured |
| Unit tests, chess rules, and perft | Not implemented |
| UCI protocol and playable engine | Not implemented |
| Search, evaluation, and node-count benchmark | Not implemented |
| Elo testing | No results yet |

## Requirements

- GNU Make
- A C++20 compiler: GCC 11 or later, or Clang 14 or later
- Git (for cloning and contributing)

The engine has no third-party runtime dependencies. Development is intended to
remain lightweight; use `make -j2` on resource-constrained machines.

## Build and run

Clone the repository and build the release executable:

```sh
git clone https://github.com/hmuazzam/Bubu.git
cd Bubu
make -j2
./bubu
```

Expected output:

```text
Bubu chess engine scaffold
```

To build with Clang, pass the compiler explicitly:

```sh
make clean
make -j2 CXX=clang++
```

## Make targets

| Command | Description |
| --- | --- |
| `make` or `make release` | Build the optimized executable at `./bubu`. |
| `make debug` | Build `build/bubu-debug` with debug symbols, ASan, and UBSan. |
| `make test` | Run the current scaffold smoke check. |
| `make bench` | Run the bench placeholder; search and node counts are not available yet. |
| `make clean` | Remove generated build files and the release executable. |

The release build uses `-O3 -DNDEBUG`; the debug build uses
`-O1 -g -fsanitize=address,undefined`. Warnings are enabled with
`-Wall -Wextra -Wpedantic`.

## Development roadmap

Work follows the milestone and task sequence defined by the project plan:

1. **Setup:** repository structure, build system, CI, and documentation
   *(current)*.
2. **Board foundation:** core types, bitboard utilities, position state, FEN,
   hashing, and reversible moves.
3. **Move generation:** attack tables, legal move generation, and perft.
4. **UCI and basic search:** protocol support and a first playable engine.
5. **Search and evaluation:** incremental improvements measured with
   deterministic benchmarks and engine matches.
6. **Tuning and release:** self-play data, parameter tuning, and a measured
   public release.

Correctness comes before optimization. Search and evaluation changes should be
measured, and performance or Elo results should only be reported after they
have actually been run. See [`docs/DESIGN.md`](docs/DESIGN.md),
[`docs/SOURCES.md`](docs/SOURCES.md), and [`docs/ELO_LOG.md`](docs/ELO_LOG.md).

## Contributing

Contributions and careful reviews are welcome. Before making changes, read
[`AGENTS.md`](AGENTS.md) and the project plan. Work on one roadmap task at a
time, keep changes within the documented scope, and include reproducible
verification results. Do not weaken tests to make a change pass or report
unmeasured strength claims.

The project is intended to be original. Implement general chess-engine ideas
independently, and document references for new concepts in
[`docs/SOURCES.md`](docs/SOURCES.md).

## License

Bubu is distributed under the GNU General Public License version 3.0. See
[`LICENSE`](LICENSE) for the full license text.

## Acknowledgements

The project draws on publicly documented chess-programming concepts and
community testing practices. References will be recorded as those concepts are
implemented.
