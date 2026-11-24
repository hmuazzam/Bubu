#include "bitboard.h"

#include "magic_numbers.h"

namespace bubu {
namespace {

constexpr int knight_offsets[8][2] = {
    {1, 2}, {2, 1}, {2, -1}, {1, -2},
    {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}};
constexpr int king_offsets[8][2] = {
    {1, 0}, {1, 1}, {0, 1}, {-1, 1},
    {-1, 0}, {-1, -1}, {0, -1}, {1, -1}};
constexpr int bishop_dirs[4][2] = {{1, 1}, {-1, 1}, {1, -1}, {-1, -1}};
constexpr int rook_dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

Bitboard ray_attacks(Square square, Bitboard occupied, const int dirs[][2],
                     int count);

struct SliderMagic {
    Bitboard mask = 0;
    int shift = 0;
    Bitboard multiplier = 0;
};

Bitboard relevant_mask(Square square, const int dirs[][2]) {
    Bitboard mask = 0;
    for (int direction = 0; direction < 4; ++direction) {
        int file = file_of(square) + dirs[direction][0];
        int rank = rank_of(square) + dirs[direction][1];
        while (file >= 0 && file < 8 && rank >= 0 && rank < 8) {
            const int next_file = file + dirs[direction][0];
            const int next_rank = rank + dirs[direction][1];
            if (next_file < 0 || next_file >= 8 || next_rank < 0 || next_rank >= 8)
                break;
            mask |= square_bb(make_square(file, rank));
            file = next_file;
            rank = next_rank;
        }
    }
    return mask;
}

struct SlidingTables {
    std::array<SliderMagic, 64> bishop_info{};
    std::array<SliderMagic, 64> rook_info{};
    std::array<std::array<Bitboard, 4096>, 64> bishop{};
    std::array<std::array<Bitboard, 4096>, 64> rook{};

    SlidingTables() {
        for (Square square = 0; square < 64; ++square) {
            fill_square(square, true);
            fill_square(square, false);
        }
    }

    void fill_square(Square square, bool diagonal) {
        const auto& magics = diagonal ? bishop_magics : rook_magics;
        const auto& dirs = diagonal ? bishop_dirs : rook_dirs;
        SliderMagic& info = diagonal ? bishop_info[square] : rook_info[square];
        auto& attacks = diagonal ? bishop[square] : rook[square];
        info.mask = relevant_mask(square, dirs);
        info.shift = 64 - popcount(info.mask);
        info.multiplier = magics[square];

        const int blocker_bits = popcount(info.mask);
        const unsigned subset_count = 1U << blocker_bits;
        for (unsigned subset = 0; subset < subset_count; ++subset) {
            Bitboard occupancy = 0;
            Bitboard mask = info.mask;
            for (int bit = 0; mask != 0; ++bit) {
                const Square blocker_square = lsb(mask);
                pop_lsb(mask);
                if ((subset & (1U << bit)) != 0)
                    occupancy |= square_bb(blocker_square);
            }
            const std::size_t index =
                static_cast<std::size_t>((occupancy * info.multiplier) >> info.shift);
            attacks[index] = ray_attacks(
                square, occupancy, dirs, 4);
        }
    }
};

Bitboard leaper_attacks(Square square, const int offsets[][2], int count) {
    Bitboard attacks = 0;
    const int file = file_of(square);
    const int rank = rank_of(square);
    for (int index = 0; index < count; ++index) {
        const int target_file = file + offsets[index][0];
        const int target_rank = rank + offsets[index][1];
        if (target_file >= 0 && target_file < 8 && target_rank >= 0 &&
            target_rank < 8) {
            attacks |= square_bb(make_square(target_file, target_rank));
        }
    }
    return attacks;
}

Bitboard ray_attacks(Square square, Bitboard occupied, const int dirs[][2],
                     int count) {
    Bitboard attacks = 0;
    for (int index = 0; index < count; ++index) {
        int file = file_of(square) + dirs[index][0];
        int rank = rank_of(square) + dirs[index][1];
        while (file >= 0 && file < 8 && rank >= 0 && rank < 8) {
            const Square target = make_square(file, rank);
            const Bitboard target_bb = square_bb(target);
            attacks |= target_bb;
            if ((occupied & target_bb) != 0) break;
            file += dirs[index][0];
            rank += dirs[index][1];
        }
    }
    return attacks;
}

// Build once before main so hot-path lookups do not check a local-static guard.
const SlidingTables slider_tables{};

struct LeaperTables {
    std::array<Bitboard, 64> knight{};
    std::array<Bitboard, 64> king{};
    std::array<std::array<Bitboard, 64>, 2> pawn{};

    LeaperTables() {
        for (Square square = 0; square < 64; ++square) {
            knight[square] = leaper_attacks(square, knight_offsets, 8);
            king[square] = leaper_attacks(square, king_offsets, 8);
            for (Color color : {Color::white, Color::black}) {
                Bitboard attacks = 0;
                const int next_rank =
                    rank_of(square) + (color == Color::white ? 1 : -1);
                if (next_rank >= 0 && next_rank < 8) {
                    if (file_of(square) > 0)
                        attacks |= square_bb(make_square(file_of(square) - 1,
                                                        next_rank));
                    if (file_of(square) < 7)
                        attacks |= square_bb(make_square(file_of(square) + 1,
                                                        next_rank));
                }
                pawn[color == Color::white ? 0 : 1][square] = attacks;
            }
        }
    }
};

const LeaperTables leaper_tables{};

}  // namespace

Bitboard pawn_attacks(Color color, Square square) {
    return leaper_tables.pawn[color == Color::white ? 0 : 1][square];
}

Bitboard knight_attacks(Square square) {
    return leaper_tables.knight[square];
}

Bitboard king_attacks(Square square) {
    return leaper_tables.king[square];
}

Bitboard bishop_attacks(Square square, Bitboard occupied) {
    const SlidingTables& tables = slider_tables;
    const SliderMagic& magic = tables.bishop_info[square];
    const std::size_t index = static_cast<std::size_t>(
        (((occupied & magic.mask) * magic.multiplier) >> magic.shift));
    return tables.bishop[square][index];
}

Bitboard rook_attacks(Square square, Bitboard occupied) {
    const SlidingTables& tables = slider_tables;
    const SliderMagic& magic = tables.rook_info[square];
    const std::size_t index = static_cast<std::size_t>(
        (((occupied & magic.mask) * magic.multiplier) >> magic.shift));
    return tables.rook[square][index];
}

Bitboard queen_attacks(Square square, Bitboard occupied) {
    return bishop_attacks(square, occupied) | rook_attacks(square, occupied);
}

}  // namespace bubu
