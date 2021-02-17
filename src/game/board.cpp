#include <map>
#include <iostream>
#include <istream>
#include <sstream>
#include <iomanip>
#include <cstdint> //for uint8_t
#include <stdexcept>

#include "board.hpp"

#define toDigit(c) ( c - '0')

std::map<char, Piece> fen_decode_map = {
    {'p', Piece::Black | Piece::Pawn},
    {'n', Piece::Black | Piece::Knight},
    {'b', Piece::Black | Piece::Bishop},
    {'r', Piece::Black | Piece::Rook},
    {'q', Piece::Black | Piece::Queen},
    {'k', Piece::Black | Piece::King},
    {'P', Piece::White | Piece::Pawn},
    {'N', Piece::White | Piece::Knight},
    {'B', Piece::White | Piece::Bishop},
    {'R', Piece::White | Piece::Rook},
    {'Q', Piece::White | Piece::Queen},
    {'K', Piece::White | Piece::King}
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
    return file_encode_map[file()] + std::to_string(8 - rank());
};

std::ostream& operator<<(std::ostream& os, const Square move) {
        os << move.pretty_print();
        return os;
    }
Square::square_t Square::squares_to_north() const{
    // Number of squares between this square and the north edge
    return rank();
}
Square::square_t Square::squares_to_south() const{
    // Number of squares between this square and the south edge
    return 7-rank();
}
Square::square_t Square::squares_to_east() const{
    // Number of squares between this square and the south edge
    return 7-file();
}
Square::square_t Square::squares_to_west() const{
    // Number of squares between this square and the south edge
    return file();
}

std::vector<Square> Square::knight_moves() const{
    std::vector<Square> moves;
    if (Square::squares_to_north() >= 2){
        if(Square::squares_to_west() >= 1) {
            moves.push_back(*this + Square::NNW);
        }
        if(Square::squares_to_east() >= 1) {
            moves.push_back(*this + Square::NNE);
        }
    }
    if (Square::squares_to_east() >= 2){
        if(Square::squares_to_north() >= 1) {
            moves.push_back(*this + Square::ENE);
        }
        if(Square::squares_to_south() >= 1) {
            moves.push_back(*this + Square::ESE);
        }
    }
    if (Square::squares_to_south() >= 2){
        if(Square::squares_to_east() >= 1) {
            moves.push_back(*this + Square::SSE);
        }
        if(Square::squares_to_west() >= 1) {
            moves.push_back(*this + Square::SSW);
        }
    }
    if (Square::squares_to_west() >= 2){
        if(Square::squares_to_south() >= 1) {
            moves.push_back(*this + Square::WSW);
        }
        if(Square::squares_to_north() >= 1) {
            moves.push_back(*this + Square::WNW);
        }
    }

    return moves;
}

std::ostream& operator<<(std::ostream& os, const Move move) {
    os << move.pretty_print();
    return os;
}

void Board::fen_decode(const std::string& fen){
    uint N = fen.length(), board_position;
    uint rank = 0, file = 0;
    char my_char;
    
    std::stringstream stream;
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
    int halfmove, counter;
    stream = std::stringstream(fen.substr(board_position));
    stream >> std::ws >> side_to_move;
    stream >> std::ws >> castling;
    stream >> std::ws >> en_passent;
    stream >> std::ws >> halfmove;
    stream >> std::ws >> counter;

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

    castle_black_kingside = false;
    castle_black_queenside = false;
    castle_white_kingside = false;
    castle_white_queenside = false;
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
                castle_black_queenside = true;
                break;

            case 'Q':
                castle_white_queenside = true;
                break;

            case 'k':
                castle_black_kingside = true;
                break;

            case 'K':
                castle_white_kingside = true;
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
        en_passent_target = 0;
    } else {
        en_passent_target = Square(en_passent);
    }

    // Halfmove clock
    halfmove_clock = halfmove;
    // Fullmove counter
    fullmove_counter = counter;
    
};

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
            if (idx == en_passent_target & pieces[idx] == Piece::Blank & en_passent_target.get_value() != 0){
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
            if (idx == en_passent_target & en_passent_target.get_value() != 0 & pieces[idx] == Piece::Blank){
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
            if (castle_black_kingside) { std::cout << 'k'; }
            if (castle_black_queenside) { std::cout << 'q'; }
        }
        if (rank == 6){
            std::cout << "  ";
            if (castle_white_kingside) { std::cout << 'K'; }
            if (castle_white_queenside) { std::cout << 'Q'; }
        }
        if (rank == 3){
            std::cout << "  " << std::setw(3) << std::setfill(' ') << halfmove_clock;
            
        }
        if (rank == 4){
            std::cout << "  " << std::setw(3) << std::setfill(' ') << fullmove_counter;
            
        }
        std::cout << std::endl;
    }
};

