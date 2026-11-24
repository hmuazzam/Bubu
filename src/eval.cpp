#include "eval.h"

#include "bitboard.h"
#include "movegen.h"
#include "params.h"

#include <algorithm>
#include <cstdlib>

namespace bubu {
namespace {

struct ScorePair {
    int mg = 0;
    int eg = 0;
    ScorePair& operator+=(ScorePair other) {
        mg += other.mg;
        eg += other.eg;
        return *this;
    }
};

int material_value(PieceType type, bool endgame) {
    switch (type) {
        case PieceType::pawn: return endgame ? params::pawn_eg : params::pawn_mg;
        case PieceType::knight: return endgame ? params::knight_eg : params::knight_mg;
        case PieceType::bishop: return endgame ? params::bishop_eg : params::bishop_mg;
        case PieceType::rook: return endgame ? params::rook_eg : params::rook_mg;
        case PieceType::queen: return endgame ? params::queen_eg : params::queen_mg;
        default: return 0;
    }
}

int center_distance(Square square) {
    return std::abs(2 * file_of(square) - 7) +
           std::abs(2 * rank_of(square) - 7);
}

int oriented_rank(Color color, Square square) {
    return color == Color::white ? rank_of(square) : 7 - rank_of(square);
}

ScorePair piece_square(PieceType type, Color color, Square square) {
    const int rank = oriented_rank(color, square);
    const int center = 14 - center_distance(square);
    switch (type) {
        case PieceType::pawn:
            return {rank * params::pawn_advance_mg + center * params::pawn_center_mg,
                    rank * params::pawn_advance_eg + center * params::pawn_center_eg};
        case PieceType::knight:
            return {center * params::knight_center_mg, center * params::knight_center_eg};
        case PieceType::bishop:
            return {center * params::bishop_center, center * params::bishop_center};
        case PieceType::rook:
            return {rank == 6 ? params::rook_seventh_pst : 0,
                    rank * params::rook_advance_eg};
        case PieceType::queen:
            return {center * params::queen_center_mg, center * params::queen_center_eg};
        case PieceType::king:
            return {-center * params::king_center_mg +
                        (rank < 2 ? params::king_home_bonus : 0),
                    center * params::king_center_eg};
        default: return {};
    }
}

bool has_pawn_on_file(const Position& position, Color color, int file) {
    Bitboard pawns = position.piece_bb[piece_index(color, PieceType::pawn)];
    while (pawns != 0) {
        const Square square = lsb(pawns);
        pop_lsb(pawns);
        if (file_of(square) == file) return true;
    }
    return false;
}

bool is_passed_pawn(const Position& position, Color color, Square square) {
    Bitboard enemies = position.piece_bb[piece_index(opposite(color), PieceType::pawn)];
    while (enemies != 0) {
        const Square enemy = lsb(enemies);
        pop_lsb(enemies);
        const int rank_delta = color == Color::white ? rank_of(enemy) - rank_of(square)
                                                      : rank_of(square) - rank_of(enemy);
        if (rank_delta > 0 && std::abs(file_of(enemy) - file_of(square)) <= 1)
            return false;
    }
    return true;
}

ScorePair pawn_structure(const Position& position, Color color) {
    ScorePair score;
    Bitboard pawns = position.piece_bb[piece_index(color, PieceType::pawn)];
    const Bitboard friendly = pawns;
    while (pawns != 0) {
        const Square square = lsb(pawns);
        pop_lsb(pawns);
        const int file = file_of(square);
        int same_file = 0;
        Bitboard file_pawns = friendly;
        while (file_pawns != 0) {
            const Square other = lsb(file_pawns);
            pop_lsb(file_pawns);
            if (file_of(other) == file) ++same_file;
        }
        if (same_file > 1) score += {-params::doubled_pawn, -params::doubled_pawn};
        const bool neighbor_left = file > 0 && has_pawn_on_file(position, color, file - 1);
        const bool neighbor_right = file < 7 && has_pawn_on_file(position, color, file + 1);
        if (!neighbor_left && !neighbor_right)
            score += {-params::isolated_pawn, -params::isolated_pawn};

        const int forward = color == Color::white ? 8 : -8;
        const Square front = square + forward;
        if (front >= 0 && front < 64 &&
            (pawn_attacks(opposite(color), front) &
             position.piece_bb[piece_index(opposite(color), PieceType::pawn)]) != 0 &&
            !neighbor_left && !neighbor_right)
            score += {-params::backward_pawn, -params::backward_pawn};

        if (is_passed_pawn(position, color, square)) {
            const int rank = oriented_rank(color, square);
            const int bonus = params::passed_pawn_base * rank * rank;
            score += {bonus / 2, bonus};
        }
        const Bitboard adjacent = pawn_attacks(opposite(color), square) & friendly;
        if (adjacent != 0)
            score += {params::connected_pawn, params::connected_pawn};
    }
    return score;
}

ScorePair evaluate_side(const Position& position, Color color, int& phase) {
    ScorePair score;
    for (int piece_type = 0; piece_type < 6; ++piece_type) {
        const PieceType type = static_cast<PieceType>(piece_type);
        Bitboard pieces = position.piece_bb[piece_index(color, type)];
        while (pieces != 0) {
            const Square square = lsb(pieces);
            pop_lsb(pieces);
            score.mg += material_value(type, false);
            score.eg += material_value(type, true);
            score += piece_square(type, color, square);

            Bitboard attacks = 0;
            if (type == PieceType::knight) attacks = knight_attacks(square);
            else if (type == PieceType::bishop) attacks = bishop_attacks(square, position.occupied);
            else if (type == PieceType::rook) attacks = rook_attacks(square, position.occupied);
            else if (type == PieceType::queen) attacks = queen_attacks(square, position.occupied);
            if (attacks != 0)
                score.mg += popcount(attacks & ~position.color_bb[color_index(color)]) *
                            params::mobility_unit;

            if (type == PieceType::knight) phase += params::phase_knight;
            if (type == PieceType::bishop) phase += params::phase_bishop;
            if (type == PieceType::rook) phase += params::phase_rook;
            if (type == PieceType::queen) phase += params::phase_queen;

            if (type == PieceType::rook) {
                const bool friendly_pawn = has_pawn_on_file(position, color, file_of(square));
                const bool enemy_pawn = has_pawn_on_file(position, opposite(color), file_of(square));
                if (!friendly_pawn && !enemy_pawn) score.mg += params::rook_open_file_mg;
                else if (!friendly_pawn) score.mg += params::rook_semi_open_file_mg;
                if (oriented_rank(color, square) == 6) score.mg += params::rook_seventh_mg;
            }
        }
    }

    if (popcount(position.piece_bb[piece_index(color, PieceType::bishop)]) >= 2)
        score += {params::bishop_pair_mg, params::bishop_pair_eg};
    score += pawn_structure(position, color);

    const Square king = position.king_square(color);
    if (king != no_square) {
        const int front_rank = rank_of(king) + (color == Color::white ? 1 : -1);
        if (front_rank >= 0 && front_rank < 8) {
            for (int file = std::max(0, file_of(king) - 1);
                 file <= std::min(7, file_of(king) + 1); ++file) {
                if (position.mailbox[make_square(file, front_rank)] ==
                    make_piece(color, PieceType::pawn))
                    score.mg += params::king_shield_unit;
            }
        }
        const Bitboard zone = king_attacks(king) | square_bb(king);
        Bitboard enemies = position.color_bb[color_index(opposite(color))];
        int attackers = 0;
        while (enemies != 0) {
            const Square source = lsb(enemies);
            pop_lsb(enemies);
            const Piece piece = position.mailbox[source];
            Bitboard attacks = 0;
            switch (type_of(piece)) {
                case PieceType::pawn: attacks = pawn_attacks(opposite(color), source); break;
                case PieceType::knight: attacks = knight_attacks(source); break;
                case PieceType::bishop: attacks = bishop_attacks(source, position.occupied); break;
                case PieceType::rook: attacks = rook_attacks(source, position.occupied); break;
                case PieceType::queen: attacks = queen_attacks(source, position.occupied); break;
                case PieceType::king: attacks = king_attacks(source); break;
                default: break;
            }
            if ((attacks & zone) != 0) ++attackers;
        }
        score.mg -= attackers * params::king_zone_attack_unit;
    }
    return score;
}

}  // namespace

int evaluate(const Position& position) {
    int phase = 0;
    const ScorePair white = evaluate_side(position, Color::white, phase);
    const ScorePair black = evaluate_side(position, Color::black, phase);
    phase = std::min(phase, params::max_phase);
    const int mg = white.mg - black.mg;
    const int eg = white.eg - black.eg;
    const int blended = (mg * phase + eg * (params::max_phase - phase)) /
                        params::max_phase;
    const int scaled = blended * params::eval_scale_percent / 100;
    return (position.side_to_move == Color::white ? scaled : -scaled) + params::tempo;
}

}  // namespace bubu
