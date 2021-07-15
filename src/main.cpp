#include "bitboard.hpp"
#include "board.hpp"
#include "cache.hpp"
#include "evaluate.hpp"
#include "movegen.hpp"
#include "printing.hpp"
#include "search.hpp"
#include "transposition.hpp"
#include "uci.hpp"
#include "zobrist.hpp"
#include <iostream>

int main() {
    Bitboards::init();
    Cache::init();
    GameCache::init();
    Evaluation::init();
    Search::init();
    Zobrist::init();

    UCI::uci();
}