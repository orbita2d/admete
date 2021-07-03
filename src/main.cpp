#include "bitboard.hpp"
#include "board.hpp"
#include "evaluate.hpp"
#include "movegen.hpp"
#include "printing.hpp"
#include "search.hpp"
#include "transposition.hpp"
#include "uci.hpp"
#include <iostream>

int main() {
    Bitboards::init();
    Zorbist::init();
    Evaluation::init();
    Cache::init();

    UCI::uci();
}