#include "uci.h"

#include "bench.h"
#include "eval.h"
#include "movegen.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <sstream>
#include <string_view>
#include <vector>

namespace bubu {
namespace {

constexpr const char* initial_fen =
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

std::vector<std::string> words(const std::string& line) {
    std::istringstream input(line);
    std::vector<std::string> result;
    std::string word;
    while (input >> word) result.push_back(word);
    return result;
}

int to_int(const std::string& text, int fallback = 0) {
    try {
        std::size_t consumed = 0;
        const int value = std::stoi(text, &consumed);
        return consumed == text.size() ? value : fallback;
    } catch (...) {
        return fallback;
    }
}

std::string join(const std::vector<std::string>& tokens, std::size_t begin,
                 std::size_t end) {
    std::string result;
    for (std::size_t index = begin; index < end; ++index) {
        if (!result.empty()) result += ' ';
        result += tokens[index];
    }
    return result;
}

}  // namespace

UciEngine::UciEngine() { position_.set_fen(initial_fen); }

UciEngine::~UciEngine() { stop_and_join(); }

void UciEngine::write_line(const std::string& line) {
    std::lock_guard<std::mutex> lock(output_mutex_);
    std::cout << line << std::endl;
}

void UciEngine::stop_and_join() {
    if (worker_.joinable()) {
        searcher_.stop();
        worker_.join();
    }
    searching_.store(false, std::memory_order_relaxed);
}

void UciEngine::handle_position(const std::string& command) {
    stop_and_join();
    const std::vector<std::string> tokens = words(command);
    if (tokens.size() < 2) {
        write_line("info string position requires startpos or fen");
        return;
    }
    Position next;
    std::size_t moves_start = tokens.size();
    if (tokens[1] == "startpos") {
        if (!next.set_fen(initial_fen)) return;
        if (tokens.size() > 2 && tokens[2] == "moves") moves_start = 3;
    } else if (tokens[1] == "fen") {
        std::size_t fen_end = tokens.size();
        for (std::size_t index = 2; index < tokens.size(); ++index) {
            if (tokens[index] == "moves") {
                fen_end = index;
                moves_start = index + 1;
                break;
            }
        }
        const std::string fen_text = join(tokens, 2, fen_end);
        std::string error;
        if (!next.set_fen(fen_text, &error)) {
            write_line("info string invalid FEN: " + error);
            return;
        }
    } else {
        write_line("info string position expects startpos or fen");
        return;
    }

    if (moves_start < tokens.size()) {
        for (std::size_t index = moves_start; index < tokens.size(); ++index) {
            Move move;
            if (!parse_uci_move(next, tokens[index], move)) {
                write_line("info string illegal move in position: " + tokens[index]);
                return;
            }
            UndoState undo;
            if (!next.make_move(move, undo)) return;
        }
    }
    position_ = next;
}

void UciEngine::handle_go(const std::string& command) {
    stop_and_join();
    const std::vector<std::string> tokens = words(command);
    SearchLimits limits;
    for (std::size_t index = 1; index < tokens.size(); ++index) {
        const std::string& token = tokens[index];
        if (token == "infinite") limits.infinite = true;
        else if (token == "depth" && index + 1 < tokens.size())
            limits.depth = std::clamp(to_int(tokens[++index], 64), 1, 126);
        else if (token == "nodes" && index + 1 < tokens.size())
            limits.nodes = static_cast<std::uint64_t>(std::max(0, to_int(tokens[++index])));
        else if (token == "movetime" && index + 1 < tokens.size())
            limits.movetime_ms = std::max(0, to_int(tokens[++index]));
        else if (token == "wtime" && index + 1 < tokens.size()) {
            limits.white_time_ms = std::max(0, to_int(tokens[++index]));
            limits.clock_time_supplied = true;
        } else if (token == "btime" && index + 1 < tokens.size()) {
            limits.black_time_ms = std::max(0, to_int(tokens[++index]));
            limits.clock_time_supplied = true;
        }
        else if (token == "winc" && index + 1 < tokens.size())
            limits.white_increment_ms = std::max(0, to_int(tokens[++index]));
        else if (token == "binc" && index + 1 < tokens.size())
            limits.black_increment_ms = std::max(0, to_int(tokens[++index]));
        else if (token == "movestogo" && index + 1 < tokens.size())
            limits.moves_to_go = std::max(0, to_int(tokens[++index]));
    }

    limits.move_overhead_ms = move_overhead_ms_;
    const Position root = position_;
    searching_.store(true, std::memory_order_relaxed);
    worker_ = std::thread([this, root, limits] () mutable {
        const SearchResult result = searcher_.search(
            root, limits, [this](const SearchInfo& info) { print_info(info); });
        std::string best = "bestmove ";
        if (result.best_move.from == no_square) best += "0000";
        else best += move_to_uci(result.best_move);
        write_line(best);
        searching_.store(false, std::memory_order_relaxed);
    });
}

void UciEngine::handle_setoption(const std::string& command) {
    stop_and_join();
    const std::vector<std::string> tokens = words(command);
    std::size_t name_start = 0;
    std::size_t value_start = tokens.size();
    for (std::size_t index = 1; index < tokens.size(); ++index) {
        if (tokens[index] == "name") name_start = index + 1;
        else if (tokens[index] == "value") {
            value_start = index + 1;
            break;
        }
    }
    const std::size_t name_end = value_start == tokens.size() ? tokens.size()
                                                               : value_start - 1;
    const std::string name = join(tokens, name_start, name_end);
    if (name == "Clear Hash") {
        searcher_.clear_hash();
    } else if (name == "Hash") {
        const int value = value_start < tokens.size() ? to_int(tokens[value_start], hash_mb_)
                                                       : hash_mb_;
        hash_mb_ = std::clamp(value, 1, 1024);
        searcher_.set_hash_mb(hash_mb_);
    } else if (name == "KingSafetyV2") {
        const bool enabled = value_start < tokens.size() &&
                             tokens[value_start] == "true";
        if (enabled != king_safety_v2_) {
            king_safety_v2_ = enabled;
            searcher_.set_king_safety_v2(king_safety_v2_);
            // TT scores use the active evaluator, so do not reuse entries from
            // a different KingSafetyV2 setting.
            searcher_.clear_hash();
        }
    } else if (name == "Move Overhead") {
        const int value = value_start < tokens.size()
                              ? to_int(tokens[value_start], move_overhead_ms_)
                              : move_overhead_ms_;
        move_overhead_ms_ = std::clamp(value, 0, 5000);
    } else if (name == "Threads") {
        write_line("info string Threads is fixed at 1");
    }
}

void UciEngine::print_info(const SearchInfo& info) {
    std::ostringstream output;
    output << "info depth " << info.depth << " seldepth " << info.selective_depth
           << " score ";
    if (std::abs(info.score) > 29000) {
        const int plies = 30000 - std::abs(info.score);
        const int mate = (plies + 1) / 2;
        output << "mate " << (info.score < 0 ? -mate : mate);
    } else {
        output << "cp " << info.score;
    }
    const std::uint64_t nps = info.nodes * 1000 / std::max<std::uint64_t>(1, info.time_ms);
    output << " nodes " << info.nodes << " nps " << nps << " time " << info.time_ms
           << " hashfull " << info.hashfull << " pv";
    for (int index = 0; index < info.pv_length; ++index)
        output << ' ' << move_to_uci(info.pv[index]);
    write_line(output.str());
}

int UciEngine::run() {
    std::string command;
    while (std::getline(std::cin, command)) {
        if (command == "uci") {
            write_line("id name Bubu");
            write_line("id author Bubu contributors");
            write_line("option name Hash type spin default 16 min 1 max 1024");
            write_line("option name Threads type spin default 1 min 1 max 1");
            write_line("option name Clear Hash type button");
            write_line("option name KingSafetyV2 type check default true");
            write_line("option name Move Overhead type spin default 30 min 0 max 5000");
            write_line("uciok");
        } else if (command == "isready") {
            write_line("readyok");
        } else if (command == "ucinewgame") {
            stop_and_join();
            position_.set_fen(initial_fen);
            searcher_.clear_hash();
        } else if (command.rfind("position", 0) == 0) {
            handle_position(command);
        } else if (command.rfind("setoption", 0) == 0) {
            handle_setoption(command);
        } else if (command.rfind("go", 0) == 0) {
            handle_go(command);
        } else if (command == "stop") {
            stop_and_join();
        } else if (command == "quit") {
            stop_and_join();
            return 0;
        } else if (command == "d") {
            for (int rank = 7; rank >= 0; --rank) {
                std::ostringstream row;
                row << rank + 1 << ' ';
                for (int file = 0; file < 8; ++file)
                    row << piece_symbol(position_.mailbox[make_square(file, rank)]) << ' ';
                write_line(row.str());
            }
            write_line("  a b c d e f g h");
            write_line(position_.fen());
        } else if (command == "eval") {
            write_line("info score cp " +
                       std::to_string(evaluate(position_, king_safety_v2_)));
        } else if (command.rfind("bench", 0) == 0) {
            stop_and_join();
            const std::vector<std::string> tokens = words(command);
            const int depth = tokens.size() > 1
                                  ? std::clamp(to_int(tokens[1], 6), 1, 12)
                                  : 6;
            const BenchResult result = run_bench(searcher_, depth);
            write_line("info string bench depth " + std::to_string(depth) +
                       " nodes " + std::to_string(result.nodes) + " nps " +
                       std::to_string(result.nps) + " time " +
                       std::to_string(result.time_ms) + " ms");
        }
    }
    stop_and_join();
    return 0;
}

}  // namespace bubu
