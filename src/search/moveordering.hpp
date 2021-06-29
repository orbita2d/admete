#include "board.hpp"
#include "types.hpp"

namespace Ordering {
void sort_moves(Board &board, MoveList &legal_moves, const DenseMove hash_dmove, const KillerTableRow killer_moves);
}