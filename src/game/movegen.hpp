#include "board.hpp"

enum GenType { PSEUDOLEGAL, QUIET, LEGAL, CAPTURES, EVASIONS, CHECKS, QUIETCHECKS };

constexpr int MAX_MOVES = 256;