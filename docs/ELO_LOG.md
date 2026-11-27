# Elo Measurement Log

This file records **only matches that were actually played**. Every entry lists the
version, game count, time control, opponent, W/D/L, and the Elo estimate with its
error margin. Nothing here is a claim of official strength.

## Current status

- **MVP gate (>= 2300 Elo) is NOT formally verified yet.**
- Early indicative results are consistent with strength around or above the gate
  (see the entries below), but they come from small samples on a slow machine and
  from opponents whose ratings were measured under different conditions.
- No official CCRL rating exists for Bubu.

## Test setup

| Item | Value |
|------|-------|
| Match tool | fastchess alpha 1.8.3 (20261121-af12bde) |
| Hardware | Intel Celeron N4020 (2 cores, no AVX), 4 GB RAM, Parrot OS |
| Bubu settings | 1 thread, Hash 16 MB |
| Opponent hash | 16 MB where the engine supports the option |
| Time control | 10+0.1 unless stated otherwise |
| Concurrency | 1 game at a time |
| Openings | `noob_3moves.epd` from `official-stockfish/books`, random order, `-repeat` (each opening played with both colors). The first 20-game test used no book. |
| Bubu build | Release build, current default bench signature `42654` nodes (depth 6); `KingSafetyV2=false` signature `47626` |
| Speed reference | startpos, `go depth 12`: 767,327 nodes at about 295k nps (build after the performance pass; about 183k nps before it) |

## Results

| Date | Version | Opponent | Games | TC | W-D-L | Score | Elo +/- error | Notes |
|------|---------|----------|------:|----|-------|------:|---------------|-------|
| 2026-11-25 | 92f6630 (confirm) | Stockfish, `UCI_LimitStrength`, `UCI_Elo=1500` | 20 | 5+0.05 | 16-1-3 | 82.5% | +269.37 +/- 219.00 | No opening book. Sanity run only. Margin is too wide to use. |
| 2026-11-25 | 92f6630 (confirm) | Stockfish, `UCI_Elo=1500` | 100 | 10+0.1 | 99-0-1 | 99.0% | saturated (nan) | With book. Score too high for the Elo formula to be meaningful. Used only as a sanity check. |
| 2026-11-25 | 92f6630 (confirm) | Stockfish, `UCI_Elo=2400` | 50 | 10+0.1 | 22-5-23 | 49.0% | -6.95 +/- 95.72 | Roughly balanced. Indicative only. |
| 2026-11-25 | 92f6630 (confirm) | AICE 0.99.2 (CCRL Blitz about 2300) | 50 | 10+0.1 | 43-3-4 | 89.0% | +363.20 +/- 155.42 | AICE options: `BookUsage=None`, `Resigns=false`, `AcceptsDraw=false`, Hash 16. Not used as an absolute estimate (see notes). |

## Evaluation feature SPRT

The project owner supplied the following paired `KingSafetyV2` result. The
match date, engine W-D-L, and exact version identifier were not provided, so no
Elo estimate is derived here.

| Feature comparison | Games | TC | Score | LLR | Decision | Castling rate |
|--------------------|------:|----|------:|----:|----------|---------------|
| `KingSafetyV2=true` vs `false` | 810 | 5+0.05 | 54.1% | 3.01 | H1 accepted | 98% vs 16% |

## Validation checks

| Check | Result |
|-------|--------|
| Perft (6 reference positions) | All node counts match |
| Bench determinism | Default: 42,654 nodes; `KingSafetyV2=false`: 47,626 nodes |
| UCI compliance (`fastchess --compliance`) | Passed all 40 steps |
| Sanity matches vs Stockfish, 120 games total | All games ended normally; 0 illegal moves, forfeits, disconnects, or stalls |
| Match vs AICE, 50 games | All games ended `normal`; 0 illegal moves, forfeits, disconnects, or stalls |

## Notes and caveats

- **Stockfish `UCI_Elo` is only a rough scale.** Its calibration targets longer time
  controls than these tests, so a limited Stockfish may play stronger or weaker than
  its nominal rating at 10+0.1 on this CPU. Results against it are indicative only.
- The warning `Bestmove does not match beginning of last PV` seen in the Stockfish
  matches comes from Stockfish in limited-strength mode, not from Bubu.
- **AICE result is not converted to an absolute rating.** CCRL ratings were measured
  on an Intel i7-4770k with much longer time controls, and the opponent is an old
  32-bit engine that may react differently to fast time controls. A score this far
  from 50% carries a wide error margin.
- After the AICE match, fastchess printed `double free or corruption (!prev)` once.
  All 50 games were already saved and the PGN is clean. Likely an issue between
  fastchess and that old engine; to be re-checked if it appears again, using a
  sanitizer build of Bubu.
- Rating lists differ in scale (CCRL 40/15 vs CCRL Blitz). Never mix them in one
  calculation.

## How to add an entry

1. Run the match with fixed settings (see Test setup).
2. Check the PGN: `grep -i "Termination" file.pgn | sort | uniq -c` and
   `grep -ci "illegal\|forfeit\|disconnect\|stall" file.pgn`. Everything must be
   `normal` and the second count must be `0`.
3. Add one row to the Results table: date, commit hash, opponent and its settings,
   game count, time control, W-D-L, score, Elo with error margin, and notes.
4. Do not report an Elo number without its error margin and sample size.

## Planned measurements

- [ ] Gauntlet vs engines at about 2400 and about 2570 CCRL Blitz (Eeyore 1.52, Amyan 1.72, or alternatives), 100+ games each.
- [ ] SPRT of search features (pruning ON vs OFF, then per feature).
- [ ] Larger gauntlet (200-300 games per opponent) with consistent opponent settings for a defensible estimate.
- [ ] Submit Bubu to CCRL for an official rating once the gauntlet supports the MVP target.
