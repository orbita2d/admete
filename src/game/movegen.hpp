#include "board.hpp"

enum GenType {
    PSEUDOLEGAL,
    QUIET,
    LEGAL,
    CAPTURES,
    EVASIONS
};

constexpr int MAX_MOVES = 256;