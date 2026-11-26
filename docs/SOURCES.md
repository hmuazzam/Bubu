# Concepts and Originality

Bubu is implemented from general concepts rather than copied engine code or
parameter tables. Add a source note when introducing a new algorithmic idea.

| Area | Concept | Source / reference | Bubu implementation |
|------|---------|--------------------|---------------------|
| Scaffold | Makefile and compiler CI | Requirements in PRD Sections 6 and 8.5 | Build and verification infrastructure. |
| Board representation | 64-bit piece sets with square lookup | [Chess Programming Wiki: Board Representation](https://www.chessprogramming.org/Board_Representation) | Twelve piece bitboards and a 64-entry mailbox are maintained together. |
| Sliding attacks | Relevant occupancy mask, multiplication hash, shifted table index | [Chess Programming Wiki: Magic Bitboards](https://www.chessprogramming.org/Magic_Bitboards) | `tools/gen_magics.cpp` finds multipliers with Bubu's fixed-seed xorshift PRNG. No engine multiplier table is imported. A fixed-size lookup table is initialized once. |
| Move-generation verification | Perft leaf-node counts on reference positions | [Chess Programming Wiki: Perft Results](https://www.chessprogramming.org/Perft_Results) | Six required FEN/depth/node cases are stored in `tests/perft.epd`. |
| Hashing | Deterministic Zobrist key and incremental XOR updates | General Zobrist hashing concept; fixed-seed SplitMix64 in `src/position.cpp` | Keys are generated locally from a Bubu-specific fixed seed; debug assertions compare incremental and recomputed hashes. |
| Search | Negamax, alpha-beta, PVS, iterative deepening, quiescence, transposition tables | [Search](https://www.chessprogramming.org/Search), [PVS](https://www.chessprogramming.org/Principal_Variation_Search), [Transposition Table](https://www.chessprogramming.org/Transposition_Table), [Quiescence Search](https://www.chessprogramming.org/Quiescence_Search) | Implemented from algorithm descriptions with Bubu-owned state and fixed-capacity move/PV storage. |
| Search ordering and pruning | MVV/LVA ordering, SEE, killers, history, NMP, LMR, futility | [Move Ordering](https://www.chessprogramming.org/Move_Ordering), [SEE](https://www.chessprogramming.org/Static_Exchange_Evaluation), [NMP](https://www.chessprogramming.org/Null_Move_Pruning), [LMR](https://www.chessprogramming.org/Late_Move_Reductions) | Each feature has a local implementation and compile-time disable flag where applicable. No engine source or tuning table was copied. Features remain unproven until match testing. |
| Classical evaluation | Material, formula-based square bonuses, pawn structure, mobility, king zone | [Evaluation](https://www.chessprogramming.org/Evaluation), [Tapered Eval](https://www.chessprogramming.org/Tapered_Eval) | Starting parameters are in `src/params.h`; no piece-square table is copied. |
| King safety | Castling readiness, king shelter, and exposed files as evaluation features | General king-safety concepts in [Chess Programming Wiki: King Safety](https://www.chessprogramming.org/King_Safety) | `KingSafetyV2` uses Bubu-owned middlegame parameters in `params::KingSafetyV2Params`. The feature is opt-in and awaits SPRT validation. |
