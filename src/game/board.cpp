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

const Square Square::N = -8;
const Square Square::E =  1;
const Square Square::S =  8;
const Square Square::W = -1;
const Square Square::NW = N + W;
const Square Square::NE = N + E;
const Square Square::SE = S + E;
const Square Square::SW = S + W;
const Square Square::NNW = N + N + W;
const Square Square::NNE = N + N + E;
const Square Square::ENE = E + N + E;
const Square Square::ESE = E + S + E;
const Square Square::SSE = S + S + E;
const Square Square::SSW = S + S + W;
const Square Square::WSW = W + S + W;
const Square Square::WNW = W + N + W;


const Square Square::Rank1 = 7 * 8;
const Square Square::Rank2 = 6 * 8;
const Square Square::Rank3 = 5 * 8;
const Square Square::Rank4 = 4 * 8;
const Square Square::Rank5 = 3 * 8;
const Square Square::Rank6 = 2 * 8;
const Square Square::Rank7 = 1 * 8;
const Square Square::Rank8 = 0 * 8;

const Square Square::FileA = 0 ;
const Square Square::FileB = 1;
const Square Square::FileC = 2;
const Square Square::FileD = 3;
const Square Square::FileE = 4;
const Square Square::FileF = 5;
const Square Square::FileG = 6;
const Square Square::FileH = 7;

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
Square::square_t Square::squares_to_north() const{
    // Number of squares between this square and the north edge
    return rank_index();
}
Square::square_t Square::squares_to_south() const{
    // Number of squares between this square and the south edge
    return 7-rank_index();
}
Square::square_t Square::squares_to_east() const{
    // Number of squares between this square and the south edge
    return 7-file_index();
}
Square::square_t Square::squares_to_west() const{
    // Number of squares between this square and the south edge
    return file_index();
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

void Board::get_pawn_moves(const Square origin, const Piece colour, std::vector<Move> &moves) const{
    Square target;
    Move move;
    if (colour.is_colour(Piece::White)) {
        // Normal moves are N
        // Check that we are on the 7th rank or less
        if (origin.rank() != Square::Rank7) 
        {
            target = origin + (Square::N);
            if (pieces[target].is_piece(Piece::Blank)) {
                move = Move(origin, target);
                moves.push_back(move);
            }
            // Look for double pawn push possibility
            if (origin.rank() == Square::Rank2) {
                target = origin + (Square::N + Square::N);
                if (pieces[target].is_piece(Piece::Blank) & pieces[origin + Square::N].is_piece(Piece::Blank)) {
                    move = Move(origin, target);
                    move.make_double_push();
                    moves.push_back(move);
                }
            }
            // Look for en-passent possibility
            if (origin.rank() == Square::Rank5) {
                if (en_passent_target.get_value() != 0) {
                    if (en_passent_target.file() == origin.file() + Square::W | en_passent_target.file() == origin.file() + Square::E) {
                        move = Move(origin, en_passent_target); 
                        move.make_en_passent();
                        moves.push_back(move);
                    }
                }
            }

        }
    } else
    {
        if (origin.rank() != Square::Rank1) 
        {
            target = origin + (Square::S);
            if (pieces[target].is_piece(Piece::Blank)) {
                Move move = Move(origin, target);
                moves.push_back(move);
            }
            // Look for double pawn push possibility
            if (origin.rank() == Square::Rank2) {
                target = origin + (Square::S + Square::S);
                if (pieces[target].is_piece(Piece::Blank) & pieces[origin + Square::S].is_piece(Piece::Blank)) {
                    Move move = Move(origin, target);
                    move.make_double_push();
                    moves.push_back(move);
                }
            }
            // Look for en-passent possibility
            if (origin.rank() == Square::Rank4) {
                if (en_passent_target.get_value() != 0) {
                    if (en_passent_target.file() == origin.file() + Square::W | en_passent_target.file() == origin.file() + Square::E) {
                        Move move = Move(origin, en_passent_target); 
                        move.make_en_passent();
                        moves.push_back(move);
                    }
                }
            }
        }
    }
}

void Board::get_sliding_moves(const Square origin, const Piece colour, const Square direction, const uint to_edge, std::vector<Move> &moves) const {
    Square target = origin;
    Move move;
    for (uint i = 0; i < to_edge; i++) {
        target = target + direction;
        if (pieces[target].is_piece(Piece::Blank)) {
            // Blank Square
            moves.push_back(Move(origin, target));
            continue;
        } else if (pieces[target].is_colour(~colour)) {
            // Enemy piece
            move = Move(origin, target);
            move.make_capture();
            moves.push_back(move);
            return;
        } else if (pieces[target].is_colour(colour)) {
            // Our piece, no more legal moves.
            return;
        }
    };
}

void Board::get_rook_moves(const Square origin, const Piece colour, std::vector<Move> &moves) const {
    get_sliding_moves(origin, colour, Square::N, origin.squares_to_north(), moves);
    get_sliding_moves(origin, colour, Square::E, origin.squares_to_east(), moves);
    get_sliding_moves(origin, colour, Square::S, origin.squares_to_south(), moves);
    get_sliding_moves(origin, colour, Square::W, origin.squares_to_west(), moves);
    
}

void Board::get_bishop_moves(const Square origin, const Piece colour, std::vector<Move> &moves) const {
    get_sliding_moves(origin, colour, Square::NE, std::min(origin.squares_to_north(), origin.squares_to_east()), moves);
    get_sliding_moves(origin, colour, Square::SE, std::min(origin.squares_to_east(), origin.squares_to_south()), moves);
    get_sliding_moves(origin, colour, Square::SW, std::min(origin.squares_to_south(), origin.squares_to_west()), moves);
    get_sliding_moves(origin, colour, Square::NW, std::min(origin.squares_to_west(), origin.squares_to_north()), moves);
    
}

void Board::get_queen_moves(const Square origin, const Piece colour, std::vector<Move> &moves) const {
    // Queen moves are the union superset of rook and bishop moves
    get_bishop_moves(origin,colour, moves);
    get_rook_moves(origin, colour, moves);
}
void Board::get_step_moves(const Square origin, const Square target, const Piece colour, std::vector<Move> &moves) const {
    Move move;
    if (pieces[target].is_colour(colour)) {
        // Piece on target is our colour.
        return;
    } else if (pieces[target].is_colour(~colour)) {
        //Piece on target is their colour.
        move = Move(origin, target);
        move.make_capture();
        moves.push_back(move);
        return;
    } else {
        // Space is blank.
        move = Move(origin, target);
        moves.push_back(move);
        return;
    }
}
void Board::get_king_moves(const Square origin, const Piece colour, std::vector<Move> &moves) const {
    // We should really be careful that we aren't moving into check here.
    // Look to see if we are on an edge.
    if (origin.squares_to_north() != 0) {
        get_step_moves(origin, origin + Square::N, colour, moves);
    }
    if (origin.squares_to_east() != 0) {
        get_step_moves(origin, origin + Square::E, colour, moves);
    }
    if (origin.squares_to_south() != 0) {
        get_step_moves(origin, origin + Square::S, colour, moves);
    }
    if (origin.squares_to_west() != 0) {
        get_step_moves(origin, origin + Square::W, colour, moves);
    }
    if (origin.squares_to_north() != 0 & origin.squares_to_east() != 0) {
        get_step_moves(origin, origin + Square::NE, colour, moves);
    }
    if (origin.squares_to_south() != 0 & origin.squares_to_east() != 0) {
        get_step_moves(origin, origin + Square::SE, colour, moves);
    }
    if (origin.squares_to_south() != 0 & origin.squares_to_west() != 0) {
        get_step_moves(origin, origin + Square::SW, colour, moves);
    }
    if (origin.squares_to_north() != 0 & origin.squares_to_west() != 0) {
        get_step_moves(origin, origin + Square::NW, colour, moves);
    }
}

bool Board::is_free(const Square target) const{ return pieces[target].is_piece(Piece::Blank); };

void Board::get_castle_moves(const Piece colour, std::vector<Move> &moves) const {
    // I can't check if we are moving through check yet.
    Move move;
    if (colour.is_colour(Piece::White)) {
        if (castle_white_queenside 
            & is_free(Square::FileD | Square::Rank1) 
            & is_free(Square::FileC | Square::Rank1)
            & is_free(Square::FileB | Square::Rank1)) {
            move = Move(Square::FileE | Square::Rank1, Square::FileC | Square::Rank1);
            move.make_queen_castle();
            moves.push_back(move);
        }
        if (castle_white_kingside 
            & is_free(Square::FileF | Square::Rank1) 
            & is_free(Square::FileG | Square::Rank1)) {
            move = Move(Square::FileE | Square::Rank1, Square::FileG | Square::Rank1);
            move.make_king_castle();
            moves.push_back(move);
        }
    } else
    {
        if (castle_black_queenside 
            & is_free(Square::FileD | Square::Rank8) 
            & is_free(Square::FileC | Square::Rank8)
            & is_free(Square::FileB | Square::Rank8)) {
            move = Move(Square::FileE | Square::Rank8, Square::FileC | Square::Rank8);
            move.make_queen_castle();
            moves.push_back(move);
        }
        if (castle_black_kingside 
            & is_free(Square::FileF | Square::Rank8) 
            & is_free(Square::FileG | Square::Rank8)) {
            move = Move(Square::FileE | Square::Rank8, Square::FileG | Square::Rank8);
            move.make_king_castle();
            moves.push_back(move);
        }
    }
    
}

void Board::get_knight_moves(const Square origin, const Piece colour, std::vector<Move> &moves) const {
    for (Square target : origin.knight_moves()) {
        get_step_moves(origin, target, colour, moves);
    }
};

std::vector<Move> Board::get_moves() const {
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
            get_knight_moves(square, colour, moves);
        } else if (piece.is_piece(Piece::Pawn)) {
            get_pawn_moves(square, colour, moves);
        } else if (piece.is_piece(Piece::Rook)) {
            get_rook_moves(square, colour, moves);
        } else if (piece.is_piece(Piece::Bishop)) {
            get_bishop_moves(square, colour, moves);
        } else if (piece.is_piece(Piece::Queen)) {
            get_queen_moves(square, colour, moves);
        } else if (piece.is_piece(Piece::King)) {
            get_king_moves(square, colour, moves);
        }
    }
    get_castle_moves(colour, moves);
    return moves;
}


