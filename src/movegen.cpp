#include "movegen.h"

#include "bitboard.h"

#include <cmath>

namespace bubu {
namespace {

void add_promotion_moves(MoveList& moves, Square from, Square to) {
    moves.push({from, to, PieceType::queen});
    moves.push({from, to, PieceType::rook});
    moves.push({from, to, PieceType::bishop});
    moves.push({from, to, PieceType::knight});
}

void generate_pseudo_legal(const Position& position, MoveList& moves,
                           MoveMode mode) {
    const Color us = position.side_to_move;
    const Color them = opposite(us);
    const bool checked = in_check(position, us);
    const bool captures_only = mode == MoveMode::captures && !checked;
    const bool quiets_only = mode == MoveMode::quiets && !checked;

    Bitboard pawns = position.piece_bb[piece_index(us, PieceType::pawn)];
    const int forward = us == Color::white ? 8 : -8;
    const int start_rank = us == Color::white ? 1 : 6;
    const int promotion_rank = us == Color::white ? 7 : 0;
    while (pawns != 0) {
        const Square from = lsb(pawns);
        pop_lsb(pawns);
        const Square one = from + forward;
        if (one >= 0 && one < 64 &&
            position.mailbox[one] == Piece::none) {
            if (rank_of(one) == promotion_rank) {
                if (!quiets_only) add_promotion_moves(moves, from, one);
            } else if (!quiets_only) {
                if (!captures_only) moves.push({from, one, PieceType::none});
            }
            const Square two = one + forward;
            if (rank_of(from) == start_rank && two >= 0 && two < 64 &&
                position.mailbox[two] == Piece::none && !captures_only && !quiets_only)
                moves.push({from, two, PieceType::none});
        }

        Bitboard targets = pawn_attacks(us, from) & position.color_bb[color_index(them)];
        if (position.en_passant != no_square &&
            (pawn_attacks(us, from) & square_bb(position.en_passant)) != 0)
            targets |= square_bb(position.en_passant);
        while (targets != 0) {
            const Square to = lsb(targets);
            pop_lsb(targets);
            if (rank_of(to) == promotion_rank) add_promotion_moves(moves, from, to);
            else if (!quiets_only) moves.push({from, to, PieceType::none});
        }
    }

    const PieceType types[] = {PieceType::knight, PieceType::bishop,
                               PieceType::rook, PieceType::queen,
                               PieceType::king};
    for (PieceType type : types) {
        Bitboard pieces = position.piece_bb[piece_index(us, type)];
        while (pieces != 0) {
            const Square from = lsb(pieces);
            pop_lsb(pieces);
            Bitboard targets = 0;
            switch (type) {
                case PieceType::knight: targets = knight_attacks(from); break;
                case PieceType::bishop: targets = bishop_attacks(from, position.occupied); break;
                case PieceType::rook: targets = rook_attacks(from, position.occupied); break;
                case PieceType::queen: targets = queen_attacks(from, position.occupied); break;
                case PieceType::king: targets = king_attacks(from); break;
                default: break;
            }
            targets &= ~position.color_bb[color_index(us)];
            if (captures_only) targets &= position.color_bb[color_index(them)];
            if (quiets_only) targets &= ~position.color_bb[color_index(them)];
            while (targets != 0) {
                const Square to = lsb(targets);
                pop_lsb(targets);
                moves.push({from, to, PieceType::none});
            }
        }
    }

    if (captures_only || quiets_only || checked) return;
    const Square king = us == Color::white ? 4 : 60;
    if (position.mailbox[king] != make_piece(us, PieceType::king) ||
        square_attacked(position, king, them))
        return;
    if (us == Color::white) {
        if ((position.castling & white_king_side) != 0 &&
            position.mailbox[5] == Piece::none && position.mailbox[6] == Piece::none &&
            position.mailbox[7] == Piece::white_rook &&
            !square_attacked(position, 5, them) && !square_attacked(position, 6, them))
            moves.push({4, 6, PieceType::none});
        if ((position.castling & white_queen_side) != 0 &&
            position.mailbox[1] == Piece::none && position.mailbox[2] == Piece::none &&
            position.mailbox[3] == Piece::none && position.mailbox[0] == Piece::white_rook &&
            !square_attacked(position, 3, them) && !square_attacked(position, 2, them))
            moves.push({4, 2, PieceType::none});
    } else {
        if ((position.castling & black_king_side) != 0 &&
            position.mailbox[61] == Piece::none && position.mailbox[62] == Piece::none &&
            position.mailbox[63] == Piece::black_rook &&
            !square_attacked(position, 61, them) && !square_attacked(position, 62, them))
            moves.push({60, 62, PieceType::none});
        if ((position.castling & black_queen_side) != 0 &&
            position.mailbox[57] == Piece::none && position.mailbox[58] == Piece::none &&
            position.mailbox[59] == Piece::none && position.mailbox[56] == Piece::black_rook &&
            !square_attacked(position, 59, them) && !square_attacked(position, 58, them))
            moves.push({60, 58, PieceType::none});
    }
}

}  // namespace

bool square_attacked(const Position& position, Square square, Color by_color) {
    if (square < 0 || square >= 64) return false;
    const Bitboard pawns = position.piece_bb[piece_index(by_color, PieceType::pawn)];
    if ((pawn_attacks(opposite(by_color), square) & pawns) != 0) return true;
    if ((knight_attacks(square) &
         position.piece_bb[piece_index(by_color, PieceType::knight)]) != 0)
        return true;
    if ((king_attacks(square) &
         position.piece_bb[piece_index(by_color, PieceType::king)]) != 0)
        return true;
    const Bitboard diagonals = position.piece_bb[piece_index(by_color, PieceType::bishop)] |
                               position.piece_bb[piece_index(by_color, PieceType::queen)];
    if ((bishop_attacks(square, position.occupied) & diagonals) != 0) return true;
    const Bitboard straights = position.piece_bb[piece_index(by_color, PieceType::rook)] |
                               position.piece_bb[piece_index(by_color, PieceType::queen)];
    return (rook_attacks(square, position.occupied) & straights) != 0;
}

bool in_check(const Position& position, Color color) {
    return square_attacked(position, position.king_square(color), opposite(color));
}

MoveList generate_legal_moves(Position& position, MoveMode mode) {
    MoveList candidates;
    generate_pseudo_legal(position, candidates, mode);
    MoveList legal;
    const Color mover = position.side_to_move;
    for (int index = 0; index < candidates.count; ++index) {
        const Move move = candidates.moves[index];
        UndoState undo;
        if (!position.make_move(move, undo)) continue;
        const bool safe = !in_check(position, mover);
        position.unmake_move(move, undo);
        if (safe) legal.push(move);
    }
    return legal;
}

std::uint64_t perft(Position& position, int depth) {
    if (depth <= 0) return 1;
    const MoveList moves = generate_legal_moves(position);
    if (depth == 1) return static_cast<std::uint64_t>(moves.count);
    std::uint64_t nodes = 0;
    for (int index = 0; index < moves.count; ++index) {
        UndoState undo;
        const Move move = moves.moves[index];
        if (position.make_move(move, undo)) {
            nodes += perft(position, depth - 1);
            position.unmake_move(move, undo);
        }
    }
    return nodes;
}

std::string move_to_uci(Move move) {
    std::string text = square_name(move.from) + square_name(move.to);
    if (move.promotion != PieceType::none) {
        constexpr char promotions[] = "nbrq";
        switch (move.promotion) {
            case PieceType::knight: text += promotions[0]; break;
            case PieceType::bishop: text += promotions[1]; break;
            case PieceType::rook: text += promotions[2]; break;
            case PieceType::queen: text += promotions[3]; break;
            default: break;
        }
    }
    return text;
}

bool parse_uci_move(Position& position, const std::string& text, Move& move) {
    if (text.size() != 4 && text.size() != 5) return false;
    Square from, to;
    if (!parse_square(text.substr(0, 2), from) ||
        !parse_square(text.substr(2, 2), to))
        return false;
    PieceType promotion = PieceType::none;
    if (text.size() == 5) {
        switch (text[4]) {
            case 'n': promotion = PieceType::knight; break;
            case 'b': promotion = PieceType::bishop; break;
            case 'r': promotion = PieceType::rook; break;
            case 'q': promotion = PieceType::queen; break;
            default: return false;
        }
    }
    const MoveList legal = generate_legal_moves(position);
    for (int index = 0; index < legal.count; ++index) {
        if (legal.moves[index] == Move{from, to, promotion}) {
            move = legal.moves[index];
            return true;
        }
    }
    return false;
}

}  // namespace bubu
