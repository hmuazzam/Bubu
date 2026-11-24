#include "position.h"

#include "bitboard.h"

#include <cassert>
#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <sstream>
#include <string_view>

namespace bubu {
namespace {

struct ZobristKeys {
    std::array<std::array<std::uint64_t, 64>, 12> pieces{};
    std::array<std::uint64_t, 16> castling{};
    std::array<std::uint64_t, 8> en_passant_file{};
    std::uint64_t side = 0;
};

std::uint64_t splitmix64(std::uint64_t& state) {
    std::uint64_t value = (state += 0x9e3779b97f4a7c15ULL);
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}

const ZobristKeys& zobrist() {
    // Fixed-seed keys make hashes reproducible across builds and machines.
    static const ZobristKeys keys = [] {
        ZobristKeys result{};
        std::uint64_t seed = 0x4255425543484553ULL;
        for (auto& piece : result.pieces)
            for (auto& square : piece) square = splitmix64(seed);
        for (auto& key : result.castling) key = splitmix64(seed);
        for (auto& key : result.en_passant_file) key = splitmix64(seed);
        result.side = splitmix64(seed);
        return result;
    }();
    return keys;
}

bool parse_nonnegative(const std::string& text, int& value) {
    const char* begin = text.data();
    const char* end = begin + text.size();
    const auto result = std::from_chars(begin, end, value);
    return result.ec == std::errc{} && result.ptr == end && value >= 0;
}

void set_error(std::string* error, const char* message) {
    if (error != nullptr) *error = message;
}

void xor_piece(std::uint64_t& key, Piece piece, Square square) {
    key ^= zobrist().pieces[static_cast<int>(piece)][square];
}

void xor_en_passant(std::uint64_t& key, Square square) {
    if (square != no_square) key ^= zobrist().en_passant_file[file_of(square)];
}

}  // namespace

std::string square_name(Square square) {
    if (square < 0 || square >= 64) return "-";
    std::string name(2, ' ');
    name[0] = static_cast<char>('a' + file_of(square));
    name[1] = static_cast<char>('1' + rank_of(square));
    return name;
}

bool parse_square(const std::string& text, Square& square) {
    if (text.size() != 2 || text[0] < 'a' || text[0] > 'h' || text[1] < '1' ||
        text[1] > '8')
        return false;
    square = make_square(text[0] - 'a', text[1] - '1');
    return true;
}

char piece_symbol(Piece piece) {
    constexpr char symbols[] = "PNBRQKpnbrqk";
    return is_piece(piece) ? symbols[static_cast<int>(piece)] : '.';
}

Piece parse_piece(char symbol) {
    constexpr std::string_view symbols = "PNBRQKpnbrqk";
    const auto index = symbols.find(symbol);
    return index == std::string_view::npos ? Piece::none
                                           : static_cast<Piece>(index);
}

Position::Position() { clear(); }

void Position::clear() {
    piece_bb.fill(0);
    color_bb.fill(0);
    occupied = 0;
    mailbox.fill(Piece::none);
    side_to_move = Color::white;
    castling = 0;
    en_passant = no_square;
    halfmove_clock = 0;
    fullmove_number = 1;
    hash = 0;
    history.fill(0);
    history_count = 0;
}

void Position::put_piece(Piece piece, Square square) {
    assert(is_piece(piece) && square >= 0 && square < 64);
    const Bitboard bit = square_bb(square);
    piece_bb[static_cast<int>(piece)] |= bit;
    color_bb[color_index(color_of(piece))] |= bit;
    occupied |= bit;
    mailbox[square] = piece;
}

void Position::remove_piece(Square square) {
    assert(square >= 0 && square < 64 && is_piece(mailbox[square]));
    const Piece piece = mailbox[square];
    const Bitboard bit = square_bb(square);
    piece_bb[static_cast<int>(piece)] &= ~bit;
    color_bb[color_index(color_of(piece))] &= ~bit;
    occupied &= ~bit;
    mailbox[square] = Piece::none;
}

Square Position::king_square(Color color) const {
    return lsb(piece_bb[piece_index(color, PieceType::king)]);
}

std::uint64_t Position::recompute_hash() const {
    std::uint64_t key = 0;
    for (Square square = 0; square < 64; ++square) {
        const Piece piece = mailbox[square];
        if (is_piece(piece)) xor_piece(key, piece, square);
    }
    key ^= zobrist().castling[castling];
    xor_en_passant(key, en_passant);
    if (side_to_move == Color::black) key ^= zobrist().side;
    return key;
}

bool Position::set_fen(const std::string& text, std::string* error) {
    std::istringstream input(text);
    std::string placement, side, rights, ep, halfmove, fullmove, extra;
    if (!(input >> placement >> side >> rights >> ep)) {
        set_error(error, "FEN requires placement, side, castling, and en-passant fields");
        return false;
    }
    if (!(input >> halfmove)) halfmove = "0";
    if (!(input >> fullmove)) fullmove = "1";
    if (input >> extra) {
        set_error(error, "FEN has more than six fields");
        return false;
    }

    Position parsed;
    int rank = 7;
    int file = 0;
    int white_kings = 0;
    int black_kings = 0;
    for (char symbol : placement) {
        if (symbol == '/') {
            if (file != 8 || rank == 0) {
                set_error(error, "invalid rank width in FEN placement");
                return false;
            }
            --rank;
            file = 0;
            continue;
        }
        if (symbol >= '1' && symbol <= '8') {
            file += symbol - '0';
            if (file > 8) {
                set_error(error, "FEN rank contains too many squares");
                return false;
            }
            continue;
        }
        const Piece piece = parse_piece(symbol);
        if (!is_piece(piece) || file >= 8 || rank < 0) {
            set_error(error, "invalid piece or square in FEN placement");
            return false;
        }
        parsed.put_piece(piece, make_square(file++, rank));
        if (piece == Piece::white_king) ++white_kings;
        if (piece == Piece::black_king) ++black_kings;
    }
    if (rank != 0 || file != 8 || white_kings != 1 || black_kings != 1) {
        set_error(error, "FEN placement must contain eight ranks and one king per side");
        return false;
    }

    if (side == "w") parsed.side_to_move = Color::white;
    else if (side == "b") parsed.side_to_move = Color::black;
    else {
        set_error(error, "invalid active color in FEN");
        return false;
    }

    if (rights != "-") {
        for (char right : rights) {
            std::uint8_t bit = 0;
            switch (right) {
                case 'K': bit = white_king_side; break;
                case 'Q': bit = white_queen_side; break;
                case 'k': bit = black_king_side; break;
                case 'q': bit = black_queen_side; break;
                default:
                    set_error(error, "invalid castling rights in FEN");
                    return false;
            }
            if ((parsed.castling & bit) != 0) {
                set_error(error, "duplicate castling right in FEN");
                return false;
            }
            parsed.castling |= bit;
        }
    }

    if (((parsed.castling & white_king_side) != 0 &&
         (parsed.mailbox[4] != Piece::white_king || parsed.mailbox[7] != Piece::white_rook)) ||
        ((parsed.castling & white_queen_side) != 0 &&
         (parsed.mailbox[4] != Piece::white_king || parsed.mailbox[0] != Piece::white_rook)) ||
        ((parsed.castling & black_king_side) != 0 &&
         (parsed.mailbox[60] != Piece::black_king || parsed.mailbox[63] != Piece::black_rook)) ||
        ((parsed.castling & black_queen_side) != 0 &&
         (parsed.mailbox[60] != Piece::black_king || parsed.mailbox[56] != Piece::black_rook))) {
        set_error(error, "castling rights require the king and rook on their home squares");
        return false;
    }

    if (ep != "-") {
        if (!parse_square(ep, parsed.en_passant) ||
            rank_of(parsed.en_passant) != (parsed.side_to_move == Color::white ? 5 : 2) ||
            parsed.mailbox[parsed.en_passant] != Piece::none) {
            set_error(error, "invalid en-passant target in FEN");
            return false;
        }
        const Square pawn_square = parsed.en_passant +
                                   (parsed.side_to_move == Color::white ? -8 : 8);
        const Piece expected_pawn = make_piece(opposite(parsed.side_to_move), PieceType::pawn);
        if (parsed.mailbox[pawn_square] != expected_pawn) {
            set_error(error, "en-passant target has no pawn to capture in FEN");
            return false;
        }
    }

    if (!parse_nonnegative(halfmove, parsed.halfmove_clock) ||
        !parse_nonnegative(fullmove, parsed.fullmove_number) ||
        parsed.fullmove_number < 1) {
        set_error(error, "invalid move counters in FEN");
        return false;
    }

    parsed.hash = parsed.recompute_hash();
    parsed.history[0] = parsed.hash;
    parsed.history_count = 1;
    *this = parsed;
    if (error != nullptr) error->clear();
    return true;
}

std::string Position::fen() const {
    std::ostringstream output;
    for (int rank = 7; rank >= 0; --rank) {
        int empty = 0;
        for (int file = 0; file < 8; ++file) {
            const Piece piece = mailbox[make_square(file, rank)];
            if (!is_piece(piece)) {
                ++empty;
                continue;
            }
            if (empty != 0) output << empty;
            empty = 0;
            output << piece_symbol(piece);
        }
        if (empty != 0) output << empty;
        if (rank != 0) output << '/';
    }
    output << (side_to_move == Color::white ? " w " : " b ");
    if (castling == 0) output << '-';
    else {
        if ((castling & white_king_side) != 0) output << 'K';
        if ((castling & white_queen_side) != 0) output << 'Q';
        if ((castling & black_king_side) != 0) output << 'k';
        if ((castling & black_queen_side) != 0) output << 'q';
    }
    output << ' ' << (en_passant == no_square ? "-" : square_name(en_passant));
    output << ' ' << halfmove_clock << ' ' << fullmove_number;
    return output.str();
}

bool Position::make_move(Move move, UndoState& undo) {
    if (move.from < 0 || move.from >= 64 || move.to < 0 || move.to >= 64 ||
        !is_piece(mailbox[move.from]))
        return false;

    const Piece moving = mailbox[move.from];
    const Color us = side_to_move;
    if (color_of(moving) != us) return false;
    const PieceType moving_type = type_of(moving);
    if (is_piece(mailbox[move.to]) && color_of(mailbox[move.to]) == us) return false;
    const bool reaches_back_rank = moving_type == PieceType::pawn &&
                                   (rank_of(move.to) == 0 || rank_of(move.to) == 7);
    if ((reaches_back_rank &&
         (move.promotion == PieceType::none || move.promotion == PieceType::pawn ||
          move.promotion == PieceType::king)) ||
        (!reaches_back_rank && move.promotion != PieceType::none))
        return false;
    if (moving_type == PieceType::king &&
        std::abs(file_of(move.to) - file_of(move.from)) == 2) {
        const Square rook_from = move.to > move.from ? move.from + 3 : move.from - 4;
        if (move.from != (us == Color::white ? 4 : 60) || rook_from < 0 ||
            rook_from >= 64 || mailbox[rook_from] != make_piece(us, PieceType::rook))
            return false;
    }
    Square captured_square = move.to;
    if (moving_type == PieceType::pawn && move.to == en_passant &&
        mailbox[move.to] == Piece::none)
        captured_square += us == Color::white ? -8 : 8;
    const Piece captured = mailbox[captured_square];

    undo = {captured, captured == Piece::none ? no_square : captured_square,
            castling, en_passant, halfmove_clock, fullmove_number, hash,
            history_count};

    hash ^= zobrist().castling[castling];
    xor_en_passant(hash, en_passant);
    xor_piece(hash, moving, move.from);
    remove_piece(move.from);
    if (is_piece(captured)) {
        xor_piece(hash, captured, captured_square);
        remove_piece(captured_square);
    }

    Piece placed = moving;
    if (reaches_back_rank) {
        placed = make_piece(us, move.promotion);
    }
    put_piece(placed, move.to);
    xor_piece(hash, placed, move.to);

    // Moving a king two files is the compact internal representation of castling.
    if (moving_type == PieceType::king && std::abs(file_of(move.to) - file_of(move.from)) == 2) {
        const Square rook_from = move.to > move.from ? move.from + 3 : move.from - 4;
        const Square rook_to = move.to > move.from ? move.from + 1 : move.from - 1;
        const Piece rook = make_piece(us, PieceType::rook);
        xor_piece(hash, rook, rook_from);
        remove_piece(rook_from);
        put_piece(rook, rook_to);
        xor_piece(hash, rook, rook_to);
    }

    if (moving_type == PieceType::king) {
        castling &= us == Color::white
                        ? static_cast<std::uint8_t>(~(white_king_side | white_queen_side))
                        : static_cast<std::uint8_t>(~(black_king_side | black_queen_side));
    }
    if (move.from == 0 || move.to == 0) castling &= ~white_queen_side;
    if (move.from == 7 || move.to == 7) castling &= ~white_king_side;
    if (move.from == 56 || move.to == 56) castling &= ~black_queen_side;
    if (move.from == 63 || move.to == 63) castling &= ~black_king_side;

    en_passant = no_square;
    if (moving_type == PieceType::pawn && std::abs(move.to - move.from) == 16)
        en_passant = (move.from + move.to) / 2;
    halfmove_clock = moving_type == PieceType::pawn || is_piece(captured)
                         ? 0
                         : halfmove_clock + 1;
    if (us == Color::black) ++fullmove_number;
    side_to_move = opposite(us);
    hash ^= zobrist().castling[castling];
    xor_en_passant(hash, en_passant);
    hash ^= zobrist().side;
    if (history_count < max_game_plies) history[history_count++] = hash;

#ifndef NDEBUG
    assert(hash == recompute_hash());
    assert((piece_bb[0] | piece_bb[1] | piece_bb[2] | piece_bb[3] |
            piece_bb[4] | piece_bb[5] | piece_bb[6] | piece_bb[7] |
            piece_bb[8] | piece_bb[9] | piece_bb[10] | piece_bb[11]) == occupied);
#endif
    return true;
}

void Position::unmake_move(Move move, const UndoState& undo) {
    side_to_move = opposite(side_to_move);
    const Color us = side_to_move;
    Piece placed = mailbox[move.to];
    const PieceType moving_type = (rank_of(move.to) == 0 || rank_of(move.to) == 7) &&
                                          type_of(placed) != PieceType::pawn &&
                                          move.promotion != PieceType::none
                                      ? PieceType::pawn
                                      : type_of(placed);
    const Piece moving = make_piece(us, moving_type);
    remove_piece(move.to);
    put_piece(moving, move.from);

    if (moving_type == PieceType::king && std::abs(file_of(move.to) - file_of(move.from)) == 2) {
        const Square rook_from = move.to > move.from ? move.from + 3 : move.from - 4;
        const Square rook_to = move.to > move.from ? move.from + 1 : move.from - 1;
        const Piece rook = make_piece(us, PieceType::rook);
        remove_piece(rook_to);
        put_piece(rook, rook_from);
    }
    if (is_piece(undo.captured)) put_piece(undo.captured, undo.captured_square);

    castling = undo.castling;
    en_passant = undo.en_passant;
    halfmove_clock = undo.halfmove_clock;
    fullmove_number = undo.fullmove_number;
    hash = undo.hash;
    history_count = undo.history_count;
}

void Position::make_null_move(UndoState& undo) {
    undo = {Piece::none, no_square, castling, en_passant, halfmove_clock,
            fullmove_number, hash, history_count};
    xor_en_passant(hash, en_passant);
    en_passant = no_square;
    side_to_move = opposite(side_to_move);
    hash ^= zobrist().side;
#ifndef NDEBUG
    assert(hash == recompute_hash());
#endif
}

void Position::unmake_null_move(const UndoState& undo) {
    side_to_move = opposite(side_to_move);
    castling = undo.castling;
    en_passant = undo.en_passant;
    halfmove_clock = undo.halfmove_clock;
    fullmove_number = undo.fullmove_number;
    hash = undo.hash;
    history_count = undo.history_count;
#ifndef NDEBUG
    assert(hash == recompute_hash());
#endif
}

bool Position::is_repetition(int required_occurrences) const {
    int seen = 0;
    const int first = std::max(0, history_count - 1 - halfmove_clock);
    for (int index = history_count - 1; index >= first; index -= 2) {
        if (history[index] == hash && ++seen >= required_occurrences) return true;
    }
    return false;
}

}  // namespace bubu
