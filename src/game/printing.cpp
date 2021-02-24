#include <iostream>
#include <istream>
#include <sstream>
#include <iomanip>

#include "board.hpp"

#define toDigit(c) ( c - '0')


std::map<char, Piece> fen_decode_map = {
    {'p', Pieces::Black | Pieces::Pawn},
    {'n', Pieces::Black | Pieces::Knight},
    {'b', Pieces::Black | Pieces::Bishop},
    {'r', Pieces::Black | Pieces::Rook},
    {'q', Pieces::Black | Pieces::Queen},
    {'k', Pieces::Black | Pieces::King},
    {'P', Pieces::White | Pieces::Pawn},
    {'N', Pieces::White | Pieces::Knight},
    {'B', Pieces::White | Pieces::Bishop},
    {'R', Pieces::White | Pieces::Rook},
    {'Q', Pieces::White | Pieces::Queen},
    {'K', Pieces::White | Pieces::King}
};


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



void Board::fen_decode(const std::string& fen){
    uint N = fen.length(), board_position;
    uint rank = 0, file = 0;
    char my_char;
    
    std::stringstream stream;
    // reset board
    for (uint i = 0; i < 64; i++) {
        pieces[i] = Pieces::Blank;
    }
    // First, go through the board position part of the fen string
    for (uint i = 0; i < N; i++) {
        my_char = fen[i];
        if (my_char == '/'){
            rank++;
            file = 0;
            continue;
        }
        if (isdigit(my_char)) {
            file += toDigit(my_char);
            continue;
        }
        if (my_char == ' ') {
            // Space is at the end of the board position section
            board_position = i;
            break;
        }
        // Otherwise should be a character for a piece
        pieces[Square::to_index(rank, file)] = fen_decode_map[my_char];
        file ++;
    }

    std::string side_to_move, castling, en_passent;
    int halfmove = 0, counter = 0;
    stream = std::stringstream(fen.substr(board_position));
    stream >> std::ws;
    stream >> side_to_move >> std::ws;
    stream >> castling >> std::ws;
    stream >>  en_passent >> std::ws;
    stream >> halfmove >> std::ws;
    stream >> counter >> std::ws;

    // Side to move
    if (side_to_move.length() > 1) {
        throw std::domain_error("<Side to move> length > 1");
    }
    switch (side_to_move[0])
    {
    case 'w':
        whos_move = white_move;
        break;

    case 'b':
        whos_move = black_move;
        break;
    
    default:
        throw std::domain_error("Unrecognised <Side to move> character");
    }

    aux_info.castle_black_kingside = false;
    aux_info.castle_black_queenside = false;
    aux_info.castle_white_kingside = false;
    aux_info.castle_white_queenside = false;
    // Castling rights
    if (castling.length() > 4) {
        throw std::domain_error("<Castling> length > 4");
    }
    if (castling[0] == '-') {
        // No castling rights, continue
    } else {
        for (uint i = 0; i < castling.length(); i++) {
            switch (castling[i])
            {
            case 'q':
                aux_info.castle_black_queenside = true;
                break;

            case 'Q':
                aux_info.castle_white_queenside = true;
                break;

            case 'k':
                aux_info.castle_black_kingside = true;
                break;

            case 'K':
                aux_info.castle_white_kingside = true;
                break;

            default:
                throw std::domain_error("Unrecognised <Castling> character");
            }
        }
    }

    // En passent
    if (en_passent.length() > 2) {
        throw std::domain_error("<en passent> length > 2");
    }
    if (en_passent[0] == '-') {
        // No en passent, continue
        aux_info.en_passent_target = 0;
    } else {
        aux_info.en_passent_target = Square(en_passent);
    }

    // Halfmove clock
    aux_info.halfmove_clock = halfmove;
    // Fullmove counter
    fullmove_counter = counter;
    initialise();
};

