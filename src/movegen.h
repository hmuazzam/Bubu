#pragma once

#include "position.h"

namespace bubu {

enum class MoveMode { all, captures, quiets };

bool square_attacked(const Position& position, Square square, Color by_color);
bool in_check(const Position& position, Color color);
MoveList generate_legal_moves(Position& position, MoveMode mode = MoveMode::all);
std::uint64_t perft(Position& position, int depth);
std::string move_to_uci(Move move);
bool parse_uci_move(Position& position, const std::string& text, Move& move);

}  // namespace bubu
