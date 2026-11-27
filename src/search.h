#pragma once

#include "position.h"

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <vector>

#ifndef BUBU_ENABLE_NMP
#define BUBU_ENABLE_NMP 1
#endif
#ifndef BUBU_ENABLE_RFP
#define BUBU_ENABLE_RFP 1
#endif
#ifndef BUBU_ENABLE_FUTILITY
#define BUBU_ENABLE_FUTILITY 1
#endif
#ifndef BUBU_ENABLE_LMP
#define BUBU_ENABLE_LMP 1
#endif
#ifndef BUBU_ENABLE_LMR
#define BUBU_ENABLE_LMR 1
#endif
#ifndef BUBU_ENABLE_IIR
#define BUBU_ENABLE_IIR 1
#endif
#ifndef BUBU_ENABLE_ASPIRATION
#define BUBU_ENABLE_ASPIRATION 1
#endif
#ifndef BUBU_ENABLE_CHECK_EXTENSION
#define BUBU_ENABLE_CHECK_EXTENSION 1
#endif
#ifndef BUBU_ENABLE_SEE_PRUNING
#define BUBU_ENABLE_SEE_PRUNING 1
#endif

namespace bubu {

struct SearchLimits {
    int depth = 64;
    std::uint64_t nodes = 0;
    int movetime_ms = 0;
    int white_time_ms = 0;
    int black_time_ms = 0;
    int white_increment_ms = 0;
    int black_increment_ms = 0;
    int moves_to_go = 0;
    int move_overhead_ms = 30;
    bool clock_time_supplied = false;
    bool infinite = false;
};

struct SearchInfo {
    int depth = 0;
    int selective_depth = 0;
    int score = 0;
    std::uint64_t nodes = 0;
    std::uint64_t time_ms = 0;
    int hashfull = 0;
    std::array<Move, 128> pv{};
    int pv_length = 0;
};

struct SearchResult {
    Move best_move{};
    int score = 0;
    int depth = 0;
    std::uint64_t nodes = 0;
};

class Searcher {
public:
    Searcher();
    void set_hash_mb(int megabytes);
    void set_king_safety_v2(bool enabled);
    void clear_hash();
    void stop();
    SearchResult search(Position root, const SearchLimits& limits,
                        const std::function<void(const SearchInfo&)>& report);

private:
    enum class Bound : std::uint8_t { exact, lower, upper };
    struct Entry {
        std::uint64_t key = 0;
        Move move{};
        int score = 0;
        int depth = -1;
        Bound bound = Bound::upper;
        std::uint8_t generation = 0;
    };
    using Bucket = std::array<Entry, 4>;
    static constexpr int max_ply = 128;
    static constexpr int mate_score = 30000;
    static constexpr int mate_threshold = 29000;

    std::vector<Bucket> table_;
    std::atomic<bool> stop_requested_{false};
    SearchLimits limits_{};
    std::uint8_t generation_ = 0;
    bool king_safety_v2_ = true;
    std::uint64_t nodes_ = 0;
    int selective_depth_ = 0;
    int soft_time_ms_ = 0;
    int hard_time_ms_ = 0;
    int time_check_interval_ = 256;
    bool has_hard_time_limit_ = false;
    bool quick_move_ = false;
    std::chrono::steady_clock::time_point start_time_{};
    std::array<std::array<Move, 2>, max_ply> killers_{};
    std::array<std::array<std::array<int, 64>, 64>, 2> history_{};
    std::array<std::array<Move, 64>, 12> counter_moves_{};
    std::array<std::array<Move, max_ply>, max_ply> pv_table_{};
    std::array<int, max_ply> pv_length_{};

    int negamax(Position& position, int depth, int alpha, int beta, int ply,
                bool pv_node, Move previous_move);
    int quiescence(Position& position, int alpha, int beta, int ply);
    bool should_stop();
    int hashfull() const;
    int score_to_tt(int score, int ply) const;
    int score_from_tt(int score, int ply) const;
    int score_move(const Position& position, Move move, Move tt_move,
                   Move previous_move, int ply) const;
    int static_exchange(const Position& position, Move move) const;
    void order_moves(Position& position, MoveList& moves, Move tt_move,
                     Move previous_move, int ply) const;
    bool is_draw(const Position& position) const;
    Entry* probe(std::uint64_t key);
    void store(std::uint64_t key, Move move, int score, int depth, Bound bound,
               int ply);
};

}  // namespace bubu
