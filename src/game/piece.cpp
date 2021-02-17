#include "piece.hpp"
#include <map>
#include <string>

const Piece Piece::Blank = Piece(0);
const Piece Piece::Pawn = Piece(1);
const Piece Piece::Knight = Piece(2);
const Piece Piece::Bishop = Piece(3);
const Piece Piece::Rook = Piece(4);
const Piece Piece::Queen = Piece(5);
const Piece Piece::King = Piece(6);
const Piece Piece::White = Piece(16);
const Piece Piece::Black = Piece(32);

std::map<uint8_t, char> pretty_print_map = {
    { Piece::Blank, '.' },
    { Piece::Pawn, 'p' },
    { Piece::Knight, 'n' },
    { Piece::Bishop, 'b' },
    { Piece::Rook, 'r' },
    { Piece::Queen, 'q' },
    { Piece::King, 'k' }
};

char Piece::pretty_print() const{
        Piece piece = get_piece();
        Piece colour = get_colour();
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

std::ostream& operator<<(std::ostream& os, const Piece piece) {
    os << piece.pretty_print();
    return os;
}