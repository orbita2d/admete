#include "board.hpp"
#include "types.hpp"

namespace Ordering {
void sort_moves(Board &board, MoveList &legal_moves, const DenseMove hash_dmove, const KillerTableRow killer_moves);
} // namespace Ordering

namespace SEE {
// This material includes a high value for king and a 0 for NO_PIECE.
constexpr score_t material[N_PIECE + 1] = {100, 320, 350, 500, 900, MAX_SCORE, 0};
Bitboard get_smallest_attacker(Board &board, const Square origin, const Bitboard mask, const Colour side);
score_t see(Board &board, const Square target, Colour side, PieceType pt, Bitboard mask);
score_t see_capture(Board &board, const Move move);
score_t see_quiet(Board &board, const Move move);
}