std::string Board::fen_encode() const {
    uint space_counter = 0;
    std::stringstream ss;
    for (uint rank = 0; rank< 8; rank++) {
        for (uint file = 0; file< 8; file++) {
            int idx = rank * 8 + file;
            if (pieces[idx].is_blank()) {
                space_counter++;
                continue;
            } 
            if (space_counter > 0){
                // Found a piece after a little space.
                ss << space_counter;
                space_counter = 0;
            }
            ss << fen_encode_map[pieces[idx].get_value()];
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
    ss << (whos_move == white_move ? "w" : "b");
    ss << " ";
    
    if (aux_info.castle_white_kingside) {
        ss << "K";
    }
    if (aux_info.castle_white_queenside) {
        ss << "Q";
    }
    if (aux_info.castle_black_kingside) {
        ss << "k";
    }
    if (aux_info.castle_black_queenside) {
        ss << "q";
    }
    
    if (!(aux_info.castle_black_kingside|aux_info.castle_black_queenside|aux_info.castle_white_kingside|aux_info.castle_white_queenside)) {
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

void Board::print_board_idx() {
    for (uint rank = 0; rank< 8; rank++) {
        for (uint file = 0; file< 8; file++) {
            uint idx = 8*rank +file;
            std::cout << std::setw(2) << std::setfill('0')  << idx << ' ';
        }
            std::cout << std::endl;
    }
};

void Board::print_board() {
    for (uint rank = 0; rank< 8; rank++) {
        for (uint file = 0; file< 8; file++) {
            Square::square_t idx = 8*rank +file;
            if (idx == aux_info.en_passent_target & pieces[idx].is_blank() & aux_info.en_passent_target.get_value() != 0){
                std::cout << '!';
            } else {
                std::cout << pieces[idx].pretty_print() << ' ';
            }
        }
        std::cout << std::endl;
    }
}

void Board::print_board_extra(){
    for (uint rank = 0; rank< 8; rank++) {
        for (uint file = 0; file< 8; file++) {
            Square::square_t idx = 8*rank +file;
            if (idx == aux_info.en_passent_target & aux_info.en_passent_target.get_value() != 0 & pieces[idx].is_blank()){
                std::cout << '-';
            } else {
                std::cout << pieces[idx].pretty_print();
            }
            std::cout << ' ';
        }
        if (rank == 0 & whos_move == black_move){
            std::cout << "  ***";
        }
        if (rank == 7 & whos_move == white_move){
            std::cout << "  ***";
        }
        if (rank == 1){
            std::cout << "  ";
            if (aux_info.castle_black_kingside) { std::cout << 'k'; }
            if (aux_info.castle_black_queenside) { std::cout << 'q'; }
        }
        if (rank == 6){
            std::cout << "  ";
            if (aux_info.castle_white_kingside) { std::cout << 'K'; }
            if (aux_info.castle_white_queenside) { std::cout << 'Q'; }
        }
        if (rank == 3){
            std::cout << "  " << std::setw(3) << std::setfill(' ') << aux_info.halfmove_clock;
            
        }
        if (rank == 4){
            std::cout << "  " << std::setw(3) << std::setfill(' ') << fullmove_counter;
            
        }
        std::cout << std::endl;
    }
};


std::string Board::print_move(Move move, std::vector<Move> &legal_moves){
    Piece moving_piece = pieces[move.origin];
    bool ambiguity_flag = false;
    std::string notation;
    // Pawn captures are special.
    if (moving_piece.is_piece(Pieces::Pawn) & move.is_capture()) {
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
        if (move.is_king_castle() | move.is_queen_castle()) {
            notation = move.pretty_print();
        }
        for (Move a_move : legal_moves) {
            // Ignore moves targeting somewhere else.
            if (move.target != a_move.target) {continue;}
            // Ignore this move when we find it.
            if (move.origin == a_move.origin) {continue;}
            // Check for ambiguity
            if (moving_piece.is_piece(pieces[a_move.origin])) { ambiguity_flag = true; } 
        }
        if (ambiguity_flag) {
            // This is ambiguous, use full disambiguation for now.
            notation = move.origin.pretty_print() + moving_piece.get_algebraic_character();
        }else {
            // Unambiguous move
            notation = moving_piece.get_algebraic_character();
        };
    }
    if (move.is_capture()) {
        notation = notation + "x" + move.target.pretty_print();
    } else {
        notation = notation + move.target.pretty_print();
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
            if (now_whos_move == white_move) {
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

    value = rank + 8 * file;
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
