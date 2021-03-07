#pragma once
#include "../game/board.hpp"

typedef std::vector<Move> PrincipleLine;

constexpr int NEG_INF = -1000000;
constexpr int POS_INF = +1000000;

// perft.cpp
unsigned int perft_bulk(unsigned int depth, Board &board);
void perft_divide(unsigned int depth, Board &board);

// search.cpp
int alphabeta(Board& board, const uint depth, int alpha, int beta, PrincipleLine& line);
int alphabeta(Board& board, const uint depth, PrincipleLine& line);
int quiesce(Board& board, int alpha, int beta);
int pv_search(Board& board, const uint depth, int alpha, int beta, PrincipleLine& principle, const uint pv_depth, PrincipleLine& line);
int iterative_deepening(Board& board, const uint depth, const int max_millis, PrincipleLine& line);
int iterative_deepening(Board& board, const uint depth, PrincipleLine& line);
int find_best_random(Board& board, const uint depth, const int weight, const int max_millis, PrincipleLine& line);