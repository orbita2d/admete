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

constexpr Square forwards(const Piece colour) {
    if (colour.is_white()) {
        return Squares::N;
    } else {
        return Squares::S;
    }
}

constexpr Square back_rank(const Piece colour) {
    if (colour.is_white()) {
        return Squares::Rank1;
    } else {
        return Squares::Rank8;
    }
}

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

std::array<KnightMoveArray, 64> GenerateKnightMoves(){
    std::array<KnightMoveArray, 64> meta_array;
    KnightMoveArray moves;
    for (unsigned int i = 0; i < 64; i++){
        moves = KnightMoveArray();
        Square origin = Square(i);
        if (origin.to_north() >= 2){
            if(origin.to_west() >= 1) {
                moves.push_back(origin + Squares::NNW);
            }
            if(origin.to_east() >= 1) {
                moves.push_back(origin + Squares::NNE);
            }
        }
        if (origin.to_east() >= 2){
            if(origin.to_north() >= 1) {
                moves.push_back(origin + Squares::ENE);
            }
            if(origin.to_south() >= 1) {
                moves.push_back(origin + Squares::ESE);
            }
        }
        if (origin.to_south() >= 2){
            if(origin.to_east() >= 1) {
                moves.push_back(origin + Squares::SSE);
            }
            if(origin.to_west() >= 1) {
                moves.push_back(origin + Squares::SSW);
            }
        }
        if (origin.to_west() >= 2){
            if(origin.to_south() >= 1) {
                moves.push_back(origin + Squares::WSW);
            }
            if(origin.to_north() >= 1) {
                moves.push_back(origin + Squares::WNW);
            }
        }
        meta_array[i] = moves;
    }
    return meta_array;
}

std::array<KnightMoveArray, 64> knight_meta_array = GenerateKnightMoves();

KnightMoveArray knight_moves(const Square origin){
    return knight_meta_array[origin];
}

std::ostream& operator<<(std::ostream& os, const Move move) {
    os << move.pretty_print();
    return os;
}

void Board::initialise() {
    build_occupied_bb();
    search_kings();
    update_checkers();
    search_pins(white_move);
    search_pins(black_move);
}

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

void add_pawn_promotions(const Move move, std::vector<Move> &moves) {
    // Add all variations of promotions to a move.
    Move my_move = move;
    my_move.make_knight_promotion();
    moves.push_back(my_move);
    my_move.make_bishop_promotion();
    moves.push_back(my_move);
    my_move.make_rook_promotion();
    moves.push_back(my_move);
    my_move.make_queen_promotion();
    moves.push_back(my_move);
}

void Board::get_pawn_moves(const Square origin, const Piece colour, std::vector<Move> &moves) const{
    Square target;
    Move move;
    if (colour.is_white()) {
        // Moves are North
        // Normal pushes.
        target = origin + (Squares::N);
        if (pieces[target].is_piece(Pieces::Blank)) {
            move = Move(origin, target);
            if (origin.rank() == Squares::Rank7) {
                add_pawn_promotions(move, moves);
            } else {
                moves.push_back(move);
            }
        }
        // Normal captures.
        if (origin.to_west() != 0) {
            target = origin + (Squares::NW);
            if (pieces[target].is_colour(~colour)) {
                move = Move(origin, target);
                move.make_capture();
                if (origin.rank() == Squares::Rank7) {
                    add_pawn_promotions(move, moves);
                } else {
                    moves.push_back(move);
                }
            }
        }

        if (origin.to_east() != 0) {
            target = origin + (Squares::NE);
            if (pieces[target].is_colour(~colour)) {
                move = Move(origin, target);
                move.make_capture();
                if (origin.rank() == Squares::Rank7) {
                    add_pawn_promotions(move, moves);
                } else {
                    moves.push_back(move);
                }
            }
        }
        // En-passent
        if (origin.rank() == Squares::Rank5) {
            if (origin.to_west() != 0 & aux_info.en_passent_target == origin + Squares::NW | 
                origin.to_east() != 0 & aux_info.en_passent_target == origin + Squares::NE ) {
                move = Move(origin, aux_info.en_passent_target); 
                move.make_en_passent();
                moves.push_back(move);
            }
        }

        // Look for double pawn push possibility
        if (origin.rank() == Squares::Rank2) {
            target = origin + (Squares::N + Squares::N);
            if (pieces[target].is_piece(Pieces::Blank) & pieces[origin + Squares::N].is_piece(Pieces::Blank)) {
                move = Move(origin, target);
                move.make_double_push();
                moves.push_back(move);
            }
        }
    } else
    {
        // Moves are South
        // Normal pushes.
        target = origin + (Squares::S);
        if (pieces[target].is_piece(Pieces::Blank)) {
            move = Move(origin, target);
            if (origin.rank() == Squares::Rank2) {
                add_pawn_promotions(move, moves);
            } else {
                moves.push_back(move);
            }
        }
        // Normal captures.
        if (origin.to_west() != 0) {
            target = origin + (Squares::SW);
            if (pieces[target].is_colour(~colour)) {
                move = Move(origin, target);
                move.make_capture();
                if (origin.rank() == Squares::Rank2) {
                    add_pawn_promotions(move, moves);
                } else {
                    moves.push_back(move);
                }
            }
        }

        if (origin.to_east() != 0) {
            target = origin + (Squares::SE);
            if (pieces[target].is_colour(~colour)) {
                move = Move(origin, target);
                move.make_capture();
                if (origin.rank() == Squares::Rank2) {
                    add_pawn_promotions(move, moves);
                } else {
                    moves.push_back(move);
                }
            }
        }
        // En-passent
        if (origin.rank() == Squares::Rank4) {
            if (origin.to_west() != 0 & aux_info.en_passent_target == origin + Squares::SW | 
                origin.to_east() != 0 & aux_info.en_passent_target == origin + Squares::SE ) {
                move = Move(origin, aux_info.en_passent_target); 
                move.make_en_passent();
                moves.push_back(move);
            }
        }

        // Look for double pawn push possibility
        if (origin.rank() == Squares::Rank7) {
            target = origin + (Squares::S + Squares::S);
            if (pieces[target].is_piece(Pieces::Blank) & pieces[origin + Squares::S].is_piece(Pieces::Blank)) {
                move = Move(origin, target);
                move.make_double_push();
                moves.push_back(move);
            }
        }
    }
}

