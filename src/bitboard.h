#pragma once

#include "types.h"

namespace bubu {

int popcount(Bitboard board);
Square lsb(Bitboard board);
Bitboard pop_lsb(Bitboard& board);
Bitboard pawn_attacks(Color color, Square square);
Bitboard knight_attacks(Square square);
Bitboard king_attacks(Square square);
Bitboard bishop_attacks(Square square, Bitboard occupied);
Bitboard rook_attacks(Square square, Bitboard occupied);
Bitboard queen_attacks(Square square, Bitboard occupied);

}  // namespace bubu
