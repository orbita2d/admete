#include "board.hpp"

void print_tables();
score_t evaluate_material(Board &board);
score_t evaluate_lazy(Board &board, std::vector<Move> &legal_moves);
score_t piece_material(const PieceType);

// Type for a single piece-square table
typedef per_square<score_t> psqt_t;

namespace Evaluation {
void init();
void load_tables(std::string filename);
void save_tables(std::string filename);
void print_tables();
inline std::array<per_piece<psqt_t>, N_GAMEPHASE> piece_square_tables;
inline per_square<Score> pb_passed;

// Calculate the evaluation heuristic from the player's POV
score_t eval(const Board &board);

// Calculate the evaluation heuristic from white's POV.
score_t evaluate_white(const Board &board);

// Calculate the pawn structure evaluation from white's POV.
Score eval_pawns(const Board &board);

Score psqt(const Board &board);
Score psqt_diff(const Colour moving, const Move &move);
score_t eval_psqt(const Board &board);
score_t evaluate_safe(const Board &board);
score_t terminal(const Board &board);
score_t piece_phase_material(const PieceType p);
Score piece_value(const PieceType p);
score_t count_material(const Board &board);
score_t drawn_score(const Board &board);
constexpr score_t contempt = -10;
psqt_t reverse_board(psqt_t in);
} // namespace Evaluation