void Board::get_sliding_moves(const Square origin, const Piece colour, const Square direction, const uint to_edge, std::vector<Move> &moves) const {
    Square target = origin;
    Move move;
    for (uint i = 0; i < to_edge; i++) {
        target = target + direction;
        if (pieces[target].is_piece(Pieces::Blank)) {
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
    get_sliding_moves(origin, colour, Squares::N, origin.to_north(), moves);
    get_sliding_moves(origin, colour, Squares::E, origin.to_east(), moves);
    get_sliding_moves(origin, colour, Squares::S, origin.to_south(), moves);
    get_sliding_moves(origin, colour, Squares::W, origin.to_west(), moves);
    
}

void Board::get_bishop_moves(const Square origin, const Piece colour, std::vector<Move> &moves) const {
    get_sliding_moves(origin, colour, Squares::NE, std::min(origin.to_north(), origin.to_east()), moves);
    get_sliding_moves(origin, colour, Squares::SE, std::min(origin.to_east(), origin.to_south()), moves);
    get_sliding_moves(origin, colour, Squares::SW, std::min(origin.to_south(), origin.to_west()), moves);
    get_sliding_moves(origin, colour, Squares::NW, std::min(origin.to_west(), origin.to_north()), moves);
    
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
    if (origin.to_north() != 0) {
        get_step_moves(origin, origin + Squares::N, colour, moves);
    }
    if (origin.to_east() != 0) {
        get_step_moves(origin, origin + Squares::E, colour, moves);
    }
    if (origin.to_south() != 0) {
        get_step_moves(origin, origin + Squares::S, colour, moves);
    }
    if (origin.to_west() != 0) {
        get_step_moves(origin, origin + Squares::W, colour, moves);
    }
    if (origin.to_north() != 0 & origin.to_east() != 0) {
        get_step_moves(origin, origin + Squares::NE, colour, moves);
    }
    if (origin.to_south() != 0 & origin.to_east() != 0) {
        get_step_moves(origin, origin + Squares::SE, colour, moves);
    }
    if (origin.to_south() != 0 & origin.to_west() != 0) {
        get_step_moves(origin, origin + Squares::SW, colour, moves);
    }
    if (origin.to_north() != 0 & origin.to_west() != 0) {
        get_step_moves(origin, origin + Squares::NW, colour, moves);
    }
}

void Board::build_occupied_bb() {
    bitboard temp_occupied = 0;
    for (uint i = 0; i < 64; i++) {
        if (!pieces[i].is_blank()) {
            temp_occupied |= (uint64_t(1)<< i);
        }
    }
    occupied = temp_occupied;
}

void Board::print_bitboard(const bitboard bb) {
    for (uint rank = 0; rank< 8; rank++) {
        for (uint file = 0; file< 8; file++) {
            uint idx = 8*rank +file;
            std::cout << ((bb >> idx) & 1) << ' ';
        }
        std::cout << std::endl;
    }
}

bool Board::is_free(const Square target) const{  
    return ((occupied >> target) % 2) == 0 ;
};

void Board::get_castle_moves(const Piece colour, std::vector<Move> &moves) const {
    Move move;
    if (colour.is_white()) {
        // You can't castle through check, or while in check
        if (is_check(Squares::FileE | Squares::Rank1, colour)) {return; }
        if (aux_info.castle_white_queenside 
            & is_free(Squares::FileD | Squares::Rank1) 
            & is_free(Squares::FileC | Squares::Rank1)
            & is_free(Squares::FileB | Squares::Rank1)
            & !is_check(Squares::FileD | Squares::Rank1, colour)) {
            move = Move(Squares::FileE | Squares::Rank1, Squares::FileC | Squares::Rank1);
            move.make_queen_castle();
            moves.push_back(move);
        }
        if (aux_info.castle_white_kingside 
            & is_free(Squares::FileF | Squares::Rank1) 
            & is_free(Squares::FileG | Squares::Rank1)
            & !is_check(Squares::FileF | Squares::Rank1, colour)
            & !is_check(Squares::FileE | Squares::Rank1, colour)) {
            move = Move(Squares::FileE | Squares::Rank1, Squares::FileG | Squares::Rank1);
            move.make_king_castle();
            moves.push_back(move);
        }
    } else
    {
        if (is_check(Squares::FileE | Squares::Rank8, colour)) {return; }
        if (aux_info.castle_black_queenside 
            & is_free(Squares::FileD | Squares::Rank8) 
            & is_free(Squares::FileC | Squares::Rank8)
            & is_free(Squares::FileB | Squares::Rank8)
            & !is_check(Squares::FileD | Squares::Rank8, colour)) {
            move = Move(Squares::FileE | Squares::Rank8, Squares::FileC | Squares::Rank8);
            move.make_queen_castle();
            moves.push_back(move);
        }
        if (aux_info.castle_black_kingside 
            & is_free(Squares::FileF | Squares::Rank8) 
            & is_free(Squares::FileG | Squares::Rank8)
            & !is_check(Squares::FileF | Squares::Rank8, colour)) {
            move = Move(Squares::FileE | Squares::Rank8, Squares::FileG | Squares::Rank8);
            move.make_king_castle();
            moves.push_back(move);
        }
    }
    
}

void Board::get_knight_moves(const Square origin, const Piece colour, std::vector<Move> &moves) const {
    for (Square target : knight_moves(origin)) {
        get_step_moves(origin, target, colour, moves);
    }
};

std::vector<Move> Board::get_pseudolegal_moves() const {
    Piece colour = whos_move;
    
    Piece piece;
    std::vector<Square> targets;
    std::vector<Move> moves;
    moves.reserve(256);
    for (Square::square_t i = 0; i < 64; i++) {
        Square square = Square(i);
        piece = pieces[square];
        if (! piece.is_colour(colour)) {continue; }
        if (piece.is_knight()) {
            get_knight_moves(square, colour, moves);
        } else if (piece.is_pawn()) {
            get_pawn_moves(square, colour, moves);
        } else if (piece.is_rook()) {
            get_rook_moves(square, colour, moves);
        } else if (piece.is_bishop()) {
            get_bishop_moves(square, colour, moves);
        } else if (piece.is_queen()) {
            get_queen_moves(square, colour, moves);
        } else if (piece.is_king()) {
            get_king_moves(square, colour, moves);
        }
    }
    get_castle_moves(colour, moves);
    return moves;
}


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

void Board::make_move(Move &move) {
    last_move = move;
    // Iterate counters.
    if (whos_move == black_move) {fullmove_counter++;}
    aux_history[ply_counter] = aux_info;
    if (move.is_capture() | pieces[move.origin].is_piece(Pieces::Pawn)) {
        aux_info.halfmove_clock = 0;
    } else {aux_info.halfmove_clock++ ;}
    // Track en-passent square
    if (move.is_double_push()) {
        // Little hacky but this is the square in between.
        aux_info.en_passent_target = (move.origin.get_value() + move.target.get_value())/2;
    } else {
        aux_info.en_passent_target = 0;
    }

    if (pieces[move.origin].is_king()) {
        if (whos_move == white_move) {
            king_square[0] = move.target;
        } else {
            king_square[1] = move.target;
        }
    }
    bitboard from_bb = uint64_t(1) << move.origin;
    bitboard to_bb = uint64_t(1) << move.target;
    bitboard from_to_bb;

    // Castling is special
    if (move.is_king_castle()) {
        pieces[move.target] = pieces[move.origin];
        pieces[move.origin] = Pieces::Blank;
        pieces[move.origin + Squares::E] = pieces[move.origin.rank() | Squares::FileH];
        pieces[move.origin.rank() | Squares::FileH] = Pieces::Blank;
        from_bb = from_bb ^ (uint64_t(1) << (move.origin.rank() | Squares::FileH));
        to_bb = to_bb ^ (uint64_t(1) << (move.origin.rank() | Squares::FileF));
        // Update the bitboard.
        from_to_bb = from_bb ^ to_bb;
        occupied ^= from_to_bb;
        if (whos_move == white_move) {
            aux_info.castle_white_kingside  = false;
            aux_info.castle_white_queenside = false;
        } else {
            aux_info.castle_black_kingside  = false;
            aux_info.castle_black_queenside = false;
        }
    } else if (move.is_queen_castle()) {
        pieces[move.target] = pieces[move.origin];
        pieces[move.origin] = Pieces::Blank;
        pieces[move.origin + Squares::W] = pieces[move.origin.rank() | Squares::FileA];
        pieces[move.origin.rank() | Squares::FileA] = Pieces::Blank;
        from_bb = from_bb ^ (uint64_t(1) << (move.origin.rank() | Squares::FileA));
        to_bb = to_bb ^ (uint64_t(1) << (move.origin.rank() | Squares::FileD));
        // Update the bitboard.
        from_to_bb = from_bb ^ to_bb;
        occupied ^= from_to_bb;
        if (whos_move == white_move) {
            aux_info.castle_white_kingside  = false;
            aux_info.castle_white_queenside = false;
        } else {
            aux_info.castle_black_kingside  = false;
            aux_info.castle_black_queenside = false;
        }
    } else if(move.is_ep_capture()) {
        // En-passent is weird too.
        const Square captured_square = move.origin.rank() | move.target.file();
        // Update the bitboard.
        from_to_bb = from_bb ^ to_bb;
        from_to_bb ^= (uint64_t(1) << captured_square);
        occupied ^= from_to_bb;
        // Make sure to lookup and record the piece captured 
        move.captured_peice = pieces[captured_square];
        pieces[move.target] = pieces[move.origin];
        pieces[move.origin] = Pieces::Blank;
        pieces[captured_square] = Pieces::Blank;
    } else if (move.is_capture()){
        // Update the bitboard.
        occupied ^= from_bb;
        // Make sure to lookup and record the piece captured 
        move.captured_peice = pieces[move.target];
        pieces[move.target] = pieces[move.origin];
        pieces[move.origin] = Pieces::Blank;
    } else {
        pieces[move.target] = pieces[move.origin];
        pieces[move.origin] = Pieces::Blank;
        from_to_bb = from_bb ^ to_bb;
        occupied ^= from_to_bb;
    }
    // And now do the promotion if it is one.
    if (move.is_knight_promotion()) {
        pieces[move.target] = pieces[move.target].get_colour() | Pieces::Knight;
    } else if (move.is_bishop_promotion()){
        pieces[move.target] = pieces[move.target].get_colour() | Pieces::Bishop;
    } else if (move.is_rook_promotion()){
        pieces[move.target] = pieces[move.target].get_colour() | Pieces::Rook;
    } else if (move.is_queen_promotion()){
        pieces[move.target] = pieces[move.target].get_colour() | Pieces::Queen;
    }
    
    if ((aux_info.castle_white_kingside | aux_info.castle_white_queenside) & whos_move == white_move){
        if (move.origin == (Squares::Rank1 | Squares::FileE)) {
            // Check if they've moved their King
            aux_info.castle_white_kingside  = false;
            aux_info.castle_white_queenside  = false;
        }
        if (move.origin == (Squares::FileH | Squares::Rank1)) {
            // Check if they've moved their rook.
            aux_info.castle_white_kingside  = false;
        } else if (move.origin == (Squares::FileA | Squares::Rank1)) {
            // Check if they've moved their rook.
            aux_info.castle_white_queenside  = false;
        }
    }
    if ((aux_info.castle_black_kingside | aux_info.castle_black_queenside) & whos_move == black_move){
        if (move.origin == (Squares::Rank8 | Squares::FileE)) {
            // Check if they've moved their King
            aux_info.castle_black_kingside  = false;
            aux_info.castle_black_queenside  = false;
        }
        if (move.origin == (Squares::FileH | Squares::Rank8)) {
            // Check if they've moved their rook.
            aux_info.castle_black_kingside  = false;
        } else if (move.origin == (Squares::FileA | Squares::Rank8)) {
            // Check if they've moved their rook.
            aux_info.castle_black_queenside  = false;
        }
    }

    if ((aux_info.castle_black_kingside | aux_info.castle_black_queenside) & whos_move == white_move){
        // Check for rook captures.
        if (move.target == (Squares::FileH | Squares::Rank8)) {
            aux_info.castle_black_kingside = false;
        }
        if (move.target == (Squares::FileA | Squares::Rank8)) {
            aux_info.castle_black_queenside = false;
        }
    }
    if ((aux_info.castle_white_kingside | aux_info.castle_white_queenside) & whos_move == black_move){
        // Check for rook captures.
        if (move.target == (Squares::FileH | Squares::Rank1)) {
            aux_info.castle_white_kingside = false;
        }
        if (move.target == (Squares::FileA | Squares::Rank1)) {
            aux_info.castle_white_queenside = false;
        }
    }

    search_pins(!whos_move);

    // Switch whos turn it is to play
    whos_move = ! whos_move;

    update_checkers();
    ply_counter ++;
}

void Board::unmake_move(const Move move) {
    // Iterate counters.
    if (whos_move == white_move) {fullmove_counter--;}
    ply_counter--;
    aux_info = aux_history[ply_counter];

    // Switch whos turn it is to play
    whos_move = ! whos_move;
    // Castling is special

    if (pieces[move.target].is_king()) {
        if (whos_move == white_move) {
            king_square[0] = move.origin;
        } else {
            king_square[1] = move.origin;
        }
    }

    bitboard from_bb = uint64_t(1) << move.origin;
    bitboard to_bb = uint64_t(1) << move.target;
    bitboard from_to_bb;
    if (move.is_king_castle()) {
        pieces[move.origin] = pieces[move.target];
        pieces[move.target] = Pieces::Blank;
        pieces[move.origin.rank() | Squares::FileH] = pieces[move.origin + Squares::E];
        pieces[move.origin + Squares::E] = Pieces::Blank;
        from_bb = from_bb ^ (uint64_t(1) << (move.origin.rank() | Squares::FileH));
        to_bb = to_bb ^ (uint64_t(1) << (move.origin.rank() | Squares::FileF));
        // Update the bitboard.
        from_to_bb = from_bb ^ to_bb;
        occupied ^= from_to_bb;
    } else if (move.is_queen_castle()) {
        pieces[move.origin] = pieces[move.target];
        pieces[move.target] = Pieces::Blank;
        pieces[move.origin.rank() | Squares::FileA] = pieces[move.origin + Squares::W];
        pieces[move.origin + Squares::W] = Pieces::Blank;
        from_bb = from_bb ^ (uint64_t(1) << (move.origin.rank() | Squares::FileA));
        to_bb = to_bb ^ (uint64_t(1) << (move.origin.rank() | Squares::FileD));
        // Update the bitboard.
        from_to_bb = from_bb ^ to_bb;
        occupied ^= from_to_bb;
    } else if(move.is_ep_capture()) {
        // En-passent is weird too.
        const Square captured_square = move.origin.rank() | move.target.file();
        // Make sure to lookup and record the piece captured 
        pieces[move.origin] = pieces[move.target];
        pieces[move.target] = Pieces::Blank;
        pieces[captured_square] = move.captured_peice;
        from_to_bb = from_bb ^ to_bb;
        from_to_bb ^= (uint64_t(1) << captured_square);
        occupied ^= from_to_bb;
    } else if (move.is_capture()){
        // Make sure to lookup and record the piece captured 
        pieces[move.origin] = pieces[move.target];
        pieces[move.target] = move.captured_peice;
        occupied ^= from_bb;
    } else {
        pieces[move.origin] = pieces[move.target];
        pieces[move.target] = Pieces::Blank;
        from_to_bb = from_bb ^ to_bb;
        occupied ^= from_to_bb;
    }
    // And now do the promotion if it is one.
    if (move.is_promotion()) {
        pieces[move.origin] = pieces[move.origin].get_colour() | Pieces::Pawn;
    } 
    search_pins(whos_move); 
    update_checkers();
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

Square Board::slide_to_edge(const Square origin, const Square direction, const uint to_edge) const {
    // Look along a direction until you get to the edge of the board or a piece.
    Square target = origin;
    for (uint i = 0; i < to_edge; i++) {
        target = target + direction;
        if (is_free(target)) {
            // Blank Square
            continue;
        } else {
            return target;
        }
    };
    return target;
}

bool Board::is_check(const Square origin, const Piece colour) const{
    // Want to (semi-efficiently) see if a square is attacked, ignoring pins.
    // First off, if the square is attacked by a knight, it's definitely in check.
    for (Square target : knight_moves(origin)) {
        if (pieces[target] == (~colour | Pieces::Knight)) {
            return true;
        }
    }

    // Pawn square
    Square target;
    if (origin.to_west() != 0) {
        target = origin + (Squares::W + forwards(colour));
        if (pieces[target] == (~colour | Pieces::Pawn)) {
            return true;
        }
    }
    if (origin.to_east() != 0) {
        target = origin + (Squares::E + forwards(colour));
        if (pieces[target] == (~colour | Pieces::Pawn)) {
            return true;
        }
    }

    // Sliding moves.
    Piece target_piece;
    std::array<Square, 4> targets;
    targets[0] = slide_to_edge(origin, Squares::N, origin.to_north());
    targets[1] = slide_to_edge(origin, Squares::E, origin.to_east());
    targets[2] = slide_to_edge(origin, Squares::S, origin.to_south());
    targets[3] = slide_to_edge(origin, Squares::W, origin.to_west());
    for (Square target : targets){
        target_piece = pieces[target];
        if ((target_piece == (~colour | Pieces::Rook)) | (target_piece == (~colour | Pieces::Queen))) {
            return true;
        }
    }
    
    // Diagonals
    targets[0] = slide_to_edge(origin, Squares::NE, std::min(origin.to_north(), origin.to_east()));
    targets[1] = slide_to_edge(origin, Squares::SE, std::min(origin.to_south(), origin.to_east()));
    targets[2] = slide_to_edge(origin, Squares::SW, std::min(origin.to_south(), origin.to_west()));
    targets[3] = slide_to_edge(origin, Squares::NW, std::min(origin.to_north(), origin.to_west()));
    for (Square target : targets){
        target_piece = pieces[target];
        if ((target_piece == (~colour | Pieces::Bishop)) | (target_piece == (~colour | Pieces::Queen))) {
            return true;
        }
    }
    
    // King's can't be next to each other in a game, but this is how we enforce that.
    if (origin.to_north() != 0) {
        if (pieces[origin + Squares::N] == (~colour.get_colour() | Pieces::King)) {
            return true;
        }
        if (origin.to_east() != 0) {
            if (pieces[origin + Squares::NE] == (~colour.get_colour() | Pieces::King)) {
                return true;
            }
        }
        if (origin.to_west() != 0) {
            if (pieces[origin + Squares::NW] == (~colour.get_colour() | Pieces::King)) {
                return true;
            }
        }
    } 
    if (origin.to_south() != 0) {
        if (pieces[origin + Squares::S] == (~colour.get_colour() | Pieces::King)) {
            return true;
        }
        if (origin.to_east() != 0) {
            if (pieces[origin + Squares::SE] == (~colour.get_colour() | Pieces::King)) {
                return true;
            }
        }
        if (origin.to_west() != 0) {
            if (pieces[origin + Squares::SW] == (~colour.get_colour() | Pieces::King)) {
                return true;
            }
        }
    } 
    if (origin.to_east() != 0) {
        if (pieces[origin + Squares::E] == (~colour.get_colour() | Pieces::King)) {
            return true;
        }
    }
    if (origin.to_west() != 0) {
        if (pieces[origin + Squares::W] == (~colour.get_colour() | Pieces::King)) {
            return true;
        }
    }
    
    // Nothing so far, that's good, no checks.
    return false;
}

void Board::search_kings() {
    for (Square::square_t i = 0; i < 64; i++) {
        if (pieces[i].is_king()) { 
            if (pieces[i].is_white()) {
                king_square[0] = i;
            } else {
                king_square[1] = i;
            }
        }
    }
}

Square Board::find_king(const Piece colour) const{
    return colour.is_white() ? king_square[0] : king_square[1];
}

std::vector<Move> Board::get_moves(){
    Piece colour = whos_move;
    const std::vector<Move> pseudolegal_moves = get_pseudolegal_moves();
    std::vector<Move> legal_moves;
    legal_moves.reserve(256);
    Square king_square = find_king(colour);
    if (aux_info.is_check) {
        for (Move move : pseudolegal_moves) {
            if (move.origin == king_square) {
                // King moves have to be very careful.
                make_move(move);
                if (!is_check(move.target, colour)) {legal_moves.push_back(move); }
                unmake_move(move);
            } else if (numer_checkers == 2) {
                // double checks require a king move, which we've just seen this is not.
                continue;
            } else if (is_pinned(move.origin)) {
                continue;
            } else if (move.target == checkers[0]) {
                // this captures the checker, it's legal unless the peice in absolutely pinned.
                legal_moves.push_back(move);
            } else if ( interposes(king_square, checkers[0], move.target)) {
                // this interposes the check it's legal unless the peice in absolutely pinned.
                legal_moves.push_back(move);
            } else if (move.is_ep_capture() & ((move.origin.rank()|move.target.file()) == checkers[0])){
                // If it's an enpassent capture, the captures piece isn't at the target.
                legal_moves.push_back(move);
            } else {
                // All other moves are illegal.
                continue;
            }
        }
        return legal_moves;
    } 
    
    // Otherwise we can be smarter.
    for (Move move : pseudolegal_moves) {
        if (move.origin == king_square) {
            // This is a king move, check where he's gone.
            make_move(move);
            if (!is_check(move.target, colour)) {legal_moves.push_back(move); }
            unmake_move(move);  
        } else if (move.is_ep_capture()) {
            // en_passent's are weird.
            if (is_pinned(move.origin)) {
                // If the pawn was pinned to the diagonal or file, the move is definitely illegal.
                continue;
            } else {
                // This can open a rank. if the king is on that rank it could be a problem.
                if (king_square.rank() == move.origin.rank()) {
                    make_move(move);
                    if (!is_check(king_square, colour)) {legal_moves.push_back(move); }
                    unmake_move(move);
                } else {
                    legal_moves.push_back(move); 
                }
            }
        } else if (is_pinned(move.origin)) {
            // This piece is absoluetly pinned, only legal moves will maintain the pin or capture the pinner.
            if (in_line(king_square, move.origin, move.target)) {legal_moves.push_back(move); }
        } else {
            // Piece isn't pinned, it can do what it wants. 
            legal_moves.push_back(move);
        }
    }
    return legal_moves;
}

std::vector<Move> Board::get_sorted_moves() {
    const std::vector<Move> legal_moves = get_moves();
    std::vector<Move> checks, promotions, captures, quiet_moves, sorted_moves;
    checks.reserve(16);
    promotions.reserve(16);
    captures.reserve(16);
    quiet_moves.reserve(16);
    for (Move move : legal_moves) {
        make_move(move);
        if (aux_info.is_check) {
            checks.push_back(move);
        } else if (move.is_queen_promotion()) {
            promotions.push_back(move);
        } else if (move.is_capture()) {
            promotions.push_back(move);
        } else {
            quiet_moves.push_back(move);
        }
        unmake_move(move);
    }
    sorted_moves.clear();
    sorted_moves.insert(sorted_moves.end(), checks.begin(), checks.end());
    sorted_moves.insert(sorted_moves.end(), promotions.begin(), promotions.end());
    sorted_moves.insert(sorted_moves.end(), captures.begin(), captures.end());
    sorted_moves.insert(sorted_moves.end(), quiet_moves.begin(), quiet_moves.end());
    return sorted_moves;
}

bool Board::is_in_check() const {
    Piece colour = whos_move;
    Square king_square = find_king(colour);
    return is_check(king_square, colour);
}

bool in_line(const Square origin, const Square target){
    if (origin.file() == target.file()) {return true; }
    if (origin.rank() == target.rank()) {return true; }
    if (origin.anti_diagonal() == target.anti_diagonal()) {return true; }
    if (origin.diagonal() == target.diagonal()) {return true; }
    return false;
}

bool in_line(const Square p1, const Square p2, const Square p3){
    if (p1.file() == p2.file() & p1.file() == p3.file()) {return true; }
    if (p1.rank() == p2.rank() & p1.rank() == p3.rank()) {return true; }
    if (p1.diagonal() == p2.diagonal() & p1.diagonal() == p3.diagonal()) {return true; }
    if (p1.anti_diagonal() == p2.anti_diagonal() & p1.anti_diagonal() == p3.anti_diagonal()) {return true; }
    return false;
}

bool interposes(const Square origin, const Square target, const Square query) {
    if (origin.file() == target.file()) {
        if (origin.file() != query.file()) {return false;}
        if ((query.rank() > std::min(origin.rank(), target.rank())) & (query.rank() < std::max(origin.rank(), target.rank()))) {
            return true;
        } else { return false;}
    }
    if (origin.rank() == target.rank()) {
        if (origin.rank() != query.rank()) {return false;}
        if ((query.file() > std::min(origin.file(), target.file())) & (query.file() < std::max(origin.file(), target.file()))) {
            return true;
        } else { return false;}
    }

    if (origin.diagonal() == target.diagonal()) {
        if (origin.diagonal() != query.diagonal()) {return false;}
        if ((query.anti_diagonal() > std::min(origin.anti_diagonal(), target.anti_diagonal())) & (query.anti_diagonal() < std::max(origin.anti_diagonal(), target.anti_diagonal()))) {
            return true;
        } else { return false;}
    }

    if (origin.anti_diagonal() == target.anti_diagonal()) {
        if (origin.anti_diagonal() != query.anti_diagonal()) {return false;}
        if ((query.diagonal() > std::min(origin.diagonal(), target.diagonal())) & (query.diagonal() < std::max(origin.diagonal(), target.diagonal()))) {
            return true;
        } else { return false;}
    }

    return false;
}

int what_dirx(const Square origin, const Square target){
    if (origin.file() == target.file()) {return origin.rank_index() < target.rank_index() ? 2 : 0; }
    if (origin.rank() == target.rank()) {return origin.file_index() < target.file_index() ? 1 : 3; }
    if (origin.anti_diagonal() == target.anti_diagonal()) {return origin.diagonal() < target.diagonal() ? 5 : 7; }
    if (origin.diagonal() == target.diagonal()) {return origin.anti_diagonal() < target.anti_diagonal() ? 6 : 4; }
    return -1;
}

bool Board::is_pinned(const Square origin) const{
    for (Square target : pinned_pieces) {
        if (origin == target) {
            return true;
        }
    }
    return false;
}

Square slide_rook_pin(const Board &board, const Square origin, const Square direction, const uint to_edge, const Piece colour) {
    // Slide from the origin in a direction, to look for a peice pinned by some sliding piece of ~colour
    Square target = origin;
    // Flag of if we have found a piece in the way yet.
    bool flag = false;
    Square found_piece;
    // First look south
    for (uint i = 0; i < to_edge; i++) {
        target = target + direction;
        if (board.is_free(target)) {
            // Blank Square
            continue;
        }
        if (board.pieces[target].is_colour(colour)) {
            // It's our piece.
            if (flag) {
                // This piece isn't pinned.
                return origin;
            } else {
                flag = true;
                found_piece = target;
                continue;
            }
        } else {
            // It's their piece.
            if (flag) {
                // The last piece we found may be pinned.
                return (board.pieces[target].is_rook() | board.pieces[target].is_queen()) ? found_piece : origin;
            } else {
                // Origin is attacked, but no pins here.
                return origin;
            }
        }
    };
    return origin;
}

Square slide_bishop_pin(const Board &board, const Square origin, const Square direction, const uint to_edge, const Piece colour) {
    // Slide from the origin in a direction, to look for a peice pinned by some sliding piece of ~colour
    Square target = origin;
    // Flag of if we have found a piece in the way yet.
    bool flag = false;
    Square found_piece;
    // First look south
    for (uint i = 0; i < to_edge; i++) {
        target = target + direction;
        if (board.is_free(target)) {
            // Blank Square
            continue;
        }
        if (board.pieces[target].is_colour(colour)) {
            // It's our piece.
            if (flag) {
                // This piece isn't pinned.
                return origin;
            } else {
                flag = true;
                found_piece = target;
                continue;
            }
        } else {
            // It's their piece.
            if (flag) {
                // The last piece we found may be pinned.
                return (board.pieces[target].is_bishop() | board.pieces[target].is_queen()) ? found_piece : origin;
            } else {
                // Origin is attacked, but no pins here.
                return origin;
            }
        }
    };
    return origin;
}

void Board::search_pins(const Piece colour) {
    // Max 8 sliding pieces in line. 
    // Sliding moves.
    Square origin = find_king(colour);
    uint start_index = colour.is_white() ? 0 : 8;
    std::array<Square, 16> targets;
    targets[0] = slide_rook_pin(*this, origin, Squares::N, origin.to_north(), colour);
    targets[1] = slide_rook_pin(*this, origin, Squares::E, origin.to_east(), colour);
    targets[2] = slide_rook_pin(*this, origin, Squares::S, origin.to_south(), colour);
    targets[3] = slide_rook_pin(*this, origin, Squares::W, origin.to_west(), colour);
    targets[4] = slide_bishop_pin(*this, origin, Squares::NE, origin.to_northeast(), colour);
    targets[5] = slide_bishop_pin(*this, origin, Squares::SE, origin.to_southeast(), colour);
    targets[6] = slide_bishop_pin(*this, origin, Squares::SW, origin.to_southwest(), colour);
    targets[7] = slide_bishop_pin(*this, origin, Squares::NW, origin.to_northwest(), colour);

    for (int i = 0; i < 8; i++){
        pinned_pieces[i + start_index] = targets[i];
    }
    
}


void Board::search_pins(const Piece colour, const Square origin, const Square target) {
    Square king_square = find_king(colour);
    if (!in_line(king_square, origin) & !in_line(king_square, target)) {return;}
    uint start_index = colour.is_white() ? 0 : 8;
    std::array<Square, 16> targets;;
    if (in_line(king_square, origin)) {
        int origin_dirx = what_dirx(king_square, origin);
        if (origin_dirx < 4) {
            // Rook move.
            pinned_pieces[start_index + origin_dirx] = slide_rook_pin(*this, king_square, Squares::by_dirx[origin_dirx], king_square.to_dirx(origin_dirx), colour);
        } else {
            pinned_pieces[start_index + origin_dirx] = slide_bishop_pin(*this, king_square, Squares::by_dirx[origin_dirx], king_square.to_dirx(origin_dirx), colour);
        }
    }
    if (in_line(king_square, target)) {
        int target_dirx = what_dirx(king_square, target);
        if (target_dirx < 4) {
            // Rook move.
            pinned_pieces[start_index + target_dirx] = slide_rook_pin(*this, king_square, Squares::by_dirx[target_dirx], king_square.to_dirx(target_dirx), colour);
        } else {
            pinned_pieces[start_index + target_dirx] = slide_bishop_pin(*this, king_square, Squares::by_dirx[target_dirx], king_square.to_dirx(target_dirx), colour);
        }
    }
}

void Board::update_checkers() {
    Square origin = find_king(whos_move);
    Piece colour = whos_move;
    // Want to (semi-efficiently) see if a square is attacked, ignoring pins.
    // First off, if the square is attacked by a knight, it's definitely in check.
    numer_checkers = 0;
    for (Square target : knight_moves(origin)) {
        if (pieces[target] == (~colour | Pieces::Knight)) {
            checkers[numer_checkers] = target;
            numer_checkers++;
            continue;
        }
    }

    // Pawn square
    Square target;
    if (origin.to_west() != 0) {
        target = origin + (Squares::W + forwards(colour));
        if (pieces[target] == (~colour | Pieces::Pawn)) {
            checkers[numer_checkers] = target;
            numer_checkers++;
        }
    }
    if (origin.to_east() != 0) {
        target = origin + (Squares::E + forwards(colour));
        if (pieces[target] == (~colour | Pieces::Pawn)) {
            checkers[numer_checkers] = target;
            numer_checkers++;
        }
    }

    // Sliding moves.
    Piece target_piece;
    std::array<Square, 4> targets;
    targets[0] = slide_to_edge(origin, Squares::N, origin.to_north());
    targets[1] = slide_to_edge(origin, Squares::E, origin.to_east());
    targets[2] = slide_to_edge(origin, Squares::S, origin.to_south());
    targets[3] = slide_to_edge(origin, Squares::W, origin.to_west());
    for (Square target : targets){
        target_piece = pieces[target];
        if ((target_piece == (~colour | Pieces::Rook)) | (target_piece == (~colour | Pieces::Queen))) {
            checkers[numer_checkers] = target;
            numer_checkers++;
        }
    }
    
    // Diagonals
    targets[0] = slide_to_edge(origin, Squares::NE, std::min(origin.to_north(), origin.to_east()));
    targets[1] = slide_to_edge(origin, Squares::SE, std::min(origin.to_south(), origin.to_east()));
    targets[2] = slide_to_edge(origin, Squares::SW, std::min(origin.to_south(), origin.to_west()));
    targets[3] = slide_to_edge(origin, Squares::NW, std::min(origin.to_north(), origin.to_west()));
    for (Square target : targets){
        target_piece = pieces[target];
        if ((target_piece == (~colour | Pieces::Bishop)) | (target_piece == (~colour | Pieces::Queen))) {
            checkers[numer_checkers] = target;
            numer_checkers++;
        }
    }
    aux_info.is_check = (numer_checkers > 0);
}

int Board::evaluate() {
    std::vector<Move> legal_moves = get_moves();
    return evaluate(legal_moves, is_white_move());
}


int Board::evaluate(std::vector<Move> &legal_moves, const bool maximising) {
    int value = 0;
    // First check if we have been mated.
    if (legal_moves.size() == 0) {
        if (aux_info.is_check) {
            // This is checkmate
            return -mating_score * (maximising ? 1 : -1);
        } else {
            // This is stalemate.
            return -10 * (maximising ? 1 : -1);
        }
    }
    for (uint i = 0; i < 64; i++) {
        value += material(pieces[i]);
    }
    return value;
}


int Board::evaluate_negamax() {
    std::vector<Move> legal_moves = get_moves();
    return evaluate_negamax(legal_moves);
}

int Board::evaluate_negamax(std::vector<Move> &legal_moves) {
    int side_multiplier = whos_move ? -1 : 1;
    int value = 0;
    // First check if we have been mated.
    if (legal_moves.size() == 0) {
        if (aux_info.is_check) {
            // This is checkmate
            return -mating_score;
        } else {
            // This is stalemate.
            return -10;
        }
    }
    for (uint i = 0; i < 64; i++) {
        value += material(pieces[i]);
    }
    return value * side_multiplier;
}