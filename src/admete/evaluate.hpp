#include "board.hpp"

score_t piece_material(const PieceType);

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
inline score_t contempt = 10;
} // namespace Evaluation
