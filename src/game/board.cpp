#include <map>
#include <iostream>
#include <istream>
#include <sstream>
#include <iomanip>
#include <cstdint> //for uint8_t
#include <stdexcept>

#include "board.hpp"

#define toDigit(c) ( c - '0')

std::map<char, uint8_t> fen_decode_map = {
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

std::map<char, uint> file_decode_map = {
    {'a', 0},
    {'b', 1},
    {'c', 2},
    {'d', 3},
    {'e', 4},
    {'f', 5},
    {'g', 6},
    {'h', 7},

};

std::map<uint, char> file_encode_map = {
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
        pieces[to_index(rank, file)] = fen_decode_map[my_char];
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
        en_passent_target = str_to_index(en_passent);
    }

    // Halfmove clock
    halfmove_clock = halfmove;
    // Fullmove counter
    fullmove_counter = counter;
    
};

uint Board::str_to_index(const std::string& rf){
    // eg convert "e4" to (3, 4) to 28
    if (rf.length() != 2) {
        throw std::domain_error("Coordinate length != 2");
    }
    
    uint rank = 7 - (toDigit(rf[1]) - 1);
    uint file = file_decode_map[rf[0]];

    return to_index(rank, file);
}

std::string Board::idx_to_str(Board::coord idx) {
    // In our convention
    coord rank = idx / 8;
    coord file = idx % 8;
    return file_encode_map[file] + std::to_string(8 - rank);
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
            coord idx = 8*rank +file;
            if (idx == en_passent_target & pieces[idx] == Piece::Blank & en_passent_target != 0){
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
            coord idx = 8*rank +file;
            if (idx == en_passent_target & en_passent_target != 0 & pieces[idx] == Piece::Blank){
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

Board::coord Board::squares_to_north(Board::coord idx) {
    // Number of squares between this square and the north edge
    coord rank = idx / 8;
    return rank;
}
Board::coord Board::squares_to_south(Board::coord idx) {
    // Number of squares between this square and the south edge
    coord rank = idx / 8;
    return 7-rank;
}
Board::coord Board::squares_to_east(Board::coord idx) {
    // Number of squares between this square and the south edge
    coord file = idx % 8;
    return 7-file;
}
Board::coord Board::squares_to_west(Board::coord idx) {
    // Number of squares between this square and the south edge
    coord file = idx % 8;
    return file;
}

std::vector<Board::coord> possible_knight_moves(const Board::coord starting_point) {
    std::vector<Board::coord> moves;
    if (Board::squares_to_north(starting_point) >= 2){
        if(Board::squares_to_west(starting_point) >= 1) {
            moves.push_back(starting_point + Board::NNW);
        }
        if(Board::squares_to_east(starting_point) >= 1) {
            moves.push_back(starting_point + Board::NNE);
        }
    }
    if (Board::squares_to_east(starting_point) >= 2){
        if(Board::squares_to_north(starting_point) >= 1) {
            moves.push_back(starting_point + Board::ENE);
        }
        if(Board::squares_to_south(starting_point) >= 1) {
            moves.push_back(starting_point + Board::ESE);
        }
    }
    if (Board::squares_to_south(starting_point) >= 2){
        if(Board::squares_to_east(starting_point) >= 1) {
            moves.push_back(starting_point + Board::SSE);
        }
        if(Board::squares_to_west(starting_point) >= 1) {
            moves.push_back(starting_point + Board::SSW);
        }
    }
    if (Board::squares_to_west(starting_point) >= 2){
        if(Board::squares_to_south(starting_point) >= 1) {
            moves.push_back(starting_point + Board::WSW);
        }
        if(Board::squares_to_north(starting_point) >= 1) {
            moves.push_back(starting_point + Board::WNW);
        }
    }

    return moves;
}

std::vector<Board::coord> possible_rook_moves(const Board::coord starting_point, Piece piece) {
    const Piece colour = piece.get_colour();
    
}