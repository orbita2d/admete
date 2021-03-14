#include <iostream>
#include <istream>
#include <sstream>
#include <iomanip>

#include "board.hpp"

#define toDigit(c) ( c - '0')


std::map<Piece, char> fen_encode_map = {
    {Pieces::Black | Pieces::Pawn, 'p'},
    {Pieces::Black | Pieces::Knight, 'n'},
    {Pieces::Black | Pieces::Bishop, 'b'},
    {Pieces::Black | Pieces::Rook, 'r'},
    {Pieces::Black | Pieces::Queen, 'q'},
    {Pieces::Black | Pieces::King, 'k'},
    {Pieces::White | Pieces::Pawn, 'P'},
    {Pieces::White | Pieces::Knight, 'N'},
    {Pieces::White | Pieces::Bishop, 'B'},
    {Pieces::White | Pieces::Rook, 'R'},
    {Pieces::White | Pieces::Queen, 'Q'},
    {Pieces::White | Pieces::King, 'K'},
};

std::map<char, Square::square_t> file_decode_map = {
    {'a', 0},
    {'b', 1},
    {'c', 2},
    {'d', 3},
    {'e', 4},
    {'f', 5},
    {'g', 6},
    {'h', 7},

};

std::map<Square::square_t, char> file_encode_map = {
    {0, 'a'},
    {1, 'b'},
    {2, 'c'},
    {3, 'd'},
    {4, 'e'},
    {5, 'f'},
    {6, 'g'},
    {7, 'h'},

};


std::string Board::fen_encode() const {
    uint space_counter = 0;
    std::stringstream ss;
    for (uint rank = 0; rank< 8; rank++) {
        for (uint file = 0; file< 8; file++) {
            int idx = rank * 8 + file;
            if (pieces(idx).is_blank()) {
                space_counter++;
                continue;
            } 
            if (space_counter > 0){
                // Found a piece after a little space.
                ss << space_counter;
                space_counter = 0;
            }
            ss << fen_encode_map[pieces(idx).get_value()];
        }
        if (space_counter > 0){
            // Found a piece after a little space.
            ss << space_counter;
            space_counter = 0;
        }
        if (rank < 7) {
            ss << "/";
        }
    }

    ss << " ";
    ss << (whos_move == Colour::WHITE ? "w" : "b");
    ss << " ";
    
    if (aux_info.castling_rights[WHITE][KINGSIDE]) {
        ss << "K";
    }
    if (aux_info.castling_rights[WHITE][QUEENSIDE]) {
        ss << "Q";
    }
    if (aux_info.castling_rights[BLACK][KINGSIDE]) {
        ss << "k";
    }
    if (aux_info.castling_rights[BLACK][QUEENSIDE]) {
        ss << "q";
    }
    
    if (!(can_castle(WHITE) | can_castle(BLACK))) {
        ss << "-";
    }
    ss << " ";
    // En passent
    if (aux_info.en_passent_target == Square(0)) {
        ss << "-";
    } else {
        ss << aux_info.en_passent_target.pretty_print();
    }

    ss << " " << aux_info.halfmove_clock<< " " << fullmove_counter;
    std::string fen, tmp;
    while(ss >> tmp) {
        fen = fen + tmp + " ";
    }
    return fen;
}

