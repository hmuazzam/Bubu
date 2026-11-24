# Sumber Konsep dan Orisinalitas

Implementasi engine akan ditulis dari konsep umum, bukan dengan menyalin kode
atau tabel dari engine lain. Tambahkan catatan sumber ketika konsep algoritmik
baru diterapkan.

| Area | Concept | Source / reference | Bubu implementation |
|------|---------|--------------------|---------------------|
| Scaffold | Makefile and compiler CI | Requirements in PRD sections 6 and 8.5 | Infrastructure only. |
| Board representation | 64-bit piece sets alongside square lookup | [Chess Programming Wiki: Board Representation](https://www.chessprogramming.org/Board_Representation) | Twelve piece bitboards and a 64-entry mailbox are maintained together. |
| Sliding attacks | Relevant occupancy mask, multiplication hash, shifted table index | [Chess Programming Wiki: Magic Bitboards](https://www.chessprogramming.org/Magic_Bitboards) | `tools/gen_magics.cpp` finds multipliers using Bubu's fixed-seed xorshift PRNG; no engine's multiplier table is imported. A fixed-size lookup table is initialized once. |
| Move-generation verification | Perft leaf-node counts on reference positions | [Chess Programming Wiki: Perft Results](https://www.chessprogramming.org/Perft_Results) | The six required FEN/depth/node cases are kept in `tests/perft.epd`. |
| Hashing | Deterministic Zobrist key and incremental XOR updates | General Zobrist hashing concept; fixed-seed SplitMix64 in `src/position.cpp` | Keys are generated locally from a Bubu-specific fixed seed; debug assertions compare incremental and recomputed hashes. |
| Search | Negamax, alpha-beta, PVS, iterative deepening, quiescence, transposition tables | [Chess Programming Wiki: Search](https://www.chessprogramming.org/Search), [PVS](https://www.chessprogramming.org/Principal_Variation_Search), [Transposition Table](https://www.chessprogramming.org/Transposition_Table), [Quiescence Search](https://www.chessprogramming.org/Quiescence_Search) | Implemented from the algorithm descriptions with Bubu-owned state and fixed-capacity move/PV storage. |
| Search ordering and pruning | MVV/LVA ordering, SEE, killers, history, NMP, LMR, futility | [Move Ordering](https://www.chessprogramming.org/Move_Ordering), [Static Exchange Evaluation](https://www.chessprogramming.org/Static_Exchange_Evaluation), [Null Move Pruning](https://www.chessprogramming.org/Null_Move_Pruning), [Late Move Reductions](https://www.chessprogramming.org/Late_Move_Reductions) | Each feature has a local implementation and compile-time disable flag where applicable. No engine source or tuning table was copied. Features remain unproven until match testing. |
| Classical evaluation | Material, formula-based square bonuses, pawn structure, mobility, king zone | [Chess Programming Wiki: Evaluation](https://www.chessprogramming.org/Evaluation), [Tapered Eval](https://www.chessprogramming.org/Tapered_Eval) | Starting parameters are in `src/params.h`; no piece-square table is copied. |
