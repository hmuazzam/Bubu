#include "search.h"

#include "bitboard.h"
#include "eval.h"
#include "movegen.h"
#include "params.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace bubu {
namespace {

int exchange_value(PieceType type) {
    switch (type) {
        case PieceType::pawn: return params::pawn_mg;
        case PieceType::knight: return params::knight_mg;
        case PieceType::bishop: return params::bishop_mg;
        case PieceType::rook: return params::rook_mg;
        case PieceType::queen: return params::queen_mg;
        default: return 20000;
    }
}

#if BUBU_ENABLE_NMP
bool has_non_pawn_material(const Position& position, Color color) {
    return (position.piece_bb[piece_index(color, PieceType::knight)] |
            position.piece_bb[piece_index(color, PieceType::bishop)] |
            position.piece_bb[piece_index(color, PieceType::rook)] |
            position.piece_bb[piece_index(color, PieceType::queen)]) != 0;
}
#endif

Bitboard attackers_to(const Position& position, Square target, Color side,
                      Bitboard occupied) {
    Bitboard attackers = pawn_attacks(opposite(side), target) &
                         position.piece_bb[piece_index(side, PieceType::pawn)];
    attackers |= knight_attacks(target) &
                 position.piece_bb[piece_index(side, PieceType::knight)];
    attackers |= king_attacks(target) &
                 position.piece_bb[piece_index(side, PieceType::king)];
    attackers |= bishop_attacks(target, occupied) &
                 (position.piece_bb[piece_index(side, PieceType::bishop)] |
                  position.piece_bb[piece_index(side, PieceType::queen)]);
    attackers |= rook_attacks(target, occupied) &
                 (position.piece_bb[piece_index(side, PieceType::rook)] |
                  position.piece_bb[piece_index(side, PieceType::queen)]);
    return attackers & occupied;
}

bool is_capture(const Position& position, Move move) {
    return is_piece(position.mailbox[move.to]) ||
           (type_of(position.mailbox[move.from]) == PieceType::pawn &&
            move.to == position.en_passant);
}

}  // namespace

Searcher::Searcher() { set_hash_mb(16); }

void Searcher::set_hash_mb(int megabytes) {
    megabytes = std::clamp(megabytes, 1, 1024);
    const std::size_t bytes = static_cast<std::size_t>(megabytes) * 1024 * 1024;
    const std::size_t bucket_count = std::max<std::size_t>(1, bytes / sizeof(Bucket));
    table_.assign(bucket_count, Bucket{});
    generation_ = 0;
}

void Searcher::set_king_safety_v2(bool enabled) {
    king_safety_v2_ = enabled;
}

void Searcher::clear_hash() {
    for (Bucket& bucket : table_)
        for (Entry& entry : bucket) entry = Entry{};
    generation_ = 0;
}

void Searcher::stop() { stop_requested_.store(true, std::memory_order_relaxed); }

Searcher::Entry* Searcher::probe(std::uint64_t key) {
    if (table_.empty()) return nullptr;
    Bucket& bucket = table_[key % table_.size()];
    for (Entry& entry : bucket)
        if (entry.depth >= 0 && entry.key == key) return &entry;
    return nullptr;
}

void Searcher::store(std::uint64_t key, Move move, int score, int depth,
                     Bound bound, int ply) {
    if (table_.empty()) return;
    Bucket& bucket = table_[key % table_.size()];
    Entry* replace = &bucket[0];
    for (Entry& entry : bucket) {
        if (entry.key == key || entry.depth < 0) {
            replace = &entry;
            break;
        }
        const int current_quality = entry.depth - 4 * (entry.generation != generation_);
        const int replace_quality = replace->depth -
                                    4 * (replace->generation != generation_);
        if (current_quality < replace_quality) replace = &entry;
    }
    *replace = {key, move, score_to_tt(score, ply), depth, bound, generation_};
}

int Searcher::score_to_tt(int score, int ply) const {
    if (score > mate_threshold) return score + ply;
    if (score < -mate_threshold) return score - ply;
    return score;
}

