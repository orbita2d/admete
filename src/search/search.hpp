#pragma once
#include "../game/board.hpp"

typedef std::vector<Move> PrincipleLine;

constexpr int NEG_INF = -1000000000;
constexpr int POS_INF = +1000000000;

// perft.cpp
unsigned long perft_comparison(depth_t depth, Board &board);
unsigned long perft(depth_t depth, Board &board);
void perft_divide(depth_t depth, Board &board);

// search.cpp
int alphabeta(Board& board, const depth_t depth, int alpha, int beta, PrincipleLine& line, long &nodes);
int alphabeta(Board& board, const depth_t depth, PrincipleLine& line, long &nodes);
int quiesce(Board& board, int alpha, int beta, long &nodes);
int pv_search(Board& board, const depth_t depth, int alpha, int beta, PrincipleLine& principle, const uint pv_depth, PrincipleLine& line, long &nodes);
int iterative_deepening(Board& board, const depth_t depth, const int max_millis, PrincipleLine& line, long &nodes);
int iterative_deepening(Board& board, const depth_t depth, PrincipleLine& line);