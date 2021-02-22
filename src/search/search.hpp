#pragma once
#include "../game/board.hpp"

typedef std::vector<Move> PrincipleLine;
int minimax(Board& board, const uint depth);
int find_best(Board& board, const uint depth, PrincipleLine& line);