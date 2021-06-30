#include "board.hpp"
#include "types.hpp"

namespace Ordering {
Bitboard get_smallest_attacker(Board &board, const Square origin, const Bitboard mask, const Colour side);
score_t see(Board &board, const Square target, Colour side, PieceType pt, Bitboard mask);
score_t see_capture(Board &board, const Move move);
void sort_moves(Board &board, MoveList &legal_moves, const DenseMove hash_dmove, const KillerTableRow killer_moves);
} // namespace Ordering