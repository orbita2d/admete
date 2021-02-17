#include "piece.hpp"
#include <map>
#include <string>

std::map<uint8_t, char> pretty_print_map = {
    { Piece::Blank, '.' },
    { Piece::Pawn, 'p' },
    { Piece::Knight, 'n' },
    { Piece::Bishop, 'b' },
    { Piece::Rook, 'r' },
    { Piece::Queen, 'q' },
    { Piece::King, 'k' }
};

char Piece::pretty_print(){
        uint8_t piece = get_piece();
        uint8_t colour = get_colour();
        char character = pretty_print_map[piece];
        if (piece == Piece::Blank) {
            return character;
        }
        if (colour == Piece::Black) {
            return character;
        } else if (colour == Piece::White){
            return toupper(character);
        } else { return 'X'; }
       
    }