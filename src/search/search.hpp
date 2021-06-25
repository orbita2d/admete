#pragma once
#include "../game/board.hpp"
#include <chrono>

typedef std::vector<Move> PrincipleLine;

// perft.cpp
unsigned long perft_comparison(depth_t depth, Board &board);
unsigned long perft(depth_t depth, Board &board);
void perft_divide(depth_t depth, Board &board);

// search.cpp
score_t alphabeta(Board& board, const depth_t depth, score_t alpha, score_t beta, PrincipleLine& line, long &nodes, std::chrono::high_resolution_clock::time_point time_cutoff, bool &kill_flag, bool allow_cutoff);
score_t quiesce(Board& board, score_t alpha, score_t beta, long &nodes);
score_t pv_search(Board& board, const depth_t depth, score_t alpha, score_t beta, PrincipleLine& principle, const uint pv_depth, PrincipleLine& line, long &nodes, std::chrono::high_resolution_clock::time_point time_cutoff, bool &kill_flag, bool allow_cutoff);
score_t iterative_deepening(Board& board, const depth_t depth, const int max_millis, PrincipleLine& line, long &nodes);
score_t iterative_deepening(Board& board, const depth_t depth, PrincipleLine& line);
PrincipleLine unroll_tt_line(Board& board);
PrincipleLine unroll_tt_line(Board& board, PrincipleLine principle);