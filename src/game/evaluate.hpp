#include "board.hpp"

void print_tables();
score_t evaluate_material(Board &board);
score_t evaluate_lazy(Board &board, std::vector<Move> &legal_moves);
score_t piece_value(const PieceType);

typedef std::array<score_t, 64> position_board;
typedef std::array<position_board, 6> position_board_set;

namespace Evaluation {
void init();
void load_tables(std::string filename);
score_t heuristic(Board &board);
score_t negamax_heuristic(Board &board);
score_t evaluate(Board &board);
score_t evaluate(Board &board, std::vector<Move> &legal_moves);
score_t count_material(const Board &board);
constexpr score_t OPENING_MATERIAL = 6000;
constexpr score_t ENDGAME_MATERIAL = 2000;
} // namespace Evaluation
