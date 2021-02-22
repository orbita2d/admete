#include "piece.hpp"
#include <map>
#include <string>

std::map<uint8_t, char> pretty_print_map = {
    { Pieces::Blank, '.' },
    { Pieces::Pawn, 'p' },
    { Pieces::Knight, 'n' },
    { Pieces::Bishop, 'b' },
    { Pieces::Rook, 'r' },
    { Pieces::Queen, 'q' },
    { Pieces::King, 'k' }
};

char Piece::pretty_print() const{
        Piece piece = get_piece();
        Piece colour = get_colour();
        char character = pretty_print_map[piece];
        if (piece == Pieces::Blank) {
            return character;
        }
        if (colour == Pieces::Black) {
            return character;
        } else if (colour == Pieces::White){
            return toupper(character);
        } else { return 'X'; }
       
    }

std::string Piece::get_algebraic_character() const{
    if (is_piece(Pieces::Pawn)) {
        return "";
    }else if (is_piece(Pieces::Knight)) {
        return "N";
    }else if (is_piece(Pieces::Bishop)) {
        return "B";
    }else if (is_piece(Pieces::Rook)) {
        return "R";
    }else if (is_piece(Pieces::Queen)) {
        return "Q";
    }else if (is_piece(Pieces::King)) {
        return "K";
    } else {
        return "X";
    }
}

std::ostream& operator<<(std::ostream& os, const Piece piece) {
    os << piece.pretty_print();
    return os;
}

int material(const Piece piece) {
    int side_multiplier = piece.is_white() ? 1 : -1;
    if (piece.is_blank()) {return 0; }
    if (piece.is_pawn()) {return side_multiplier * 100; }
    else if (piece.is_knight()) {return side_multiplier * 300; }
    else if (piece.is_bishop()) {return side_multiplier * 300; }
    else if (piece.is_rook()) {return side_multiplier * 500; }
    else if (piece.is_queen()) {return side_multiplier * 900; }
    else {return 0;}
}