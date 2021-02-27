#include "../game/board.hpp"
#include "search.hpp"
#include <iostream>

unsigned int perft(unsigned int depth, Board &board) {
    if (depth == 0) {
        return 1;
    }
    std::vector<Move> legal_moves = board.get_moves();
    unsigned int nodes = 0;
    for (Move move : legal_moves) {
        board.make_move(move);
        nodes += perft(depth-1, board);
        board.unmake_move(move);
    }
    return nodes;

}

unsigned int perft_bulk(unsigned int depth, Board &board) {
    std::vector<Move> legal_moves = board.get_moves();

    if (depth == 1) {
        return legal_moves.size();
    }

    unsigned int nodes = 0;
    for (Move move : legal_moves) {
        board.make_move(move);
        nodes += perft_bulk(depth-1, board);
        board.unmake_move(move);
    }
    return nodes;
}

unsigned int pseudolegal_perft(unsigned int depth, Board &board) {
    if (depth == 0) {
        return 1;
    }
    std::vector<Move> moves = board.get_pseudolegal_moves();
    unsigned int nodes = 0;
    for (Move move : moves) {
        board.make_move(move);
        if (!board.is_in_check()) {
            nodes += pseudolegal_perft(depth-1, board);
        }
        board.unmake_move(move);
    }
    return nodes;

}

void perft_divide(unsigned int depth, Board &board) {
    std::vector<Move> legal_moves = board.get_moves();
    unsigned int nodes = 0;
    unsigned int child_nodes;
    for (auto move : legal_moves) {
        board.make_move(move);
        child_nodes =  perft(depth-1, board);
        std::cout << move.pretty_print() << ": "<< child_nodes << std::endl;
        nodes += child_nodes;
        board.unmake_move(move);
    }
    std::cout << "nodes searched: " << nodes << std::endl;
}