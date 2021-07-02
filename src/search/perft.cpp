#include "../game/board.hpp"
#include "search.hpp"
#include "transposition.hpp"
#include <iostream>

namespace Search {

unsigned long perft_bulk(depth_t depth, Board &board);

unsigned long perft_bulk(depth_t depth, Board &board) {

    std::vector<Move> legal_moves = board.get_moves();
    if (depth == 1) {
        return legal_moves.size();
    }

    unsigned long nodes = 0;
    for (Move move : legal_moves) {
        board.make_move(move);
        nodes += perft_bulk(depth - 1, board);
        board.unmake_move(move);
    }
    return nodes;
}

void perft_divide(depth_t depth, Board &board) {
    std::vector<Move> legal_moves = board.get_moves();
    unsigned long nodes = 0;
    unsigned long child_nodes;
    for (auto move : legal_moves) {
        board.make_move(move);
        child_nodes = perft(depth - 1, board);
        std::cout << move.pretty() << ": " << std::dec << child_nodes << std::endl;
        nodes += child_nodes;
        board.unmake_move(move);
    }
    std::cout << "nodes searched: " << nodes << std::endl;
}

unsigned long perft(depth_t depth, Board &board) {

    std::vector<Move> legal_moves = board.get_moves();
    if (depth == 0) {
        return 1;
    }

    unsigned long nodes = 0;
    for (Move move : legal_moves) {
        board.make_move(move);
        nodes += perft(depth - 1, board);
        board.unmake_move(move);
    }
    return nodes;
}
} // namespace Search