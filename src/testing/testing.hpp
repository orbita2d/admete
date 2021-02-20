#pragma once
#include "../game/board.hpp"
unsigned int perft(unsigned int depth, Board &board);
unsigned int pseudolegal_perft(unsigned int depth, Board &board);
void perft_divide(unsigned int depth, Board &board);