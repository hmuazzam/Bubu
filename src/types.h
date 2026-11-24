#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace bubu {

using Bitboard = std::uint64_t;
using Square = int;

constexpr Square no_square = -1;
constexpr int board_size = 64;

enum class Color : std::uint8_t { white = 0, black = 1 };
enum class PieceType : std::uint8_t {
    pawn = 0,
    knight,
    bishop,
    rook,
    queen,
    king,
    none
};
enum class Piece : std::uint8_t {
    white_pawn = 0,
    white_knight,
    white_bishop,
    white_rook,
    white_queen,
    white_king,
    black_pawn,
    black_knight,
    black_bishop,
    black_rook,
    black_queen,
    black_king,
    none
};

constexpr int file_of(Square square) { return square & 7; }
constexpr int rank_of(Square square) { return square >> 3; }
constexpr Square make_square(int file, int rank) { return rank * 8 + file; }
constexpr Bitboard square_bb(Square square) { return Bitboard{1} << square; }
constexpr Color opposite(Color color) {
    return color == Color::white ? Color::black : Color::white;
}
constexpr int color_index(Color color) { return static_cast<int>(color); }
constexpr int piece_index(Color color, PieceType type) {
    return color_index(color) * 6 + static_cast<int>(type);
}
constexpr Piece make_piece(Color color, PieceType type) {
    return static_cast<Piece>(piece_index(color, type));
}
constexpr Color color_of(Piece piece) {
    return static_cast<int>(piece) < 6 ? Color::white : Color::black;
}
constexpr PieceType type_of(Piece piece) {
    return static_cast<PieceType>(static_cast<int>(piece) % 6);
}
constexpr bool is_piece(Piece piece) { return piece != Piece::none; }

struct Move {
    Square from = no_square;
    Square to = no_square;
    PieceType promotion = PieceType::none;

    friend constexpr bool operator==(Move left, Move right) {
        return left.from == right.from && left.to == right.to &&
               left.promotion == right.promotion;
    }
};

struct MoveList {
    std::array<Move, 256> moves{};
    int count = 0;

    void push(Move move) {
        if (count < static_cast<int>(moves.size())) moves[count++] = move;
    }
};

std::string square_name(Square square);
bool parse_square(const std::string& text, Square& square);
char piece_symbol(Piece piece);
Piece parse_piece(char symbol);

}  // namespace bubu
