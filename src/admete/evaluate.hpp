#include "board.hpp"

score_t piece_material(const PieceType);

// Type for a single piece-square table
typedef per_square<Score> sqt_t;
typedef per_piece<sqt_t> psqt_t;

namespace Evaluation {
// Calculate the evaluation heuristic from the player's POV
score_t eval(const Board &board);

// Calculate the evaluation heuristic from white's POV.
score_t evaluate_white(const Board &board);

score_t evaluate_safe(const Board &board);
score_t terminal(const Board &board);
score_t piece_phase_material(const PieceType p);
score_t count_phase_material(const Board &board);

score_t drawn_score(const Board &board);
constexpr score_t contempt = -10;
sqt_t reverse_board(sqt_t in);

// Vector of pointers to training parameters
typedef std::pair<Score *, std::string> labled_parameter;
inline std::vector<labled_parameter> training_parameters;
} // namespace Evaluation
