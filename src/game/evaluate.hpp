#include "board.hpp"

void print_tables();
int heuristic_diff(Colour us, Move &move, int material_value);
int negamax_heuristic(Board &board);
int evaluate(Board &board);
int evaluate(Board &board, std::vector<Move> &legal_moves);
int evaluation_diff(Board &board, Move move);
int evaluate_material(Board &board);
int evaluate_lazy(Board &board, std::vector<Move> &legal_moves);
int count_material(Board &board);
int piece_value(const PieceType);