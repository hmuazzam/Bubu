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

> **Project status:** early development. Bubu has a UCI loop and classical
> single-thread search, but its playing strength has not been measured.

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

## Project status

The repository contains a C++20 board and legal move generator, FEN support,
deterministic hashing, perft tests, UCI, classical evaluation, and a single-
thread search. Bubu can play through UCI, but has not yet been tested in a
Cute Chess tournament and has no published Elo rating.

| Capability | Status |
| --- | --- |
| C++20 release and debug builds | Available |
| GCC and Clang builds | Passing locally |
| FEN, hashing, make/unmake, and legal move generation | Available |
| Required six-position perft suite | Passing locally |
| UCI, evaluation, and single-thread search | Available; early implementation |
| Deterministic search benchmark | Available; 47,626 nodes at depth 6 |
| Cute Chess games, SPRT, and Elo | Not measured |

## Requirements

- GNU Make
- A C++20 compiler: GCC 11 or later, or Clang 14 or later
- Python 3 for the UCI smoke test and optional tuning tools
- Git (for cloning and contributing)

The engine has no third-party runtime dependencies. Development is intended to
remain lightweight; use `make -j2` on resource-constrained machines.

## Build and run

```sh
git clone https://github.com/hmuazzam/Bubu.git
cd Bubu
make -j2
```

With no arguments, `./bubu` starts the UCI command loop and waits for a GUI or
UCI commands on standard input. For command-line perft and evaluation:

```sh
./bubu perft 5
./bubu perft divide 4 'r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1'
./bubu eval
```

To build with Clang:

```sh
make clean
make -j2 CXX=clang++
```

## Make targets

| Command | Description |
| --- | --- |
| `make` or `make release` | Build the optimized executable at `./bubu`. |
| `make debug` | Build `build/bubu-debug` with debug symbols, ASan, and UBSan. |
| `make test` | Run FEN/hash/make-unmake checks, exhaustive generated-magic checks, the six-position perft suite, tactical/UCI checks, and a deterministic-node check. |
| `make bench` | Run the fixed depth-6 search benchmark and print nodes, NPS, and time. |
| `make clean` | Remove generated build files and the release executable. |

The release build uses `-O3 -DNDEBUG`; the debug build uses
`-O1 -g -fsanitize=address,undefined`. Warnings are enabled with
`-Wall -Wextra -Wpedantic`.

## UCI and local play

Bubu supports the common UCI handshake, positions and move lists, depth, node,
move-time and clock limits, `stop`, `Hash`, `Threads=1`, and `Clear Hash`. For a
quick terminal check:

```text
uci
isready
position startpos
go depth 8
```

To play a short match with [Cute Chess](https://github.com/cutechess/cutechess),
provide an opponent engine executable:

```sh
cutechess-cli \
  -engine name=Bubu cmd=./bubu proto=uci \
  -engine name=Opponent cmd=/path/to/opponent proto=uci \
  -each tc=10+0.1 option.Hash=16 \
  -games 2 -repeat -concurrency 2
```

For repeatable SPRT or a multi-engine gauntlet, see
[`tools/sprt.sh`](tools/sprt.sh) and [`tools/gauntlet.sh`](tools/gauntlet.sh).
Both require `cutechess-cli` and an openings PGN supplied by the user. Bubu's
playing strength has not yet been measured.

## Lichess bot (experimental)

Bubu speaks UCI, the engine interface used by
[lichess-bot](https://github.com/lichess-bot-devs/lichess-bot). Build Bubu and
put the executable in the bot's engine directory. Set `engine.dir`,
`engine.name`, `engine.protocol: "uci"`, and `engine.uci_options.Hash: 16` in
`config.yml`. Then follow the upstream instructions to create a bot account,
configure its token, and run `python3 lichess-bot.py`. See the upstream
[engine setup](https://github.com/lichess-bot-devs/lichess-bot/wiki/Setup-the-engine),
[installation](https://github.com/lichess-bot-devs/lichess-bot/wiki/How-to-Install),
and [run guide](https://github.com/lichess-bot-devs/lichess-bot/wiki/How-to-Run-lichess%E2%80%90bot).
This integration has not been tested, and Bubu's playing strength is unmeasured.

## Development roadmap

1. **Board and move generation:** implemented; six reference perft positions
   pass locally.
2. **UCI and classical search:** implemented; protocol smoke tests pass.
3. **Search and evaluation improvements:** early implementation; pruning
   features still need match testing.
4. **Strength testing and tuning:** pending Cute Chess, opponent engines,
   opening data, and self-play results.
5. **MVP release:** pending verification and measured strength results.

Correctness comes before optimization. Search and evaluation changes should be
measured, and performance or Elo results should only be reported after they
have actually been run. See [`docs/DESIGN.md`](docs/DESIGN.md),
[`docs/SOURCES.md`](docs/SOURCES.md), and [`docs/ELO_LOG.md`](docs/ELO_LOG.md).

## Contributing

Contributions and careful reviews are welcome. Before making changes, read
[`AGENTS.md`](AGENTS.md) and [`PRD.md`](PRD.md). Keep changes within the
documented scope, include reproducible verification results, and do not weaken
tests or report unmeasured strength claims.

The project is intended to be original. Implement general chess-engine ideas
independently, and document references for new concepts in
[`docs/SOURCES.md`](docs/SOURCES.md).

## License

Bubu is distributed under the GNU General Public License version 3.0. See
[`LICENSE`](LICENSE) for the full license text.
