#include "board.hpp"

enum MoveGen {
    PSEUDOLEGAL,
    LEGAL,
    CAPTURES,
    EVASIONS
};

constexpr int MAX_MOVES = 256;
typedef std::vector<Move> MoveList;