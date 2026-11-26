# Bubu Chess Engine: Product Requirements

> Draft v1.0 | Project owner: Bubu maintainers | Language: universal English
>
> This document is the single source of truth for project scope and agent
> workflow.

## 1. Summary

Bubu is an original, open-source UCI chess engine written from scratch in
C++20. It is an Indonesian project with a grey-cat mascot and a long-term
development plan.

Strategic direction:

1. **MVP (v0.1):** a classical engine without a neural network, targeting at
   least 2300 Elo on an engine rating scale, measured in engine matches.
2. **After the MVP:** seek server donations, explore NNUE, and aim for 3000+ Elo.
3. **Long term:** pursue rating-list submissions, a Lichess bot, and chess
   events such as WCCC/ICGA. Check current organizer requirements before entry.

## 2. Background and problem

- The project aims to make an Indonesian chess engine visible to the engine
  programming community.
- The owner is a vibe coder using a laptop with 4 GB RAM. Development must stay
  lightweight, structured, and automatically verifiable because much of the
  code is AI-assisted.
- The main risks are subtle implementation bugs and the perception that the
  engine is derivative. Correctness and original work must be documented.

## 3. Goals and non-goals

### MVP goals

| ID | Goal |
|----|------|
| G1 | The legal move generator is correct and passes the required perft suite. |
| G2 | The engine supports UCI and works with common interfaces such as Cute Chess, Arena, and lichess-bot. |
| G3 | Reach at least 2300 Elo, supported by a tournament of at least 1,000 games against reference engines. |
| G4 | Measure every algorithm change with SPRT or a documented benchmark. |
| G5 | Keep the implementation original, document idea sources, and make the code suitable for tournament review. |
| G6 | Publish v0.1.0 binaries for Windows and Linux and provide a Lichess bot setup. |

### MVP non-goals

- Neural networks, including NNUE (planned for a later phase).
- A custom GUI, a large opening book, Syzygy tablebases, or Lazy SMP
  multithreading.
- Chess variants. Chess960 may be considered later.

## 4. Users

| Persona | Need |
|---------|------|
| Owner/developer | A clear workflow, automatic tests, and fast bug detection. |
| Indonesian chess players and community | A way to play Bubu locally or on Lichess. |
| GUI and tournament users | A stable UCI binary and measured playing strength. |
| Donors and contributors | A tidy repository, transparent progress, and a clear project story. |

## 5. Success metrics

| Metric | MVP target |
|--------|------------|
| Perft | Every position in `tests/perft.epd` matches its expected count. |
| Illegal moves or crashes in 1,000 games | 0 |
| Playing strength | At least 2300 Elo (see Section 10). |
| Speed | At least 1 Mnps on one thread as an indicative, hardware-dependent goal. |
| Benchmark | Deterministic node count across repeated runs. |
| CI | Green build, perft, benchmark, and sanitizer checks. |

> **Elo reporting:** 2300 is a minimum target, not a result or an upper limit.
> Engine Elo is not human Elo from Lichess or Chess.com. Always report the
> reference engines, test conditions, and uncertainty.

## 6. Technology stack

| Component | Choice | Reason |
|-----------|--------|--------|
| Language | C++20 | Performance and no runtime dependency beyond the standard library. |
| Compiler | g++ 11+ or clang 14+ | Broad compiler support with modest memory use. |
| Build | Makefile; CMake may be considered later | Simple to build and debug. |
| Runtime dependencies | None beyond the standard library | Portability and tournament reliability. |
| Release flags | `-O3 -DNDEBUG`; portable and local ISA variants are separate targets | Keep the default release practical across machines. |
| Debug flags | `-O1 -g -fsanitize=address,undefined` | Detect memory and undefined-behavior bugs. |
| Version control | Git and GitHub | Required from the start. |
| CI | GitHub Actions | Automate builds, perft, and benchmark checks. |
| Engine matches | Cute Chess (`cutechess-cli`) or fastchess | SPRT and gauntlet testing. |
| Support scripts | Python 3 | Tuning and result analysis only; not part of the engine. |
| Lichess bot | Community `lichess-bot` project | Connect UCI to Lichess. |
| Post-MVP | NNUE trainer such as bullet, OpenBench, rented GPU time | Later development phase. |
| License | GPL-3.0 by default | Common open-source engine license. |

**Development machine limits:** use `make -j2`, a 16 MB default hash (test no
more than 64 MB locally), one engine thread, and at most two concurrent match
games. Heavy testing can run in CI or on another machine.

## 7. Architecture and repository layout

