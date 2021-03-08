#include "board.hpp"

enum MoveGen {
    PSEUDOLEGAL,
    LEGAL,
    CAPTURES,
    EVASIONS
};