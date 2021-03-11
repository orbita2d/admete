#include "board.hpp"

enum MoveGen {
    PSEUDOLEGAL,
    LEGAL,
    CAPTURES,
    EVASIONS
};


typedef std::vector<Move> MoveList;