#include "board.hpp"
#include "types.hpp"

namespace Ordering {
void sort_moves(MoveList &legal_moves);
void rank_and_sort_moves(Board &board, MoveList &legal_moves, const DenseMove hash_dmove);
} // namespace Ordering

namespace SEE {
// This material includes a high value for king and a 0 for NO_PIECE.
constexpr score_t material[N_PIECE + 2] = {100, 320, 350, 500, 900, MAX_SCORE, 0, 0};
Bitboard get_smallest_attacker(Board &board, const Square origin, const Bitboard mask, const Colour side);
score_t see(Board &board, const Square target, Colour side, PieceType pt, Bitboard mask);
// Returns true if the see for a capture is greater than threshold.
bool see(Board &board, const Move move, const int threshold);
score_t see_capture(Board &board, const Move move);
score_t see_quiet(Board &board, const Move move);
} // namespace SEE