void Board::pretty() const{
    for (uint rank = 0; rank< 8; rank++) {
        for (uint file = 0; file< 8; file++) {
            Square::square_t idx = 8*rank +file;
            if (idx == aux_info.en_passent_target & aux_info.en_passent_target.get_value() != 0 & pieces(idx).is_blank()){
                std::cout << "! ";
            } else {
                std::cout << pieces(idx).pretty_print();
            }
            std::cout << " ";
        }
        if (rank == 0 & whos_move == Colour::BLACK){
            std::cout << "  ***";
        }
        if (rank == 7 & whos_move == Colour::WHITE){
            std::cout << "  ***";
        }
        if (rank == 1){
            std::cout << "  ";
            if (aux_info.castling_rights[BLACK][KINGSIDE]) { std::cout << 'k'; }
            if (aux_info.castling_rights[BLACK][QUEENSIDE]) { std::cout << 'q'; }
        }
        if (rank == 6){
            std::cout << "  ";
            if (aux_info.castling_rights[WHITE][KINGSIDE]) { std::cout << 'K'; }
            if (aux_info.castling_rights[WHITE][QUEENSIDE]) { std::cout << 'Q'; }
        }
        if (rank == 3){
            std::cout << "  " << std::setw(3) << std::setfill(' ') << aux_info.halfmove_clock;
            
        }
        if (rank == 4){
            std::cout << "  " << std::setw(3) << std::setfill(' ') << fullmove_counter;
            
        }
        std::cout << std::endl;
    }
    std::cout << fen_encode() << std::endl;
    std::cout << std::hex << hash() << std::endl;
};


std::string Board::print_move(Move move, std::vector<Move> &legal_moves){
    Piece moving_piece = pieces(move.origin);
    bool ambiguity_flag = false;
    std::string notation;
    // Pawn captures are special.
    if (moving_piece.is_pawn() & move.is_capture()) {
        notation = std::string(1, file_encode_map[move.origin.file_index()]) + "x" + move.target.pretty_print();
        if (move.is_knight_promotion()) {
            notation = notation + "=N";
        } else if (move.is_bishop_promotion()) {
            notation = notation + "=B";
        } else if (move.is_rook_promotion()) {
            notation = notation + "=R";
        } else if (move.is_queen_promotion()) {
            notation = notation + "=Q";
        } 
    } else {
        // Castles are special
        if (move.is_king_castle()) { notation = "O-O"; }
        else if (move.is_king_castle()) { notation = "O-O-O"; }
        for (Move a_move : legal_moves) {
            // Ignore moves targeting somewhere else.
            if (move.target != a_move.target) {continue;}
            // Ignore this move when we find it.
            if (move.origin == a_move.origin) {continue;}
            // Check for ambiguity
            if (moving_piece.is_piece(pieces(a_move.origin))) { ambiguity_flag = true; } 
        }
        if (ambiguity_flag) {
            // This is ambiguous, use full disambiguation for now.
            notation = move.origin.pretty_print() + moving_piece.get_algebraic_character();
        }else {
            // Unambiguous move
            notation = moving_piece.get_algebraic_character();
        };
        if (move.is_capture()) {
            notation = notation + "x" + move.target.pretty_print();
        } else {
            notation = notation + move.target.pretty_print();
        }
    }
    // To check for mate
    make_move(move);
    bool can_not_move = (get_moves().size() == 0);
    bool now_is_check = aux_info.is_check;
    bool now_whos_move = whos_move;
    unmake_move(move);
    if (can_not_move) {
        // This could be a stalemate or a checkmate.
        if (now_is_check) {
            // Checkmate
            notation = notation + "# ";
            if (now_whos_move == Colour::WHITE) {
                notation = notation + "1-0";
            } else {
                notation = notation + "0-1";
            }
        } else {
            notation = notation + "½–½";
        }
    } else {
        if (now_is_check) {
            // Checkmate
            notation = notation + "+";
        }
    }
    return notation;
}


// Square

Square::Square(const std::string rf) {
// eg convert "e4" to (3, 4) to 28
    if (rf.length() != 2) {
        throw std::domain_error("Coordinate length != 2");
    }
    
    uint rank = 7 - (toDigit(rf[1]) - 1);
    uint file = file_decode_map[rf[0]];

    value = 8 * rank + file;
}


std::string Square::pretty_print() const{
    return file_encode_map[file_index()] + std::to_string(8 - rank_index());
};

std::ostream& operator<<(std::ostream& os, const Square move) {
        os << move.pretty_print();
        return os;
    }


std::ostream& operator<<(std::ostream& os, const Move move) {
    os << move.pretty_print();
    return os;
}