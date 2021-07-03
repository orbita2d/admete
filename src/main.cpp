#include "bitboard.hpp"
#include "board.hpp"
#include "evaluate.hpp"
#include "movegen.hpp"
#include "printing.hpp"
#include "search.hpp"
#include "transposition.hpp"
#include "uci.hpp"
#include <iostream>

int main(int argc, char *argv[]) {
    Bitboards::init();
    Zorbist::init();
    Evaluation::init();
    Cache::init();

    std::string command;
    std::cin >> command;

    if (command == "uci") {
        UCI::uci();
    }
}