int Searcher::score_from_tt(int score, int ply) const {
    if (score > mate_threshold) return score - ply;
    if (score < -mate_threshold) return score + ply;
    return score;
}

int Searcher::hashfull() const {
    if (table_.empty()) return 0;
    const std::size_t count = std::min<std::size_t>(1000, table_.size());
    std::size_t used = 0;
    for (std::size_t bucket = 0; bucket < count; ++bucket)
        for (const Entry& entry : table_[bucket])
            used += entry.depth >= 0 && entry.generation == generation_;
    const std::size_t slots = count * 4;
    return slots == 0 ? 0 : static_cast<int>(used * 1000 / slots);
}

bool Searcher::should_stop() {
    if (stop_requested_.load(std::memory_order_relaxed)) return true;
    if (limits_.nodes != 0 && nodes_ >= limits_.nodes) return true;
    if (limits_.infinite || !has_hard_time_limit_ || current_depth_ <= 2 ||
        nodes_ % static_cast<std::uint64_t>(time_check_interval_) != 0)
        return false;
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start_time_)
                             .count();
    if (elapsed >= hard_time_ms_) {
        stop_requested_.store(true, std::memory_order_relaxed);
        return true;
    }
    return false;
}

bool Searcher::is_draw(const Position& position) const {
    if (position.halfmove_clock >= 100 || position.is_repetition(3)) return true;
    const Bitboard heavy = position.piece_bb[piece_index(Color::white, PieceType::pawn)] |
                           position.piece_bb[piece_index(Color::black, PieceType::pawn)] |
                           position.piece_bb[piece_index(Color::white, PieceType::rook)] |
                           position.piece_bb[piece_index(Color::black, PieceType::rook)] |
                           position.piece_bb[piece_index(Color::white, PieceType::queen)] |
                           position.piece_bb[piece_index(Color::black, PieceType::queen)];
    if (heavy != 0) return false;
    const int minors = popcount(position.piece_bb[piece_index(Color::white, PieceType::knight)] |
                                position.piece_bb[piece_index(Color::white, PieceType::bishop)] |
                                position.piece_bb[piece_index(Color::black, PieceType::knight)] |
                                position.piece_bb[piece_index(Color::black, PieceType::bishop)]);
    if (minors <= 1) return true;
    const Bitboard knights = position.piece_bb[piece_index(Color::white, PieceType::knight)] |
                            position.piece_bb[piece_index(Color::black, PieceType::knight)];
    if (knights != 0) {
        const int white_knights = popcount(
            position.piece_bb[piece_index(Color::white, PieceType::knight)]);
        const int black_knights = popcount(
            position.piece_bb[piece_index(Color::black, PieceType::knight)]);
        const bool no_bishops =
            (position.piece_bb[piece_index(Color::white, PieceType::bishop)] |
             position.piece_bb[piece_index(Color::black, PieceType::bishop)]) == 0;
        if (no_bishops && ((white_knights == 2 && black_knights == 0) ||
                           (black_knights == 2 && white_knights == 0)))
            return true;
        return false;
    }
    Bitboard bishops = position.piece_bb[piece_index(Color::white, PieceType::bishop)] |
                       position.piece_bb[piece_index(Color::black, PieceType::bishop)];
    int square_color = -1;
    while (bishops != 0) {
        const Square square = lsb(bishops);
        pop_lsb(bishops);
        const int color = (file_of(square) + rank_of(square)) & 1;
        if (square_color != -1 && color != square_color) return false;
        square_color = color;
    }
    return true;
}

