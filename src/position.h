#pragma once

#include "types.h"

#include <array>
#include <cstdint>
#include <string>

namespace bubu {

constexpr std::uint8_t white_king_side = 1;
constexpr std::uint8_t white_queen_side = 2;
constexpr std::uint8_t black_king_side = 4;
constexpr std::uint8_t black_queen_side = 8;
constexpr int max_game_plies = 1024;

struct UndoState {
    Piece captured = Piece::none;
    Square captured_square = no_square;
    std::uint8_t castling = 0;
    Square en_passant = no_square;
    int halfmove_clock = 0;
    int fullmove_number = 1;
    std::uint64_t hash = 0;
    int history_count = 0;
};

struct Position {
    std::array<Bitboard, 12> piece_bb{};
    std::array<Piece, board_size> mailbox{};
    std::array<Bitboard, 2> color_bb{};
    Bitboard occupied = 0;
    Color side_to_move = Color::white;
    std::uint8_t castling = 0;
    Square en_passant = no_square;
    int halfmove_clock = 0;
    int fullmove_number = 1;
    std::uint64_t hash = 0;
    std::array<std::uint64_t, max_game_plies> history{};
    int history_count = 0;

    Position();
    bool set_fen(const std::string& fen, std::string* error = nullptr);
    std::string fen() const;
    void clear();
    void put_piece(Piece piece, Square square);
    void remove_piece(Square square);
    Square king_square(Color color) const;
    std::uint64_t recompute_hash() const;
    bool make_move(Move move, UndoState& undo);
    void unmake_move(Move move, const UndoState& undo);
    void make_null_move(UndoState& undo);
    void unmake_null_move(const UndoState& undo);
    bool is_repetition(int required_occurrences = 3) const;
};

}  // namespace bubu
