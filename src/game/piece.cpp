#include "piece.hpp"
#include <map>
#include <string>

std::map<uint8_t, std::string> pretty_print_map = {
    { Pieces::Blank, "." },
    { Piece(WHITE, PAWN), "P" },
    { Piece(WHITE, KNIGHT), "N" },
    { Piece(WHITE, BISHOP), "B" },
    { Piece(WHITE, ROOK), "R" },
    { Piece(WHITE, QUEEN), "Q" },
    { Piece(WHITE, KING), "K" },
    { Piece(BLACK, PAWN), "p" },
    { Piece(BLACK, KNIGHT), "n" },
    { Piece(BLACK, BISHOP), "b" },
    { Piece(BLACK, ROOK), "r" },
    { Piece(BLACK, QUEEN), "q" },
    { Piece(BLACK, KING), "k" }
};


std::map<uint8_t, std::string> prettier_print_map = {
    { Pieces::Blank, "." },
    { Piece(WHITE, PAWN), "♙" },
    { Piece(WHITE, KNIGHT), "♘" },
    { Piece(WHITE, BISHOP), "♗" },
    { Piece(WHITE, ROOK), "♖" },
    { Piece(WHITE, QUEEN), "♕" },
    { Piece(WHITE, KING), "♔" },
    { Piece(BLACK, PAWN), "♟︎" },
    { Piece(BLACK, KNIGHT), "♞" },
    { Piece(BLACK, BISHOP), "♝" },
    { Piece(BLACK, ROOK), "♜" },
    { Piece(BLACK, QUEEN), "♛" },
    { Piece(BLACK, KING), "♚" }
};

std::string Piece::pretty_print() const{
        return pretty_print_map[value];
}

std::string Piece::prettier_print() const{
        Piece piece = get_piece();
        Piece colour = get_colour();
        return prettier_print_map[piece | colour];
    }

std::string Piece::get_algebraic_character() const{
    if (is_pawn()) {
        return "";
    }else if (is_knight()) {
        return "N";
    }else if (is_bishop()) {
        return "B";
    }else if (is_rook()) {
        return "R";
    }else if (is_queen()) {
        return "Q";
    }else if (is_king()) {
        return "K";
    } else {
        return "X";
    }
}

std::ostream& operator<<(std::ostream& os, const Piece piece) {
    os << piece.pretty_print();
    return os;
}