int Searcher::static_exchange(const Position& position, Move move) const {
    const Piece moving = position.mailbox[move.from];
    const Piece victim = position.mailbox[move.to];
    if (!is_piece(victim) && move.to != position.en_passant) return 0;
    const Square captured_square = move.to == position.en_passant && !is_piece(victim)
                                       ? move.to + (position.side_to_move == Color::white ? -8 : 8)
                                       : move.to;
    const Piece captured = position.mailbox[captured_square];
    std::array<int, 32> gains{};
    gains[0] = is_piece(captured) ? exchange_value(type_of(captured)) : 0;
    PieceType occupying_type = move.promotion == PieceType::none
                                   ? type_of(moving)
                                   : move.promotion;
    if (move.promotion != PieceType::none)
        gains[0] += exchange_value(move.promotion) - exchange_value(PieceType::pawn);
    Bitboard occupied = position.occupied & ~square_bb(move.from) &
                        ~square_bb(captured_square);
    occupied |= square_bb(move.to);
    Color side = opposite(position.side_to_move);
    int depth = 0;
    while (depth < 30) {
        Bitboard attackers = attackers_to(position, move.to, side, occupied);
        if (attackers == 0) break;
        PieceType attacker_type = PieceType::king;
        Square attacker_square = no_square;
        for (int type = static_cast<int>(PieceType::pawn);
             type <= static_cast<int>(PieceType::king); ++type) {
            const Bitboard candidates = attackers &
                position.piece_bb[piece_index(side, static_cast<PieceType>(type))];
            if (candidates != 0) {
                attacker_type = static_cast<PieceType>(type);
                attacker_square = lsb(candidates);
                break;
            }
        }
        if (attacker_square == no_square) break;
        ++depth;
        gains[depth] = exchange_value(occupying_type) - gains[depth - 1];
        occupying_type = attacker_type;
        occupied &= ~square_bb(attacker_square);
        side = opposite(side);
    }
    while (depth > 0) {
        gains[depth - 1] = -std::max(-gains[depth - 1], gains[depth]);
        --depth;
    }
    return gains[0];
}