```text
bubu/
  src/
    types.h          # Square, Piece, Color, Move, and constants
    bitboard.h/.cpp  # Bitboard operations and attack tables
    position.h/.cpp  # Board state, FEN, make/unmake, Zobrist, repetition
    movegen.h/.cpp   # Legal move generation
    eval.h/.cpp      # Tapered middlegame/endgame evaluation
    search.h/.cpp    # PVS, iterative deepening, quiescence, pruning
    uci.h/.cpp       # UCI protocol
    bench.cpp        # Benchmark command
    main.cpp
  tests/
    perft.epd        # Positions and expected perft results
    tactics.epd      # Tactical sanity checks
  tools/
    sprt.sh          # Cute Chess SPRT wrapper
    texel_tune.py    # Evaluation tuning
    openings/        # Opening data for matches
  docs/
    DESIGN.md        # Design decisions
    SOURCES.md       # Concepts and idea sources
    ELO_LOG.md       # Match results
  assets/            # Bubu artwork
  .github/workflows/ci.yml
  AGENTS.md          # Agent workflow summary
  Makefile
  README.md
  LICENSE
```

Design principles:

- Give each module one responsibility and keep interfaces small.
- Avoid hidden mutable global state. Search state belongs to `Searcher`.
- Keep tunable parameters in `params.h` or an explicitly named parameter
  structure.
- Avoid dynamic allocation in the search hot path.

## 8. Functional requirements

### 8.1 Board and move generation

| ID | Requirement |
|----|-------------|
| F-BRD-1 | Maintain 12 piece bitboards and a mailbox for square lookup. |
| F-BRD-2 | Parse and serialize valid FEN; reject malformed FEN. |
| F-BRD-3 | Make/unmake moves incrementally, including castling rights, en passant, the 50-move clock, Zobrist hash, checkers, and pins. |
| F-BRD-4 | Use deterministic 64-bit Zobrist hashing with a fixed PRNG seed. |
| F-GEN-1 | Generate all legal moves, including castling, en passant, promotions, pins, and check evasions. |
| F-GEN-2 | Use magic bitboards or PEXT in an appropriate BMI2 build for sliders. |
| F-GEN-3 | Provide separate capture/promotion, quiet, and evasion generation modes. |
| F-GEN-4 | Provide `perft <depth>` and `perft divide <depth>` commands. |

### 8.2 UCI

| ID | Requirement |
|----|-------------|
| F-UCI-1 | Support `uci`, `isready`, `ucinewgame`, `position`, `go`, `stop`, `quit`, and `setoption`. |
| F-UCI-2 | Support `go` limits for depth, nodes, movetime, clocks, increments, moves to go, and infinite search. |
| F-UCI-3 | Report depth, selective depth, score, nodes, NPS, time, hash usage, and PV. |
| F-UCI-4 | Provide `Hash` (MB, default 16, range 1–1024), fixed `Threads=1`, and `Clear Hash`. |
| F-UCI-5 | Run search in a worker thread so `stop` remains responsive. |
| F-UCI-6 | Provide non-standard `bench`, `d`, and `eval` commands. |
| F-UCI-7 | Provide the experimental `KingSafetyV2` check option, disabled by default. |

### 8.3 Search

The classical search targets the 2000–2800 engine-strength range without a
neural network.

| ID | Feature | Notes |
|----|---------|-------|
| F-SRC-1 | Negamax, alpha-beta, and PVS | Search foundation. |
| F-SRC-2 | Iterative deepening and aspiration windows | Widen the window after a fail. |
| F-SRC-3 | Quiescence search | Captures, promotions, check evasions, SEE and delta pruning. |
| F-SRC-4 | Transposition table | Zobrist keys, bucket replacement, generation aging, mate-score normalization. |
| F-SRC-5 | Staged move ordering | TT move, good captures, killers, countermove, history, quiet moves, bad captures. |
| F-SRC-6 | Null-move pruning | Adaptive reduction; require non-pawn material. |
| F-SRC-7 | Late-move reductions | Log-based reduction table adjusted for PV, history, and improving status. |
| F-SRC-8 | Reverse-futility and futility pruning | Static evaluation margins. |
| F-SRC-9 | Late-move pruning | Limit late moves at shallow depths. |
| F-SRC-10 | Check extension and internal iterative reduction | Extend checks; reduce when no TT move is available. |
| F-SRC-11 | Static exchange evaluation | Move ordering and pruning. |
| F-SRC-12 | Draw detection | Repetition, 50-move rule, and insufficient material. |
| F-SRC-13 | Mate-distance pruning | Keep mate scores consistent with search distance. |
| F-SRC-14 | Time management | Soft/hard limits, clock allocation, and extensions after score or best-move changes. |

Lazy SMP, singular extensions, probcut, countermove history, and other ideas
remain post-MVP candidates. Add them only when testing supports the change.

### 8.4 Classical tapered evaluation

