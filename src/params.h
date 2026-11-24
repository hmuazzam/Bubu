#pragma once

namespace bubu::params {

// Compact opening/endgame material values are original starting parameters,
// intended for later tuning from Bubu self-play rather than copied tables.
constexpr int pawn_mg = 100;
constexpr int knight_mg = 318;
constexpr int bishop_mg = 330;
constexpr int rook_mg = 500;
constexpr int queen_mg = 950;
constexpr int pawn_eg = 120;
constexpr int knight_eg = 305;
constexpr int bishop_eg = 325;
constexpr int rook_eg = 525;
constexpr int queen_eg = 930;
constexpr int bishop_pair_mg = 28;
constexpr int bishop_pair_eg = 42;
constexpr int doubled_pawn = 13;
constexpr int isolated_pawn = 15;
constexpr int backward_pawn = 9;
constexpr int connected_pawn = 8;
constexpr int passed_pawn_base = 8;
constexpr int mobility_unit = 2;
constexpr int rook_open_file_mg = 18;
constexpr int rook_semi_open_file_mg = 9;
constexpr int rook_seventh_mg = 14;
constexpr int king_shield_unit = 8;
constexpr int king_zone_attack_unit = 3;
constexpr int tempo = 12;
constexpr int eval_scale_percent = 100;
constexpr int pawn_advance_mg = 5;
constexpr int pawn_advance_eg = 9;
constexpr int pawn_center_mg = 1;
constexpr int pawn_center_eg = 2;
constexpr int knight_center_mg = 5;
constexpr int knight_center_eg = 4;
constexpr int bishop_center = 3;
constexpr int queen_center_mg = 1;
constexpr int queen_center_eg = 2;
constexpr int rook_seventh_pst = 8;
constexpr int king_center_mg = 3;
constexpr int king_center_eg = 5;
constexpr int king_home_bonus = 12;
constexpr int rook_advance_eg = 2;
constexpr int phase_knight = 1;
constexpr int phase_bishop = 1;
constexpr int phase_rook = 2;
constexpr int phase_queen = 4;
constexpr int max_phase = 24;
constexpr int rfp_max_depth = 3;
constexpr int rfp_margin_per_ply = 90;
constexpr int nmp_min_depth = 3;
constexpr int nmp_base_reduction = 2;
constexpr int nmp_depth_divisor = 4;
constexpr int nmp_max_extra_reduction = 2;
constexpr int nmp_eval_margin_scale = 200;
constexpr int iir_min_depth = 5;
constexpr int lmp_max_depth = 3;
constexpr int lmp_base_moves = 3;
constexpr int futility_max_depth = 2;
constexpr int futility_margin_per_ply = 110;
constexpr int lmr_min_depth = 3;
constexpr int lmr_min_move_number = 3;
constexpr int lmr_deep_depth = 6;
constexpr int lmr_deep_move_number = 8;
constexpr int aspiration_min_depth = 4;
constexpr int aspiration_initial_window = 24;

}  // namespace bubu::params
