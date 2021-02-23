#pragma once
#include "../game/board.hpp"

typedef std::vector<Move> PrincipleLine;
int minimax(Board& board, const uint depth);
int minimax(Board& board, const uint depth, PrincipleLine& line);
int alphabeta(Board& board, const uint depth, int alpha, int beta, const bool maximising);
int alphabeta(Board& board, const uint depth, int alpha, int beta, const bool maximising, PrincipleLine& line);
int alphabeta(Board& board, const uint depth, PrincipleLine& line);
int pv_search(Board& board, const uint depth, int alpha, int beta, const bool maximising, PrincipleLine& principle, const uint pv_depth, PrincipleLine& line);
int iterative_deepening(Board& board, const uint depth, PrincipleLine& line);