int Searcher::score_move(const Position& position, Move move, Move tt_move,
                         Move previous_move, int ply) const {
    if (move == tt_move) return 2'000'000;
    if (is_capture(position, move)) {
        const Piece victim = position.mailbox[move.to];
        const int victim_value = is_piece(victim)
                                     ? exchange_value(type_of(victim))
                                     : params::pawn_mg;
        const int mover_value = exchange_value(type_of(position.mailbox[move.from]));
        const int exchange = static_exchange(position, move);
        return (exchange >= 0 ? 1'000'000 : 100'000) +
               victim_value * 16 - mover_value + exchange;
    }
    if (ply < max_ply) {
        if (move == killers_[ply][0]) return 900'000;
        if (move == killers_[ply][1]) return 890'000;
    }
    if (previous_move.from != no_square && previous_move.to >= 0 &&
        previous_move.to < 64) {
        const Piece previous_piece = position.mailbox[previous_move.to];
        if (is_piece(previous_piece) &&
            move == counter_moves_[static_cast<int>(previous_piece)][previous_move.to])
            return 880'000;
    }
    return history_[color_index(position.side_to_move)][move.from][move.to];
}

void Searcher::order_moves(Position& position, MoveList& moves, Move tt_move,
                           Move previous_move, int ply) const {
    std::array<int, 256> scores{};
    for (int index = 0; index < moves.count; ++index)
        scores[index] = score_move(position, moves.moves[index], tt_move,
                                   previous_move, ply);
    for (int index = 0; index < moves.count; ++index) {
        int best = index;
        for (int candidate = index + 1; candidate < moves.count; ++candidate)
            if (scores[candidate] > scores[best]) best = candidate;
        std::swap(scores[index], scores[best]);
        std::swap(moves.moves[index], moves.moves[best]);
    }
}

int Searcher::quiescence(Position& position, int alpha, int beta, int ply) {
    if (should_stop()) return 0;
    ++nodes_;
    selective_depth_ = std::max(selective_depth_, ply);
    if (is_draw(position)) return 0;
    if (ply >= max_ply - 1) return evaluate(position, king_safety_v2_);
    const bool checked = in_check(position, position.side_to_move);
    if (!checked) {
        const int stand_pat = evaluate(position, king_safety_v2_);
        if (stand_pat >= beta) return beta;
        if (stand_pat > alpha) alpha = stand_pat;
    }

    MoveList moves = generate_legal_moves(position,
                                          checked ? MoveMode::all : MoveMode::captures);
    if (checked && moves.count == 0) return -mate_score + ply;
    order_moves(position, moves, Move{}, Move{}, ply);
    for (int index = 0; index < moves.count; ++index) {
        const Move move = moves.moves[index];
#if BUBU_ENABLE_SEE_PRUNING
        if (!checked && move.promotion == PieceType::none &&
            is_capture(position, move) && static_exchange(position, move) < 0)
            continue;
#endif
        UndoState undo;
        if (!position.make_move(move, undo)) continue;
        const int score = -quiescence(position, -beta, -alpha, ply + 1);
        position.unmake_move(move, undo);
        if (stop_requested_.load(std::memory_order_relaxed)) return 0;
        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }
    return alpha;
}

int Searcher::negamax(Position& position, int depth, int alpha, int beta,
                      int ply, bool pv_node, Move previous_move) {
    if (should_stop()) return 0;
    ++nodes_;
    selective_depth_ = std::max(selective_depth_, ply);
    pv_length_[ply] = ply;
    if (ply >= max_ply - 1) return evaluate(position, king_safety_v2_);
    if (is_draw(position)) return 0;

    const bool checked = in_check(position, position.side_to_move);
#if BUBU_ENABLE_CHECK_EXTENSION
    if (checked) ++depth;
#endif
    if (depth <= 0) return quiescence(position, alpha, beta, ply);

    alpha = std::max(alpha, -mate_score + ply);
    beta = std::min(beta, mate_score - ply - 1);
    if (alpha >= beta) return alpha;
    const int original_alpha = alpha;
    Entry* tt_entry = probe(position.hash);
    Move tt_move{};
    if (tt_entry != nullptr) {
        tt_move = tt_entry->move;
        const int tt_score = score_from_tt(tt_entry->score, ply);
        if (!pv_node && tt_entry->depth >= depth) {
            if (tt_entry->bound == Bound::exact ||
                (tt_entry->bound == Bound::lower && tt_score >= beta) ||
                (tt_entry->bound == Bound::upper && tt_score <= alpha))
                return tt_score;
        }
    }

#if BUBU_ENABLE_RFP || BUBU_ENABLE_NMP || BUBU_ENABLE_FUTILITY
    const int static_eval = evaluate(position, king_safety_v2_);
#endif
#if BUBU_ENABLE_RFP
    if (!pv_node && !checked && depth <= params::rfp_max_depth &&
        static_eval - (params::rfp_margin_per_ply * depth) >= beta &&
        std::abs(beta) < mate_threshold)
        return static_eval;
#endif
#if BUBU_ENABLE_NMP
    if (!pv_node && !checked && depth >= params::nmp_min_depth &&
        beta < mate_threshold &&
        has_non_pawn_material(position, position.side_to_move)) {
        const int reduction = params::nmp_base_reduction +
                              depth / params::nmp_depth_divisor +
                              std::clamp((static_eval - beta) /
                                             params::nmp_eval_margin_scale,
                                         0, params::nmp_max_extra_reduction);
        UndoState null_undo;
        position.make_null_move(null_undo);
        const int null_score = -negamax(position, depth - 1 - reduction,
                                        -beta, -beta + 1, ply + 1, false, Move{});
        position.unmake_null_move(null_undo);
        if (stop_requested_.load(std::memory_order_relaxed)) return 0;
        if (null_score >= beta) return null_score;
    }
#endif
#if BUBU_ENABLE_IIR
    if (tt_move.from == no_square && depth >= params::iir_min_depth) --depth;
#endif

    MoveList moves = generate_legal_moves(position);
    if (moves.count == 0) return checked ? -mate_score + ply : 0;
    order_moves(position, moves, tt_move, previous_move, ply);
    Move best_move{};
    int best_score = -mate_score;
    int move_number = 0;
    for (int index = 0; index < moves.count; ++index) {
        const Move move = moves.moves[index];
        const bool capture = is_capture(position, move);
        const bool quiet = !capture && move.promotion == PieceType::none;
        const Piece moved_piece = position.mailbox[move.from];
#if BUBU_ENABLE_LMP
        if (!pv_node && !checked && depth <= params::lmp_max_depth && quiet &&
            move_number >= params::lmp_base_moves + depth * depth)
            continue;
#endif
#if BUBU_ENABLE_FUTILITY
        if (!pv_node && !checked && depth <= params::futility_max_depth &&
            move_number > 0 && quiet &&
            static_eval + params::futility_margin_per_ply * depth <= alpha)
            continue;
#endif
        UndoState undo;
        if (!position.make_move(move, undo)) continue;
        int score;
        if (move_number == 0) {
            score = -negamax(position, depth - 1, -beta, -alpha, ply + 1,
                             pv_node, move);
        } else {
            int reduction = 0;
#if BUBU_ENABLE_LMR
            if (!checked && quiet && depth >= params::lmr_min_depth &&
                move_number >= params::lmr_min_move_number)
                reduction = 1 + (depth >= params::lmr_deep_depth &&
                                 move_number >= params::lmr_deep_move_number);
#endif
            score = -negamax(position, std::max(0, depth - 1 - reduction),
                             -alpha - 1, -alpha, ply + 1, false, move);
            if (score > alpha && reduction != 0)
                score = -negamax(position, depth - 1, -alpha - 1, -alpha,
                                 ply + 1, false, move);
            if (score > alpha && score < beta)
                score = -negamax(position, depth - 1, -beta, -alpha,
                                 ply + 1, pv_node, move);
        }
        position.unmake_move(move, undo);
        if (stop_requested_.load(std::memory_order_relaxed)) return 0;
        ++move_number;
        if (score > best_score) {
            best_score = score;
            best_move = move;
        }
        if (score > alpha) {
            alpha = score;
            pv_table_[ply][ply] = move;
            for (int pv = ply + 1; pv < pv_length_[ply + 1]; ++pv)
                pv_table_[ply][pv] = pv_table_[ply + 1][pv];
            pv_length_[ply] = pv_length_[ply + 1];
        }
        if (alpha >= beta) {
            if (quiet && ply < max_ply) {
                if (killers_[ply][0] != move) {
                    killers_[ply][1] = killers_[ply][0];
                    killers_[ply][0] = move;
                }
                int& history = history_[color_index(position.side_to_move)][move.from][move.to];
                history = std::min(16384, history + depth * depth);
                if (previous_move.from != no_square && is_piece(moved_piece))
                    counter_moves_[static_cast<int>(moved_piece)][previous_move.to] = move;
            }
            break;
        }
    }
    const Bound bound = best_score <= original_alpha ? Bound::upper
                        : best_score >= beta ? Bound::lower
                                             : Bound::exact;
    store(position.hash, best_move, best_score, depth, bound, ply);
    return best_score;
}

SearchResult Searcher::search(
    Position root, const SearchLimits& limits,
    const std::function<void(const SearchInfo&)>& report) {
    limits_ = limits;
    stop_requested_.store(false, std::memory_order_relaxed);
    nodes_ = 0;
    selective_depth_ = 0;
    ++generation_;
    killers_ = {};
    history_ = {};
    counter_moves_ = {};
    pv_length_ = {};
    start_time_ = std::chrono::steady_clock::now();
    soft_time_ms_ = 0;
    hard_time_ms_ = 0;
    time_check_interval_ = 256;
    has_hard_time_limit_ = false;
    emergency_clock_ = false;
    if (!limits.infinite) {
        if (limits.movetime_ms > 0) {
            const int available = std::max(0, limits.movetime_ms -
                                                  limits.move_overhead_ms);
            emergency_clock_ = limits.movetime_ms <= limits.move_overhead_ms + 10;
            const int allocation_ceiling = std::max(1, available / 2);
            soft_time_ms_ = static_cast<int>(std::clamp<std::int64_t>(
                static_cast<std::int64_t>(available) * 4 / 5,
                std::min(20, allocation_ceiling), allocation_ceiling));
            hard_time_ms_ = static_cast<int>(std::min<std::int64_t>(
                available, static_cast<std::int64_t>(soft_time_ms_) * 3));
            has_hard_time_limit_ = available > 0;
        } else {
            const int remaining = root.side_to_move == Color::white
                                      ? limits.white_time_ms
                                      : limits.black_time_ms;
            const int increment = root.side_to_move == Color::white
                                      ? limits.white_increment_ms
                                      : limits.black_increment_ms;
            emergency_clock_ = limits.clock_time_supplied &&
                               remaining <= limits.move_overhead_ms + 10;
            if (limits.clock_time_supplied) {
                const int available = std::max(0, remaining -
                                                      limits.move_overhead_ms);
                const int moves = limits.moves_to_go > 0
                                      ? limits.moves_to_go
                                      : 30;
                const std::int64_t planned_allocation =
                    static_cast<std::int64_t>(available) / moves +
                    static_cast<std::int64_t>(increment) * 3 / 4;
                const int allocation_ceiling = std::max(1, available / 2);
                soft_time_ms_ = static_cast<int>(std::clamp<std::int64_t>(
                    planned_allocation, std::min(20, allocation_ceiling),
                    allocation_ceiling));
                hard_time_ms_ = static_cast<int>(std::min<std::int64_t>(
                    available, static_cast<std::int64_t>(soft_time_ms_) * 3));
                has_hard_time_limit_ = available > 0;
            }
        }
    }

    if (has_hard_time_limit_) {
        if (hard_time_ms_ <= 50) time_check_interval_ = 8;
        else if (hard_time_ms_ <= 200) time_check_interval_ = 16;
        else if (hard_time_ms_ <= 1000) time_check_interval_ = 32;
    }

    SearchResult result;
    MoveList root_moves = generate_legal_moves(root);
    if (root_moves.count == 0) return result;
    result.best_move = root_moves.moves[0];
    if (emergency_clock_ && root_moves.count == 1) return result;
#if BUBU_ENABLE_ASPIRATION
    int previous_score = 0;
#endif
    const int target_depth = emergency_clock_
                                 ? 2
                                 : std::clamp(limits.depth, 1, max_ply - 2);
    for (int depth = 1; depth <= target_depth; ++depth) {
        current_depth_ = depth;
#if BUBU_ENABLE_ASPIRATION
        int alpha = depth >= params::aspiration_min_depth
                        ? previous_score - params::aspiration_initial_window
                        : -mate_score;
        int beta = depth >= params::aspiration_min_depth
                       ? previous_score + params::aspiration_initial_window
                       : mate_score;
#else
        int alpha = -mate_score;
        int beta = mate_score;
#endif
        int score = 0;
        for (;;) {
            score = negamax(root, depth, alpha, beta, 0, true, Move{});
            if (should_stop()) break;
            if (score <= alpha) {
                alpha = std::max(-mate_score, alpha - (beta - alpha) * 2);
                continue;
            }
            if (score >= beta) {
                beta = std::min(mate_score, beta + (beta - alpha) * 2);
                continue;
            }
            break;
        }
        if (should_stop()) break;
#if BUBU_ENABLE_ASPIRATION
        previous_score = score;
#endif
        result.depth = depth;
        result.score = score;
        result.nodes = nodes_;
        if (pv_length_[0] > 0) result.best_move = pv_table_[0][0];

        SearchInfo info;
        info.depth = depth;
        info.selective_depth = selective_depth_;
        info.score = score;
        info.nodes = nodes_;
        info.time_ms = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time_).count());
        info.hashfull = hashfull();
        info.pv_length = pv_length_[0];
        for (int index = 0; index < info.pv_length; ++index)
            info.pv[index] = pv_table_[0][index];
        report(info);

        const bool minimum_timed_depth_done =
            depth >= 2 || (!limits.clock_time_supplied && limits.movetime_ms <= 0);
        if (minimum_timed_depth_done &&
            (std::abs(score) > mate_threshold ||
             (soft_time_ms_ > 0 &&
              static_cast<int>(info.time_ms) >= soft_time_ms_)))
            break;
    }
    result.nodes = nodes_;
    return result;
}

}  // namespace bubu
