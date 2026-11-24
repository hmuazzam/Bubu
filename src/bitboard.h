#pragma once

#include "types.h"

namespace bubu {

inline int popcount(Bitboard board) {
    return __builtin_popcountll(static_cast<unsigned long long>(board));
}

inline Square lsb(Bitboard board) {
    return board == 0 ? no_square
                      : static_cast<Square>(__builtin_ctzll(
                            static_cast<unsigned long long>(board)));
}

inline Bitboard pop_lsb(Bitboard& board) {
    const Bitboard bit = board & (~board + 1);
    board &= board - 1;
    return bit;
}
Bitboard pawn_attacks(Color color, Square square);
Bitboard knight_attacks(Square square);
Bitboard king_attacks(Square square);
Bitboard bishop_attacks(Square square, Bitboard occupied);
Bitboard rook_attacks(Square square, Bitboard occupied);
Bitboard queen_attacks(Square square, Bitboard occupied);

}  // namespace bubu