| ID | Requirement |
|----|-------------|
| F-EVL-1 | Material and formula-based piece-square bonuses blended by game phase. |
| F-EVL-2 | Doubled, isolated, backward, passed, connected, and phalanx pawn terms. |
| F-EVL-3 | Piece mobility, bishop pair, open/semi-open rook files, and seventh-rank rooks. |
| F-EVL-4 | King shelter, king-zone attacks, and open files near the king. |
| F-EVL-5 | Tempo bonus and simple endgame scaling. |
| F-EVL-6 | Tune parameters from Bubu self-play data; do not copy other engines' tables. |
| F-EVL-7 | Consider a pawn hash table only if measured to help. |
| F-EVL-8 | Keep `KingSafetyV2` opt-in until a paired match test supports enabling it. |

### 8.5 Tooling and quality

| ID | Requirement |
|----|-------------|
| F-TST-1 | `make test` runs perft, tactical checks, FEN, and make/unmake tests. |
| F-TST-2 | `bench` searches fixed positions at fixed depth and reports nodes/NPS. Node count is a signature. |
| F-TST-3 | CI builds with GCC and Clang, runs perft/bench, and builds a sanitizer binary. |
| F-TST-4 | `tools/sprt.sh` runs SPRT, with a default example of Elo bounds 0 and 5 and alpha/beta 0.05. |

## 9. Non-functional requirements

- Correctness comes before speed, and speed before extra features.
- With one thread, repeated benchmark node counts must match.
- The engine must work with a 16 MB hash and pass ASan/UBSan checks.
- Target Windows (MinGW) and Linux without external runtime dependencies.
- Use clear names, short functions, and comments that explain why non-trivial
  code exists.
- Do not copy engine code. Record conceptual references in `docs/SOURCES.md`.

## 10. Strength measurement strategy

1. Never claim Elo without data. Record version, game count, W/D/L, time
   control, opponents, Elo, and uncertainty in `docs/ELO_LOG.md`.
2. Select four to six open-source reference engines around 2000–2700 CCRL Elo;
   check current CCRL ratings before choosing. Stockfish limit-strength mode
   can be an additional rough reference, not a calibrated rating source.
3. Run at least 500 games per opponent at 10+0.1, Hash 16 MB, one thread, and
   random openings. Use no more than two concurrent games on the development
   laptop.
4. Test each search/evaluation change against the previous version with SPRT.
5. The MVP gate is an aggregate result of at least 2300 with a reported error
   margin, calculated with a recognized rating tool.

## 11. Roadmap and acceptance criteria

The initial estimate is 10–15 hours per week for one developer with AI support.
Keep the milestone order intact.

- **M0 — Setup:** repository, Makefile, CI, license, README, and design/source
  documents. Done when builds and CI pass.
- **M1 — Board and FEN:** core types, bitboard utilities, `Position`, FEN,
  hashing, and make/unmake. Done after at least 50 FEN round-trips and matching
  incremental/recomputed hashes in debug.
- **M2 — Move generation and perft:** attack tables, magic bitboards, legal
  move generation, and perft. Done when all required positions below match and
  ASan/UBSan pass. Later, add at least 100 public perft cases.
- **M3 — UCI and first games:** full UCI and basic search. Done after 100 games
  against a random mover with no illegal moves or crashes and responsive stop.
- **M4 — Core search:** PVS, iterative deepening, quiescence, TT, move ordering,
  SEE, draw detection, time management, and tapered PST. Done after stable
  bench and positive SPRT evidence for core components.
- **M5 — Pruning and richer evaluation:** aspiration, NMP, LMR, RFP, futility,
  LMP, check extension, IIR, mate-distance pruning, pawn structure, mobility,
  king safety, and other classical terms. Done when each feature passes SPRT
  or is removed, and the MVP rating gate is met.
- **M6 — Tuning and release:** self-play data, Texel tuning, v0.1.0 release,
  Lichess setup, and final ELO_LOG results.
- **M7 — Post-MVP:** NNUE and further strength improvements.

Required perft cases:

| Position | Depth | Nodes |
|----------|-------|-------|
| Start position | 5 | 4,865,609 |
| Kiwipete: `r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -` | 4 | 4,085,603 |
| Position 3: `8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -` | 5 | 674,624 |
| Position 4: `r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1` | 4 | 422,333 |
| Position 5: `rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8` | 4 | 2,103,487 |
| Position 6: `r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10` | 4 | 3,894,594 |

## 12. Atomic tasks for agents

Format: `T-<milestone>-<number>`. One task is one session and one commit.
Finish only after its acceptance criteria and tests pass.

