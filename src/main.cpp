#include <iostream>

#include "game/piece.hpp"
#include "game/board.hpp"

void print_vector(std::vector<Square> &moves) {
    for (Square move : moves) {
        std::cout << move << std::endl;
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
    
    board.get_moves();
}