std::string Board::print_move(const Move move, std::vector<Move> &legal_moves) const{
    Piece moving_piece = pieces[move.origin];
    bool ambiguity_flag = false;
    // Pawn captures are special.
    if (moving_piece.is_piece(Piece::Pawn) & move.is_capture()) {
        return std::string(1, file_encode_map[move.origin.file_index()]) + "x" + move.target.pretty_print();
    }
    // Castles are special
    if (move.is_king_castle() | move.is_queen_castle()) {
        return move.pretty_print();
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
        // This is ambiguous, use full notation for now.
        return move.pretty_print();
    }else {
        // Unambiguous move
        if (move.is_capture()) {
            return moving_piece.get_algebraic_character() + "x" + move.target.pretty_print();
        } else {
            return moving_piece.get_algebraic_character() + move.target.pretty_print();
        }
    }
}

void Board::make_move(Move &move) {
    last_move = move;
    // Iterate counters.
    if (whos_move == black_move) {fullmove_counter++;}
    move.halfmove_clock = halfmove_clock;
    if (move.is_capture() | pieces[move.origin].is_piece(Piece::Pawn)) {
        halfmove_clock = 0;
    } else {halfmove_clock++ ;}
    // Switch whos turn it is to play
    whos_move = ! whos_move;
    // Track en-passent square
    move.en_passent_target = en_passent_target;
    if (move.is_double_push()) {
        // Little hacky but this is the square in between.
        en_passent_target = (move.origin.get_value() + move.target.get_value())/2;
    } else {
        en_passent_target = 0;
    }
    // Castling is special
    if (move.is_king_castle()) {
        pieces[move.target] = pieces[move.origin];
        pieces[move.origin] = Piece::Blank;
        pieces[move.origin + Square::E] = pieces[move.origin.rank() | Square::FileH];
        pieces[move.origin.rank() | Square::FileH] = Piece::Blank;
    } else if (move.is_queen_castle()) {
        pieces[move.target] = pieces[move.origin];
        pieces[move.origin] = Piece::Blank;
        pieces[move.origin + Square::W] = pieces[move.origin.rank() | Square::FileH];
        pieces[move.origin.rank() | Square::FileA] = Piece::Blank;
    } else if(move.is_ep_capture()) {
        // En-passent is weird too.
        const Square captured_square = move.origin.rank() | move.target.file();
        // Make sure to lookup and record the piece captured 
        move.captured_peice = pieces[captured_square];
        pieces[move.target] = pieces[move.origin];
        pieces[move.origin] = Piece::Blank;
        pieces[captured_square] = Piece::Blank;
    } else if (move.is_capture()){
        // Make sure to lookup and record the piece captured 
        move.captured_peice = pieces[move.target];
        pieces[move.target] = pieces[move.origin];
        pieces[move.origin] = Piece::Blank;
    } else {
        pieces[move.target] = pieces[move.origin];
        pieces[move.origin] = Piece::Blank;
    }
}