| Task | Description | Verification |
|------|-------------|--------------|
| T0-1 | Repository scaffold, Makefile, CI | Build and CI pass. |
| T1-1 | Core types and bitboard utilities | Unit checks. |
| T1-2 | `Position` and FEN parsing/serialization | 50 FEN round-trips. |
| T1-3 | Zobrist and make/unmake | Incremental hash equals recomputed hash. |
| T2-1 | Non-slider attack tables | Unit checks. |
| T2-2 | Magic slider attacks | Match a naive generator over tested squares/occupancies. |
| T2-3 | Legal move generation | Required perft positions match. |
| T2-4 | Perft CLI and test runner | `make test` passes. |
| T3-1 | UCI loop, position, and depth search | Complete a game in Cute Chess. |
| T3-2 | Search thread, stop, and time controls | Stop responds in under 50 ms. |
| T4-1 | Negamax, alpha-beta, iterative deepening, material evaluation | Stable benchmark. |
| T4-2 | Quiescence | SPRT against T4-1. |
| T4-3 | Transposition table | SPRT. |
| T4-4 | Staged move ordering | SPRT. |
| T4-5 | SEE and capture ordering/pruning | SPRT. |
| T4-6 | PVS, draw detection, and mate scores | SPRT. |
| T4-7 | Tapered PST evaluation | SPRT. |
| T4-8 | Time management | No lost-on-time games in the fixed test. |
| T5-1 | Aspiration windows | SPRT. |
| T5-2 | Null-move pruning | SPRT. |
| T5-3 | LMR | SPRT. |
| T5-4 | RFP, futility, and LMP | SPRT. |
| T5-5 | Check extension, IIR, mate-distance pruning | SPRT. |
| T5-6 | Pawn-structure evaluation | SPRT. |
| T5-7 | Mobility, bishop pair, and rook files | SPRT. |
| T5-8 | King-safety evaluation | SPRT. |
| T5-9 | Reference-engine gauntlet and rating log | Update `ELO_LOG.md`. |
| T6-1 | Self-play data generation | Position/result data is produced. |
| T6-2 | Texel tuning and parameter update | SPRT against the pre-tuned version. |
| T6-3 | v0.1.0 release, README, and Lichess bot | Public release. |

## 13. Testing strategy

| Layer | Tool | When |
|-------|------|------|
| Unit tests | Small built-in test runner | Every commit. |
| Perft | `make test` | Every board or move-generation change. |
| Benchmark signature | `./bubu bench` | Every commit; record the node count in the commit body. |
| Sanitizers | ASan and UBSan build | Before merge and in scheduled CI. |
| UCI behavior | Cute Chess or protocol tests | Every milestone. |
| Playing strength | SPRT/gauntlet | Every search/evaluation change. |

Default SPRT example: 10+0.1, Hash 16 MB, one thread, random openings, bounds
`[0, 5]` for a gain or `[-3, 1]` for a simplification, alpha=beta=0.05.

## 14. Risks and mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| Subtle AI-assisted code bugs | Very high | Perft, benchmark signature, SPRT, sanitizers, and manual review. |
| Derivative-engine concerns | High | Independent implementation, source notes, and self-play-tuned parameters. |
| Limited laptop memory | Medium | `-j2`, small hash, CI for heavy testing, and cloud training. |
| Scope creep | High | Keep non-goals clear; require measurement for new features. |
| Loss of motivation | High | Small milestones, an early release, and visible ELO_LOG progress. |
| Name collision | Low/medium | Check CCRL, GitHub, and Lichess before release; keep a backup name. |
| Noisy match results | Medium | Enough games, randomized openings, and reported uncertainty. |

## 15. Open questions for the owner

1. Final license: GPL-3.0 or MIT?
2. Which operating system is used on the development laptop: Windows,
   MinGW/MSYS2, or WSL2; or Linux?
3. Is “Bubu” the final name? Who will create original cat artwork? Avoid
   copying a recognizable copyrighted character.
4. Which reference engines should be selected after checking current CCRL lists?
5. Which donation platform should be used?

## 16. AI agent workflow

1. Read `PRD.md` and `docs/` before writing code. Work on one Section 12 task
   per session.
2. Do not skip milestones or add features outside the PRD without approval.
3. Write original code from general concepts. Do not copy engine code or
   tables. Record concepts and sources in `docs/SOURCES.md`.
4. Prefer tests first, implement the task, run `make test` and `make bench`,
   then commit.
5. Use Conventional Commits and include `Bench: <nodes>` when search or
   evaluation changes.
6. Do not silently change engine behavior. Search/evaluation changes require
   SPRT results or a clear explanation.
7. Explain non-trivial code and finish with a beginner-friendly summary.
8. Respect machine limits: `make -j2`, a small hash, and one local thread.
9. If tests fail repeatedly, stop and report attempts. Never guess or disable
   tests to claim success.
