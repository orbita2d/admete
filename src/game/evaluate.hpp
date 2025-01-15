#include "board.hpp"

score_t piece_material(const PieceType);

// Type for a single piece-square table
typedef per_square<Score> sqt_t;
typedef per_piece<sqt_t> psqt_t;

namespace Evaluation {
void init();
inline sqt_t pb_passed;

// Calculate the evaluation heuristic from the player's POV
score_t eval(const Board &board);

// Calculate the evaluation heuristic from white's POV.
score_t evaluate_white(const Board &board);

// Calculate the pawn structure evaluation from white's POV.
Score eval_pawns(const Board &board);

score_t evaluate_safe(const Board &board);
score_t terminal(const Board &board);
score_t piece_phase_material(const PieceType p);
score_t count_phase_material(const Board &board);

score_t drawn_score(const Board &board);
constexpr score_t contempt = -10;
sqt_t reverse_board(sqt_t in);

inline Score weak_pawn = Score(-5, -5);        // Penalty for pawn not defended by another pawn.
inline Score isolated_pawn = Score(-10, -10);  // Penalty for pawn with no supporting pawns on adjacent files.
inline Score connected_passed = Score(20, 20); // Bonus for connected passed pawns (per pawn)
inline Score defended_passed = Score(0, 0);    // Bonus for defended passed pawns.
inline Score rook_open_file = Score(10, 5);    // Bonus for rook on open file
inline Score rook_hopen_file = Score(5, 0);    // Bonus for rook on half open file
inline Score doubled_pawns = Score(-5, -0);    // Pawn in front of another pawn. (Applied once, not twice)

inline Score castle_hopen_file = Score(-5, 0); // Penalty for castled king near a half open file.

// Bonus given the pawns on the 2nd and 3rd ranks in front of a castled king. This promotes castling and penalises
// pushing pawns in front of the king.
inline Score castle_pawns2 = Score(6, 0);
inline Score castle_pawns3 = Score(4, 0);

// Penealty for squares where a queen would check the king if only pawns were on the board.
inline Score queen_check = Score(-8, 0);

// Bonus for every square accessible (that isn't protected by a pawn) to every piece.
inline per_piece<Score> mobility = {{
    Score(0, 0), // Pawn, not included
    Score(6, 8), // Knight
    Score(6, 8), // Bishop
    Score(6, 8), // Rook
    Score(6, 8), // Queen
    Score(0, 0), // King, not included
}};

// Bonus for having the bishop pair.
inline Score bishop_pair = Score(15, 25);

// Bonus for a piece on a weak enemy square.
inline Score piece_weak_square = Score(5, 0);

// Bonus for a knight on an outpost.
inline Score knight_outpost = Score(15, 0);

// Bonus for an outpost (occupied or otherwise).
inline Score outpost = Score(5, 0);

// Bonus for rook behind a passed pawn.
inline Score rook_behind_passed = Score(5, 20);

// Multiplier for special psqt to push enemy king into corner same colour as our only bishop.
inline Score bishop_corner_multiplier = Score(0, 8);

// Vector of pointers to training parameters
typedef std::pair<Score *, std::string> labled_parameter;
inline std::vector<labled_parameter> training_parameters;
// Set up constants
void constants();
} // namespace Evaluation