std::vector<Move> Board::get_pawn_moves(const Square origin, const Piece colour) const{
    std::vector<Move> moves;
    Square target;
    Move move;
    if (colour.is_colour(Piece::White)) {
        // Normal moves are N
        // Check that we are on the 7th rank or less
        if (origin.rank() > 0) 
        {
            target = origin + (Square::N);
            if (pieces[target].is_piece(Piece::Blank)) {
                move = Move(origin, target);
                moves.push_back(move);
            }
            moves.push_back(Move(origin, origin + Square::N));
            // Look for double pawn push possibility
            if (origin.rank() == 6) {
                target = origin + (Square::N + Square::N);
                if (pieces[target].is_piece(Piece::Blank) & pieces[origin + Square::N].is_piece(Piece::Blank)) {
                    move = Move(origin, target);
                    move.make_double_push();
                    moves.push_back(move);
                }
            }
            // Look for en-passent possibility
            if (origin.rank() == 3) {
                if (en_passent_target.get_value() != 0) {
                    if (en_passent_target.file() == origin.file() + 1 | en_passent_target.file() == origin.file() - 1) {
                        move = Move(origin, en_passent_target); 
                        move.make_en_passent();
                        moves.push_back(move);
                    }
                }
            }

        } else
        {
            target = origin + (Square::S);
            if (pieces[target].is_piece(Piece::Blank)) {
                Move move = Move(origin, target);
                moves.push_back(move);
            }
            moves.push_back(Move(origin, origin + Square::S));
            // Look for double pawn push possibility
            if (origin.rank() == 1) {
                target = origin + (Square::S + Square::S);
                if (pieces[target].is_piece(Piece::Blank) & pieces[origin + Square::S].is_piece(Piece::Blank)) {
                    Move move = Move(origin, target);
                    move.make_double_push();
                    moves.push_back(move);
                }
            }
            // Look for en-passent possibility
            if (origin.rank() == 4) {
                if (en_passent_target.get_value() != 0) {
                    if (en_passent_target.file() == origin.file() + 1 | en_passent_target.file() == origin.file() - 1) {
                        Move move = Move(origin, en_passent_target); 
                        move.make_en_passent();
                        moves.push_back(move);
                    }
                }
            }
        }
    }
    return moves;
}
void Board::get_moves() const {
    Piece colour;
    if (whos_move == white_move) {
        colour = Piece::White;
    } else {
        colour = Piece::Black;
    }

    Piece piece;
    std::vector<Square> targets;
    std::vector<Move> moves;
    for (Square::square_t i = 0; i < 64; i++) {
        Square square = Square(i);
        piece = pieces[square];
        if (not piece.is_colour(colour)) {continue; }
        if (piece.is_piece(Piece::Knight)) {
            targets = square.knight_moves();
            for (Square target : targets) {
                // We can move to somewhere only if that square doesn't have a piece of our colour.
                if (not pieces[target].is_colour(colour)) {
                    Move move = Move(square, target);
                    if (pieces[target].is_colour(~colour)) {
                        move.make_capture();
                    }
                    moves.push_back(move);
                }
            }
        } else if (piece.is_piece(Piece::Pawn)) {
            std::vector<Move> pawn_moves = get_pawn_moves(square, colour);
            moves.insert(moves.end(), pawn_moves.begin(), pawn_moves.end());
        }
    }

    std::cout << "Pseudolegal moves:" << std::endl;
    for (Move move: moves) {
        std::cout << move << std::endl;
    }
}