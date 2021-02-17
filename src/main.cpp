#include <iostream>

#include "game/piece.hpp"
#include "game/board.hpp"

void print_vector(std::vector<Board::coord> &moves) {
    for (Board::coord move : moves) {
        std::cout << Board::idx_to_str(move) << std::endl;
    }
}

int main(int argc, char* argv[])
{
    std::cout << "admete" << std::endl << std::endl;
    Board board = Board();
    board.fen_decode("rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq c6 0 2");
    board.print_board_idx();
    std::cout << std::endl;
    board.print_board_extra();
    
    std::vector<Board::coord> knight = possible_knight_moves(Board::str_to_index("g6"));
    print_vector(knight);
}