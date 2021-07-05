#include "board.hpp"

namespace Tablebase {
bool init(const std ::string filename);
bool probe_root(Board &board, MoveList &moves);
bool probe_wdl(Board &board, score_t &result);
} // namespace Tablebase