#include "bitboard.h"
#include "bench.h"
#include "eval.h"
#include "movegen.h"
#include "params.h"
#include "position.h"
#include "search.h"
#include "uci.h"

#include <fstream>
#include <array>
#include <iostream>
#include <sstream>
#include <string>

namespace {

using namespace bubu;

constexpr const char* start_fen =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

std::string join_arguments(int argc, char* argv[], int begin) {
    std::string text;
    for (int index = begin; index < argc; ++index) {
        if (!text.empty()) text += ' ';
        text += argv[index];
    }
    return text;
}

bool position_matches(const Position& left, const Position& right) {
    return left.fen() == right.fen() && left.hash == right.hash &&
           left.piece_bb == right.piece_bb && left.mailbox == right.mailbox &&
           left.history_count == right.history_count;
}

bool test_round_trips() {
    Position position;
    if (!position.set_fen(start_fen)) return false;
    std::uint64_t random = 0x42b0b0ULL;
    int checked = 0;
    for (int game_ply = 0; game_ply < 120 && checked < 50; ++game_ply) {
        const std::string serialized = position.fen();
        Position restored;
        if (!restored.set_fen(serialized) || restored.fen() != serialized ||
            restored.hash != position.hash)
            return false;
        ++checked;

        MoveList legal = generate_legal_moves(position);
        if (legal.count == 0) {
            if (!position.set_fen(start_fen)) return false;
            continue;
        }
        random ^= random << 13;
        random ^= random >> 7;
        random ^= random << 17;
        UndoState undo;
        if (!position.make_move(legal.moves[random % legal.count], undo)) return false;
    }

    const std::string invalid_fens[] = {
        "8/8/8/8/8/8/8/8 w - - 0 1",
        "8/8/8/8/8/8/8/7Kx w - - 0 1",
        "8/8/8/8/8/8/8/7K w - - 0 1",
        "8/8/8/8/8/8/8/4K2k x - - 0 1",
        "8/8/8/8/8/8/8/4K2k w KK - 0 1",
        "8/8/8/8/8/8/8/4K2k w - e4 0 1",
        "4k3/8/8/8/8/8/8/4K3 w K - 0 1",
        "4k3/8/8/4P3/8/8/8/4K3 w - e6 0 1"};
    for (const std::string& fen : invalid_fens) {
        Position invalid;
        if (invalid.set_fen(fen)) return false;
    }
    return checked == 50;
}

bool test_make_unmake() {
    const std::string positions[] = {
        start_fen,
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"};
    for (const std::string& fen : positions) {
        Position position;
        if (!position.set_fen(fen)) return false;
        const Position before = position;
        const MoveList moves = generate_legal_moves(position);
        for (int index = 0; index < moves.count; ++index) {
            UndoState undo;
            if (!position.make_move(moves.moves[index], undo)) return false;
            if (position.hash != position.recompute_hash()) return false;
            position.unmake_move(moves.moves[index], undo);
            if (!position_matches(position, before)) return false;
        }
    }
    return true;
}

bool test_king_safety_v2() {
    struct CastlePair {
        const char* label;
        const char* before;
        const char* after;
    };
    constexpr CastlePair castle_pairs[] = {
        {"white kingside, queens present",
         "r3k2r/ppp1pppp/1nbqbn2/8/8/2NQ1B2/PPPPPPPP/R3K2R w KQkq - 0 1",
         "r3k2r/ppp1pppp/1nbqbn2/8/8/2NQ1B2/PPPPPPPP/R4RK1 w kq - 0 1"},
        {"white queenside, queens present",
         "r3k2r/ppp1pppp/1nbqbn2/8/8/2NQ1B2/PPPPPPPP/R3K2R w KQkq - 0 1",
         "r3k2r/ppp1pppp/1nbqbn2/8/8/2NQ1B2/PPPPPPPP/2KR3R w kq - 0 1"},
        {"black kingside, queens present",
         "r3k2r/ppp1pppp/1nbqbn2/8/8/2NQ1B2/PPPPPPPP/R3K2R b KQkq - 0 1",
         "r4rk1/ppp1pppp/1nbqbn2/8/8/2NQ1B2/PPPPPPPP/R3K2R b KQ - 0 1"},
        {"black queenside, queens present",
         "r3k2r/ppp1pppp/1nbqbn2/8/8/2NQ1B2/PPPPPPPP/R3K2R b KQkq - 0 1",
         "2kr3r/ppp1pppp/1nbqbn2/8/8/2NQ1B2/PPPPPPPP/R3K2R b KQ - 0 1"},
        {"white kingside, no queens",
         "r3k2r/ppp1pppp/1nb1bn2/8/8/2N2B2/PPPPPPPP/R3K2R w KQkq - 0 1",
         "r3k2r/ppp1pppp/1nb1bn2/8/8/2N2B2/PPPPPPPP/R4RK1 w kq - 0 1"},
    };

    for (const CastlePair& pair : castle_pairs) {
        Position before;
        Position after;
        if (!before.set_fen(pair.before) || !after.set_fen(pair.after) ||
            before.side_to_move != after.side_to_move)
            return false;
        if (evaluate(before, false) != evaluate(before) ||
            evaluate(after, false) != evaluate(after))
            return false;
        const int before_score = evaluate(before, true);
        const int after_score = evaluate(after, true);
        const int margin = after_score - before_score;
        std::cout << "KingSafetyV2 castle test " << pair.label << ": "
                  << before_score << " -> " << after_score << " cp (margin "
                  << margin << ")\n";
        if (margin < params::KingSafetyV2Params::minimum_test_castled_margin_cp)
            return false;
    }

    struct ShieldPair {
        const char* label;
        const char* intact;
        const char* advanced;
    };
    constexpr ShieldPair shield_pairs[] = {
        {"kingside g-pawn advanced",
         "r3k2r/ppp1pppp/1nbqbn2/8/8/2NQ1B2/PPPPPPPP/R4RK1 w kq - 0 1",
         "r3k2r/ppp1pppp/1nbqbn2/6P1/8/2NQ1B2/PPPPPP1P/R4RK1 w kq - 0 1"},
        {"queenside b-pawn advanced",
         "r3k2r/ppp1pppp/1nbqbn2/8/8/2NQ1B2/PPPPPPPP/2KR3R w kq - 0 1",
         "r3k2r/ppp1pppp/1nbqbn2/1P6/8/2NQ1B2/P1PPPPPP/2KR3R w kq - 0 1"},
    };
    for (const ShieldPair& pair : shield_pairs) {
        Position intact;
        Position advanced;
        if (!intact.set_fen(pair.intact) || !advanced.set_fen(pair.advanced))
            return false;
        const int intact_score = evaluate(intact, true);
        const int advanced_score = evaluate(advanced, true);
        std::cout << "KingSafetyV2 shield test " << pair.label << ": "
                  << intact_score << " -> " << advanced_score << " cp\n";
        if (advanced_score >= intact_score) return false;
    }
    return true;
}

Bitboard reference_slider_attacks(Square square, Bitboard occupied, bool bishop) {
    constexpr int diagonal[4][2] = {{1, 1}, {-1, 1}, {1, -1}, {-1, -1}};
    constexpr int straight[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    const int (*directions)[2] = bishop ? diagonal : straight;
    Bitboard attacks = 0;
    for (int direction = 0; direction < 4; ++direction) {
        int file = file_of(square) + directions[direction][0];
        int rank = rank_of(square) + directions[direction][1];
        while (file >= 0 && file < 8 && rank >= 0 && rank < 8) {
            const Square target = make_square(file, rank);
            const Bitboard target_bb = square_bb(target);
            attacks |= target_bb;
            if ((occupied & target_bb) != 0) break;
            file += directions[direction][0];
            rank += directions[direction][1];
        }
    }
    return attacks;
}

Bitboard reference_relevant_mask(Square square, bool bishop) {
    constexpr int diagonal[4][2] = {{1, 1}, {-1, 1}, {1, -1}, {-1, -1}};
    constexpr int straight[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    const int (*directions)[2] = bishop ? diagonal : straight;
    Bitboard mask = 0;
    for (int direction = 0; direction < 4; ++direction) {
        int file = file_of(square) + directions[direction][0];
        int rank = rank_of(square) + directions[direction][1];
        while (file >= 0 && file < 8 && rank >= 0 && rank < 8) {
            const int next_file = file + directions[direction][0];
            const int next_rank = rank + directions[direction][1];
            if (next_file < 0 || next_file >= 8 || next_rank < 0 || next_rank >= 8)
                break;
            mask |= square_bb(make_square(file, rank));
            file = next_file;
            rank = next_rank;
        }
    }
    return mask;
}

bool test_magic_tables() {
    for (Square square = 0; square < 64; ++square) {
        for (bool bishop : {false, true}) {
            Bitboard mask = reference_relevant_mask(square, bishop);
            std::array<Square, 12> bits{};
            int bit_count = 0;
            while (mask != 0) {
                bits[bit_count++] = lsb(mask);
                pop_lsb(mask);
            }
            const unsigned subsets = 1U << bit_count;
            for (unsigned subset = 0; subset < subsets; ++subset) {
                Bitboard occupied = 0;
                for (int bit = 0; bit < bit_count; ++bit)
                    if ((subset & (1U << bit)) != 0)
                        occupied |= square_bb(bits[bit]);
                const Bitboard actual = bishop ? bishop_attacks(square, occupied)
                                               : rook_attacks(square, occupied);
                if (actual != reference_slider_attacks(square, occupied, bishop))
                    return false;
            }
        }
    }
    return true;
}

bool run_perft_suite() {
    std::ifstream suite("tests/perft.epd");
    if (!suite) {
        std::cerr << "cannot open tests/perft.epd\n";
        return false;
    }
    std::string line;
    int cases = 0;
    while (std::getline(suite, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream row(line);
        int depth;
        std::uint64_t expected;
        if (!(row >> depth >> expected)) return false;
        std::string fen;
        std::getline(row >> std::ws, fen);
        Position position;
        std::string error;
        if (!position.set_fen(fen, &error)) {
            std::cerr << "bad perft FEN: " << error << '\n';
            return false;
        }
        const auto start = std::chrono::steady_clock::now();
        const std::uint64_t actual = perft(position, depth);
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 std::chrono::steady_clock::now() - start)
                                 .count();
        std::cout << "perft depth " << depth << ": " << actual << " nodes ("
                  << elapsed << " ms)";
        if (actual != expected) {
            std::cout << " expected " << expected << " -- FAIL\n";
            return false;
        }
        std::cout << " -- ok\n";
        ++cases;
    }
    return cases == 6;
}

void print_board(const Position& position) {
    for (int rank = 7; rank >= 0; --rank) {
        std::cout << rank + 1 << ' ';
        for (int file = 0; file < 8; ++file)
            std::cout << piece_symbol(position.mailbox[make_square(file, rank)]) << ' ';
        std::cout << '\n';
    }
    std::cout << "  a b c d e f g h\n";
    std::cout << position.fen() << '\n';
}

bool insufficient_material(const Position& position) {
    const Bitboard pawns = position.piece_bb[piece_index(Color::white, PieceType::pawn)] |
                           position.piece_bb[piece_index(Color::black, PieceType::pawn)];
    const Bitboard heavy = position.piece_bb[piece_index(Color::white, PieceType::rook)] |
                           position.piece_bb[piece_index(Color::black, PieceType::rook)] |
                           position.piece_bb[piece_index(Color::white, PieceType::queen)] |
                           position.piece_bb[piece_index(Color::black, PieceType::queen)];
    if (pawns != 0 || heavy != 0) return false;
    const Bitboard knights = position.piece_bb[piece_index(Color::white, PieceType::knight)] |
                            position.piece_bb[piece_index(Color::black, PieceType::knight)];
    const Bitboard bishops = position.piece_bb[piece_index(Color::white, PieceType::bishop)] |
                            position.piece_bb[piece_index(Color::black, PieceType::bishop)];
    if (popcount(knights | bishops) <= 1) return true;
    const int white_knights = popcount(
        position.piece_bb[piece_index(Color::white, PieceType::knight)]);
    const int black_knights = popcount(
        position.piece_bb[piece_index(Color::black, PieceType::knight)]);
    if (bishops == 0 && ((white_knights == 2 && black_knights == 0) ||
                         (black_knights == 2 && white_knights == 0)))
        return true;
    if (knights != 0) return false;
    int bishop_color = -1;
    Bitboard remaining = bishops;
    while (remaining != 0) {
        const Square square = lsb(remaining);
        pop_lsb(remaining);
        const int color = (file_of(square) + rank_of(square)) & 1;
        if (bishop_color != -1 && bishop_color != color) return false;
        bishop_color = color;
    }
    return true;
}

bool parse_depth(const char* text, int& depth) {
    try {
        std::size_t consumed = 0;
        depth = std::stoi(text, &consumed);
        return consumed == std::string(text).size() && depth >= 0 && depth <= 8;
    } catch (...) {
        return false;
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    using namespace bubu;
    if (argc == 1) {
        UciEngine engine;
        return engine.run();
    }
    const std::string command = argv[1];
    if (command == "uci") {
        UciEngine engine;
        return engine.run();
    }
    if (command == "test") {
        if (!test_round_trips()) {
            std::cerr << "FEN round-trip or rejection test failed\n";
            return 1;
        }
        std::cout << "FEN tests: 50 round-trips and malformed-input checks -- ok\n";
        if (!test_make_unmake()) {
            std::cerr << "make/unmake test failed\n";
            return 1;
        }
        std::cout << "make/unmake and incremental-hash tests -- ok\n";
        if (!test_magic_tables()) {
            std::cerr << "generated magic attack table test failed\n";
            return 1;
        }
        std::cout << "magic attacks: every relevant occupancy for all 64 squares -- ok\n";
        if (!test_king_safety_v2()) {
            std::cerr << "KingSafetyV2 evaluation tests failed\n";
            return 1;
        }
        std::cout << "KingSafetyV2: five castling margins and two shield-pawn tests -- ok\n";
        if (!run_perft_suite()) return 1;
        return 0;
    }
    if (command == "perft" || command == "perft-divide") {
        const bool divide = command == "perft-divide" ||
                            (argc > 2 && std::string(argv[2]) == "divide");
        const int depth_index = command == "perft-divide" ? 2
                                : argc > 2 && std::string(argv[2]) == "divide" ? 3
                                                                               : 2;
        int depth = 0;
        if (argc <= depth_index || !parse_depth(argv[depth_index], depth)) {
            std::cerr << "usage: bubu perft [divide] <depth> [FEN]\n";
            return 2;
        }
        const int fen_index = depth_index + 1;
        Position position;
        std::string error;
        const std::string fen = argc > fen_index ? join_arguments(argc, argv, fen_index)
                                                  : start_fen;
        if (!position.set_fen(fen, &error)) {
            std::cerr << "invalid FEN: " << error << '\n';
            return 2;
        }
        if (!divide || depth == 0) {
            std::cout << "nodes " << perft(position, depth) << '\n';
            return 0;
        }
        std::uint64_t total = 0;
        const MoveList moves = generate_legal_moves(position);
        for (int index = 0; index < moves.count; ++index) {
            UndoState undo;
            const Move move = moves.moves[index];
            if (!position.make_move(move, undo)) continue;
            const std::uint64_t count = perft(position, depth - 1);
            position.unmake_move(move, undo);
            total += count;
            std::cout << move_to_uci(move) << ": " << count << '\n';
        }
        std::cout << "nodes " << total << '\n';
        return 0;
    }
    if (command == "d") {
        Position position;
        std::string error;
        if (argc < 3 || !position.set_fen(join_arguments(argc, argv, 2), &error)) {
            std::cerr << (argc < 3 ? "usage: bubu d <FEN>\n" : error + "\n");
            return 2;
        }
        print_board(position);
        return 0;
    }
    if (command == "bench") {
        int depth = 6;
        if (argc > 2 && !parse_depth(argv[2], depth)) {
            std::cerr << "usage: bubu bench [depth]\n";
            return 2;
        }
        Searcher searcher;
        const BenchResult result = run_bench(searcher, depth);
        std::cout << "bench depth " << depth << " nodes " << result.nodes
                  << " nps " << result.nps << " time " << result.time_ms << " ms\n";
        return 0;
    }
    if (command == "eval") {
        Position position;
        std::string error;
        const std::string fen = argc > 2 ? join_arguments(argc, argv, 2) : start_fen;
        if (!position.set_fen(fen, &error)) {
            std::cerr << "invalid FEN: " << error << '\n';
            return 2;
        }
        std::cout << "score cp " << evaluate(position) << '\n';
        return 0;
    }
    if (command == "outcome") {
        Position position;
        std::string error;
        if (argc < 3 || !position.set_fen(join_arguments(argc, argv, 2), &error)) {
            std::cerr << (argc < 3 ? "usage: bubu outcome <FEN>\n" : error + "\n");
            return 2;
        }
        if (position.halfmove_clock >= 100 || insufficient_material(position)) {
            std::cout << "1/2-1/2\n";
            return 0;
        }
        const MoveList moves = generate_legal_moves(position);
        if (moves.count == 0) {
            if (!in_check(position, position.side_to_move))
                std::cout << "1/2-1/2\n";
            else
                std::cout << (position.side_to_move == Color::white ? "0-1\n" : "1-0\n");
            return 0;
        }
        std::cout << "*\n";
        return 0;
    }
    std::cerr << "unknown command: " << command << '\n';
    return 2;
}
