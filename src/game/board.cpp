#include <map>
#include <iostream>
#include <istream>
#include <sstream>
#include <cstdint> //for uint8_t
#include <stdexcept>

#include "board.hpp"
#include "../search/evaluate.hpp"


constexpr Square forwards(const Piece colour) {
    if (colour.is_white()) {
        return Direction::N;
    } else {
        return Direction::S;
    }
}

constexpr Square back_rank(const Piece colour) {
    if (colour.is_white()) {
        return Squares::Rank1;
    } else {
        return Squares::Rank8;
    }
}


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

void Board::fen_decode(const std::string& fen){
    uint N = fen.length(), board_position;
    uint rank = 0, file = 0;
    char my_char;
    
    std::stringstream stream;
    // reset board
    for (uint i = 0; i < 64; i++) {
        pieces_array[i] = Pieces::Blank;
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
            file += (my_char - '0');
            continue;
        }
        if (my_char == ' ') {
            // Space is at the end of the board position section
            board_position = i;
            break;
        }
        // Otherwise should be a character for a piece
        pieces_array[Square::to_index(rank, file)] = fen_decode_map[my_char];
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

// Geometry

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

void Board::initialise() {
    build_occupied_bb();
    search_kings();
    update_checkers();
    aux_info.lazy_heuristic = evaluate_material(*this);
}



void Board::build_occupied_bb() {
    bitboard temp_occupied = 0;
    for (uint i = 0; i < 64; i++) {
        if (!pieces(i).is_blank()) {
            temp_occupied |= (uint64_t(1)<< i);
        }
    }
    occupied = temp_occupied;
}

bool Board::is_free(const Square target) const{  
    return (occupied & (uint64_t(1) << target)) == 0 ;
};

void Board::make_move(Move &move) {
    last_move = move;
    // Iterate counters.
    if (whos_move == black_move) {fullmove_counter++;}
    aux_info.pinned_pieces = pinned_pieces;
    aux_info.checkers = checkers;
    aux_info.number_checkers = number_checkers;
    aux_history[ply_counter] = aux_info;
    if (move.is_capture() | pieces(move.origin).is_pawn()) {
        aux_info.halfmove_clock = 0;
    } else {aux_info.halfmove_clock++ ;}
    // Track en-passent square
    if (move.is_double_push()) {
        // Little hacky but this is the square in between.
        aux_info.en_passent_target = (move.origin.get_value() + move.target.get_value())/2;
    } else {
        aux_info.en_passent_target = 0;
    }

    if (pieces(move.origin).is_king()) {
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
        pieces_array[move.target] = pieces_array[move.origin];
        pieces_array[move.origin] = Pieces::Blank;
        pieces_array[move.origin + Direction::E] = pieces_array[move.origin.rank() | Squares::FileH];
        pieces_array[move.origin.rank() | Squares::FileH] = Pieces::Blank;
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
        pieces_array[move.target] = pieces_array[move.origin];
        pieces_array[move.origin] = Pieces::Blank;
        pieces_array[move.origin + Direction::W] = pieces_array[move.origin.rank() | Squares::FileA];
        pieces_array[move.origin.rank() | Squares::FileA] = Pieces::Blank;
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
        move.captured_peice = pieces_array[captured_square];
        pieces_array[move.target] = pieces_array[move.origin];
        pieces_array[move.origin] = Pieces::Blank;
        pieces_array[captured_square] = Pieces::Blank;
    } else if (move.is_capture()){
        // Update the bitboard.
        occupied ^= from_bb;
        // Make sure to lookup and record the piece captured 
        move.captured_peice = pieces_array[move.target];
        pieces_array[move.target] = pieces_array[move.origin];
        pieces_array[move.origin] = Pieces::Blank;
    } else {
        pieces_array[move.target] = pieces_array[move.origin];
        pieces_array[move.origin] = Pieces::Blank;
        from_to_bb = from_bb ^ to_bb;
        occupied ^= from_to_bb;
    }
    // And now do the promotion if it is one.
    if (move.is_knight_promotion()) {
        pieces_array[move.target] = pieces_array[move.target].get_colour() | Pieces::Knight;
    } else if (move.is_bishop_promotion()){
        pieces_array[move.target] = pieces_array[move.target].get_colour() | Pieces::Bishop;
    } else if (move.is_rook_promotion()){
        pieces_array[move.target] = pieces_array[move.target].get_colour() | Pieces::Rook;
    } else if (move.is_queen_promotion()){
        pieces_array[move.target] = pieces_array[move.target].get_colour() | Pieces::Queen;
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

    if (pieces_array[move.target].is_king()) {
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
        pieces_array[move.origin] = pieces_array[move.target];
        pieces_array[move.target] = Pieces::Blank;
        pieces_array[move.origin.rank() | Squares::FileH] = pieces_array[move.origin + Direction::E];
        pieces_array[move.origin + Direction::E] = Pieces::Blank;
        from_bb = from_bb ^ (uint64_t(1) << (move.origin.rank() | Squares::FileH));
        to_bb = to_bb ^ (uint64_t(1) << (move.origin.rank() | Squares::FileF));
        // Update the bitboard.
        from_to_bb = from_bb ^ to_bb;
        occupied ^= from_to_bb;
    } else if (move.is_queen_castle()) {
        pieces_array[move.origin] = pieces_array[move.target];
        pieces_array[move.target] = Pieces::Blank;
        pieces_array[move.origin.rank() | Squares::FileA] = pieces_array[move.origin + Direction::W];
        pieces_array[move.origin + Direction::W] = Pieces::Blank;
        from_bb = from_bb ^ (uint64_t(1) << (move.origin.rank() | Squares::FileA));
        to_bb = to_bb ^ (uint64_t(1) << (move.origin.rank() | Squares::FileD));
        // Update the bitboard.
        from_to_bb = from_bb ^ to_bb;
        occupied ^= from_to_bb;
    } else if(move.is_ep_capture()) {
        // En-passent is weird too.
        const Square captured_square = move.origin.rank() | move.target.file();
        // Make sure to lookup and record the piece captured 
        pieces_array[move.origin] = pieces_array[move.target];
        pieces_array[move.target] = Pieces::Blank;
        pieces_array[captured_square] = move.captured_peice;
        from_to_bb = from_bb ^ to_bb;
        from_to_bb ^= (uint64_t(1) << captured_square);
        occupied ^= from_to_bb;
    } else if (move.is_capture()){
        // Make sure to lookup and record the piece captured 
        pieces_array[move.origin] = pieces_array[move.target];
        pieces_array[move.target] = move.captured_peice;
        occupied ^= from_bb;
    } else {
        pieces_array[move.origin] = pieces_array[move.target];
        pieces_array[move.target] = Pieces::Blank;
        from_to_bb = from_bb ^ to_bb;
        occupied ^= from_to_bb;
    }
    // And now do the promotion if it is one.
    if (move.is_promotion()) {
        pieces_array[move.origin] = pieces_array[move.origin].get_colour() | Pieces::Pawn;
    } 
    pinned_pieces = aux_info.pinned_pieces;
    number_checkers = aux_info.number_checkers;
    checkers = aux_info.checkers;
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

bool Board::try_uci_move(const std::string move_sting) {
    std::vector<Move> legal_moves = get_moves();
    for (Move move : legal_moves) {
        if (move_sting == move.pretty_print()) {
            make_move(move);
            return true;
        }
    }
    return false;
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
        if (pieces_array[target] == (~colour | Pieces::Knight)) {
            return true;
        }
    }

    // Pawn square
    Square target;
    if (origin.rank() != back_rank(~colour)) {
        if (origin.to_west() != 0) {
            target = origin + (Direction::W + forwards(colour));
            if (pieces_array[target] == (~colour | Pieces::Pawn)) {
                return true;
            }
        }
        if (origin.to_east() != 0) {
            target = origin + (Direction::E + forwards(colour));
            if (pieces_array[target] == (~colour | Pieces::Pawn)) {
                return true;
            }
        }
    }
    // Sliding moves.
    Piece target_piece;
    std::array<Square, 4> targets;
    targets[0] = slide_to_edge(origin, Direction::N, origin.to_north());
    targets[1] = slide_to_edge(origin, Direction::E, origin.to_east());
    targets[2] = slide_to_edge(origin, Direction::S, origin.to_south());
    targets[3] = slide_to_edge(origin, Direction::W, origin.to_west());
    for (Square target : targets){
        target_piece = pieces_array[target];
        if ((target_piece == (~colour | Pieces::Rook)) | (target_piece == (~colour | Pieces::Queen))) {
            return true;
        }
    }
    
    // Diagonals
    targets[0] = slide_to_edge(origin, Direction::NE, std::min(origin.to_north(), origin.to_east()));
    targets[1] = slide_to_edge(origin, Direction::SE, std::min(origin.to_south(), origin.to_east()));
    targets[2] = slide_to_edge(origin, Direction::SW, std::min(origin.to_south(), origin.to_west()));
    targets[3] = slide_to_edge(origin, Direction::NW, std::min(origin.to_north(), origin.to_west()));
    for (Square target : targets){
        target_piece = pieces_array[target];
        if ((target_piece == (~colour | Pieces::Bishop)) | (target_piece == (~colour | Pieces::Queen))) {
            return true;
        }
    }
    
    // King's can't be next to each other in a game, but this is how we enforce that.
    if (origin.to_north() != 0) {
        if (pieces_array[origin + Direction::N] == (~colour.get_colour() | Pieces::King)) {
            return true;
        }
        if (origin.to_east() != 0) {
            if (pieces_array[origin + Direction::NE] == (~colour.get_colour() | Pieces::King)) {
                return true;
            }
        }
        if (origin.to_west() != 0) {
            if (pieces_array[origin + Direction::NW] == (~colour.get_colour() | Pieces::King)) {
                return true;
            }
        }
    } 
    if (origin.to_south() != 0) {
        if (pieces_array[origin + Direction::S] == (~colour.get_colour() | Pieces::King)) {
            return true;
        }
        if (origin.to_east() != 0) {
            if (pieces_array[origin + Direction::SE] == (~colour.get_colour() | Pieces::King)) {
                return true;
            }
        }
        if (origin.to_west() != 0) {
            if (pieces_array[origin + Direction::SW] == (~colour.get_colour() | Pieces::King)) {
                return true;
            }
        }
    } 
    if (origin.to_east() != 0) {
        if (pieces_array[origin + Direction::E] == (~colour.get_colour() | Pieces::King)) {
            return true;
        }
    }
    if (origin.to_west() != 0) {
        if (pieces_array[origin + Direction::W] == (~colour.get_colour() | Pieces::King)) {
            return true;
        }
    }
    
    // Nothing so far, that's good, no checks.
    return false;
}

void Board::search_kings() {
    for (Square::square_t i = 0; i < 64; i++) {
        if (pieces_array[i].is_king()) { 
            if (pieces_array[i].is_white()) {
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

bool Board::is_in_check() const {
    Piece colour = whos_move;
    Square king_square = find_king(colour);
    return is_check(king_square, colour);
}


bool Board::is_pinned(const Square origin) const{
    for (Square target : pinned_pieces) {
        if (origin == target) {
            return true;
        }
    }
    return false;
}

void Board::slide_rook_pin(const Square origin, const Square direction, const uint to_edge, const Piece colour, const int idx) {
    // Slide from the origin in a direction, to look for a peice pinned by some sliding piece of ~colour
    Square target = origin;
    // Flag of if we have found a piece in the way yet.
    bool flag = false;
    Square found_piece;
    // First look south
    for (uint i = 0; i < to_edge; i++) {
        target = target + direction;
        if (is_free(target)) {
            // Blank Square
            continue;
        }
        if (pieces_array[target].is_colour(colour)) {
            // It's our piece.
            if (flag) {
                // This piece isn't pinned.
                pinned_pieces[idx] = origin;
                return;
            } else {
                flag = true;
                found_piece = target;
                continue;
            }
        } else {
            // It's their piece.
            if (flag) {
                // The last piece we found may be pinned.
                pinned_pieces[idx] = (pieces_array[target].is_rook() | pieces_array[target].is_queen()) ? found_piece : origin;
                return;
            } else {
                // Origin is attacked, but no pins here.
                pinned_pieces[idx] = origin;
                if ((pieces_array[target].is_rook() | pieces_array[target].is_queen())) {
                    checkers[number_checkers] = target;
                    number_checkers++;
                }
                return;
            }
        }
    };
    pinned_pieces[idx] =  origin;
    return;
}

void Board::slide_bishop_pin(const Square origin, const Square direction, const uint to_edge, const Piece colour, const int idx) {
    // Slide from the origin in a direction, to look for a peice pinned by some sliding piece of ~colour
    Square target = origin;
    // Flag of if we have found a piece in the way yet.
    bool flag = false;
    Square found_piece;
    // First look south
    for (uint i = 0; i < to_edge; i++) {
        target = target + direction;
        if (is_free(target)) {
            // Blank Square
            continue;
        }
        if (pieces_array[target].is_colour(colour)) {
            // It's our piece.
            if (flag) {
                // This piece isn't pinned.
                pinned_pieces[idx] = origin;
                return;
            } else {
                flag = true;
                found_piece = target;
                continue;
            }
        } else {
            // It's their piece.
            if (flag) {
                // The last piece we found may be pinned.
                pinned_pieces[idx] = (pieces_array[target].is_bishop() | pieces_array[target].is_queen()) ? found_piece : origin;
                return;
            } else {
                // Origin is attacked, but no pins here.
                pinned_pieces[idx] = origin;
                if ((pieces_array[target].is_bishop() | pieces_array[target].is_queen())) {
                    checkers[number_checkers] = target;
                    number_checkers++;
                }
                return;
            }
        }
    };
    pinned_pieces[idx] =  origin;
    return;
}

void Board::update_checkers() {
    Square origin = find_king(whos_move);
    Piece colour = whos_move;
    // Want to (semi-efficiently) see if a square is attacked, ignoring pins.
    // First off, if the square is attacked by a knight, it's definitely in check.
    number_checkers = 0;
    for (Square target : knight_moves(origin)) {
        if (pieces_array[target] == (~colour | Pieces::Knight)) {
            checkers[number_checkers] = target;
            number_checkers++;
            continue;
        }
    }

    // Pawn square
    Square target;
    if (origin.to_west() != 0) {
        target = origin + (Direction::W + forwards(colour));
        if (pieces_array[target] == (~colour | Pieces::Pawn)) {
            checkers[number_checkers] = target;
            number_checkers++;
        }
    }
    if (origin.to_east() != 0) {
        target = origin + (Direction::E + forwards(colour));
        if (pieces_array[target] == (~colour | Pieces::Pawn)) {
            checkers[number_checkers] = target;
            number_checkers++;
        }
    }
    // Sliding_pieces

    uint start_index = colour.is_white() ? 0 : 8;
    slide_rook_pin(origin, Direction::N, origin.to_north(), colour, start_index + 0);
    slide_rook_pin(origin, Direction::E, origin.to_east(), colour, start_index + 1);
    slide_rook_pin(origin, Direction::S, origin.to_south(), colour, start_index + 2);
    slide_rook_pin(origin, Direction::W, origin.to_west(), colour, start_index + 3);
    slide_bishop_pin(origin, Direction::NE, origin.to_northeast(), colour, start_index + 4);
    slide_bishop_pin(origin, Direction::SE, origin.to_southeast(), colour, start_index + 5);
    slide_bishop_pin(origin, Direction::SW, origin.to_southwest(), colour, start_index + 6);
    slide_bishop_pin(origin, Direction::NW, origin.to_northwest(), colour, start_index + 7);
    aux_info.is_check = number_checkers > 0;
}


bool is_mating(int score) {
    return (score > (mating_score-500));
}

std::string print_score(int score) {
    std::stringstream ss;
    if (is_mating(score)) {
        //Make for white.
        signed int n = mating_score - score;
        ss << "#" << n;
    }else if (is_mating(-score)) {
        //Make for black.
        signed int n = (score + mating_score);
        ss << "#-" << n;
    } else if (score > 5) {
        // White winning
        ss << "+" << score / 100 << "." << (score % 100) / 10;
    } else if (score < -5) {
        // White winning
        ss << "-" << -score / 100 << "." << (-score % 100) / 10;
    } else {
        // White winning
        ss << "0.0";
    }
    std::string out;
    ss >> out;
    return out;
}