void Board::unmake_move(const Move move) {
    // Iterate counters.
    if (whos_move == white_move) {fullmove_counter--;}
    halfmove_clock = move.halfmove_clock;
    // Switch whos turn it is to play
    whos_move = ! whos_move;
    // Track en-passent square
    en_passent_target = move.en_passent_target;
    // Castling is special
    if (move.is_king_castle()) {
        pieces[move.origin] = pieces[move.target];
        pieces[move.target] = Piece::Blank;
        pieces[move.origin.rank() | Square::FileH] = pieces[move.origin + Square::E];
        pieces[move.origin + Square::E] = Piece::Blank;
    } else if (move.is_queen_castle()) {
        pieces[move.origin] = pieces[move.target];
        pieces[move.target] = Piece::Blank;
        pieces[move.origin.rank() | Square::FileA] = pieces[move.origin + Square::W];
        pieces[move.origin + Square::W] = Piece::Blank;
    } else if(move.is_ep_capture()) {
        // En-passent is weird too.
        const Square captured_square = move.origin.rank() | move.target.file();
        // Make sure to lookup and record the piece captured 
        pieces[move.origin] = pieces[move.target];
        pieces[move.target] = Piece::Blank;
        pieces[captured_square] = move.captured_peice;
    } else if (move.is_capture()){
        // Make sure to lookup and record the piece captured 
        pieces[move.origin] = pieces[move.target];
        pieces[move.target] = move.captured_peice;
    } else {
        pieces[move.origin] = pieces[move.target];
        pieces[move.target] = Piece::Blank;
    }
}

void Board::try_move(const std::string move_sting) {
    bool flag = false;
    std::vector<Move> legal_moves = get_moves();
    for (Move move : legal_moves) {
        if (move_sting == print_move(move, legal_moves)) {
            make_move(move);
            flag = true;
            break;
        } else if (move_sting == move.pretty_print()){
            make_move(move);
            flag = true;
            break;
        }
    }
    if (!flag) {
        std::cout << "Move not found: " << move_sting << std::endl;
    }
}