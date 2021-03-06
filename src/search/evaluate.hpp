#include "board.hpp"

int evaluate(Board &board);
int evaluate(Board &board, std::vector<Move> &legal_moves);
int evaluation_diff(Board &board, Move move);
int evaluate_material(Board &board);
int evaluate_lazy(Board &board, std::vector<Move> &legal_moves);