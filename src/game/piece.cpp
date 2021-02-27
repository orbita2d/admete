#include "piece.hpp"
#include <map>
#include <string>

std::map<uint8_t, std::string> pretty_print_map = {
    { Pieces::Blank, "." },
    { Pieces::White | Pieces::Pawn, "P" },
    { Pieces::White | Pieces::Knight, "N" },
    { Pieces::White |  Pieces::Bishop, "B" },
    { Pieces::White | Pieces::Rook, "R" },
    { Pieces::White | Pieces::Queen, "Q" },
    { Pieces::White | Pieces::King, "K" },
    { Pieces::Black | Pieces::Pawn, "p" },
    { Pieces::Black | Pieces::Knight, "n" },
    { Pieces::Black |  Pieces::Bishop, "b" },
    { Pieces::Black | Pieces::Rook, "r" },
    { Pieces::Black | Pieces::Queen, "q" },
    { Pieces::Black | Pieces::King, "k" }
};


std::map<uint8_t, std::string> prettier_print_map = {
    { Pieces::Blank, "." },
    { Pieces::White | Pieces::Pawn, "♙" },
    { Pieces::White | Pieces::Knight, "♘" },
    { Pieces::White | Pieces::Bishop, "♗" },
    { Pieces::White | Pieces::Rook, "♖" },
    { Pieces::White | Pieces::Queen, "♕" },
    { Pieces::White | Pieces::King, "♔" },
    { Pieces::Black | Pieces::Pawn, "♟︎" },
    { Pieces::Black | Pieces::Knight, "♞" },
    { Pieces::Black | Pieces::Bishop, "♝" },
    { Pieces::Black | Pieces::Rook, "♜" },
    { Pieces::Black | Pieces::Queen, "♛" },
    { Pieces::Black | Pieces::King, "♚" }
};

std::string Piece::pretty_print() const{
        Piece piece = get_piece();
        Piece colour = get_colour();
        return pretty_print_map[piece];
}

std::string Piece::prettier_print() const{
        Piece piece = get_piece();
        Piece colour = get_colour();
        return prettier_print_map[piece | colour];
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