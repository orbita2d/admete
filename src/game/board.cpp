#include <map>
#include <iostream>
#include <istream>
#include <sstream>
#include <cstdint> //for uint8_t
#include <stdexcept>
#include <random>

#include "board.hpp"


std::map<char, Piece> fen_decode_map = {
    {'p', Piece(BLACK, PAWN)},
    {'n', Piece(BLACK, KNIGHT)},
    {'b', Piece(BLACK, BISHOP)},
    {'r', Piece(BLACK, ROOK)},
    {'q', Piece(BLACK, QUEEN)},
    {'k', Piece(BLACK, KING)},
    {'P', Piece(WHITE, PAWN)},
    {'N', Piece(WHITE, KNIGHT)},
    {'B', Piece(WHITE, BISHOP)},
    {'R', Piece(WHITE, ROOK)},
    {'Q', Piece(WHITE, QUEEN)},
    {'K', Piece(WHITE, KING)}
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
    stream >> en_passent >> std::ws;
    stream >> halfmove >> std::ws;
    stream >> counter >> std::ws;

    // Side to move
    if (side_to_move.length() > 1) {
        throw std::domain_error("<Side to move> length > 1");
    }
    switch (side_to_move[0])
    {
    case 'w':
        whos_move = WHITE;
        break;

    case 'b':
        whos_move = BLACK;
        break;
    
    default:
        throw std::domain_error("Unrecognised <Side to move> character");
    }

    aux_info.castling_rights[WHITE][KINGSIDE] = false;
    aux_info.castling_rights[WHITE][QUEENSIDE] = false;
    aux_info.castling_rights[BLACK][KINGSIDE] = false;
    aux_info.castling_rights[BLACK][QUEENSIDE] = false;
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
                aux_info.castling_rights[BLACK][QUEENSIDE] = true;
                break;

            case 'Q':
                aux_info.castling_rights[WHITE][QUEENSIDE] = true;
                break;

            case 'k':
                aux_info.castling_rights[BLACK][KINGSIDE] = true;
                break;

            case 'K':
                aux_info.castling_rights[WHITE][KINGSIDE] = true;
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
    ply_counter = 0;
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
    if ((p1.file() == p2.file()) & (p1.file() == p3.file())) {return true; }
    if ((p1.rank() == p2.rank()) & (p1.rank() == p3.rank())) {return true; }
    if ((p1.diagonal() == p2.diagonal()) & (p1.diagonal() == p3.diagonal())) {return true; }
    if ((p1.anti_diagonal() == p2.anti_diagonal()) & (p1.anti_diagonal() == p3.anti_diagonal())) {return true; }
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
}

void Board::build_occupied_bb() {
    occupied_bb = 0;
    colour_bb[WHITE] = 0;
    colour_bb[BLACK] = 0;
    for (int i = 0; i < N_PIECE; i++) {
        piece_bb[i] = 0;
    }
    for (uint i = 0; i < 64; i++) {
        if (pieces(i).is_blank()) { continue; }
        occupied_bb |= sq_to_bb(i);
        if (pieces(i).is_colour(WHITE)) {
            colour_bb[WHITE] |= sq_to_bb(i);
        } else {
            colour_bb[BLACK] |= sq_to_bb(i);
        }
        piece_bb[to_enum_piece(pieces(i))] |= sq_to_bb(i);

    }
}

bool Board::is_free(const Square target) const{  
    return (occupied_bb & target) == 0 ;
};

bool Board::is_colour(const Colour c, const Square target) const{ 
    return (colour_bb[c] & target) != 0 ;
};

void Board::make_move(Move &move) {
    // Iterate counters.
    if (whos_move == Colour::BLACK) {fullmove_counter++;}
    aux_info.pinned = pinned_bb;
    aux_info.checkers = _checkers;
    aux_info.number_checkers = _number_checkers;
    aux_history[ply_counter] = aux_info;
    const Colour us = whos_move;
    const Colour them = ~ us;
    const Piece moving_piece = pieces(move.origin);
    const PieceEnum p = to_enum_piece(moving_piece);
    if (move.is_capture() | moving_piece.is_pawn()) {
        aux_info.halfmove_clock = 0;
    } else {aux_info.halfmove_clock++ ;}
    // Track en-passent square
    if (move.is_double_push()) {
        // Little hacky but this is the square in between.
        aux_info.en_passent_target = (move.origin.get_value() + move.target.get_value())/2;
    } else {
        aux_info.en_passent_target = 0;
    }

    if (p == KING) {
        king_square[us] = move.target;
        aux_info.castling_rights[us][KINGSIDE]  = false;
        aux_info.castling_rights[us][QUEENSIDE]  = false;
    }

    const Bitboard from_bb = sq_to_bb(move.origin);
    const Bitboard to_bb = sq_to_bb(move.target);
    const Bitboard from_to_bb = from_bb ^ to_bb;
    // Castling is special
    if (move.is_king_castle()) {
        pieces_array[move.target] = pieces_array[move.origin];
        pieces_array[move.origin] = Pieces::Blank;
        pieces_array[move.origin + Direction::E] = Piece(us, ROOK);
        pieces_array[RookSquare[us][KINGSIDE]] = Pieces::Blank;
        const Bitboard rook_from_to_bb = sq_to_bb(RookSquare[us][KINGSIDE]) ^ sq_to_bb(move.origin + Direction::E);
        // Update the bitboard.
        occupied_bb ^= from_to_bb;
        occupied_bb ^= rook_from_to_bb;
        colour_bb[us] ^= from_to_bb;
        colour_bb[us] ^= rook_from_to_bb;
        piece_bb[KING] ^= from_to_bb;
        piece_bb[ROOK] ^= rook_from_to_bb;
        aux_info.castling_rights[us][KINGSIDE] = false;
        aux_info.castling_rights[us][QUEENSIDE] = false;
    } else if (move.is_queen_castle()) {
        pieces_array[move.target] = pieces_array[move.origin];
        pieces_array[move.origin] = Pieces::Blank;
        pieces_array[move.origin + Direction::W] = Piece(us, ROOK);
        pieces_array[RookSquare[us][QUEENSIDE]] = Pieces::Blank;
        const Bitboard rook_from_to_bb = sq_to_bb(RookSquare[us][QUEENSIDE]) ^ sq_to_bb(move.origin + Direction::W);
        // Update the bitboard.
        occupied_bb ^= from_to_bb;
        occupied_bb ^= rook_from_to_bb;
        colour_bb[us] ^= from_to_bb;
        colour_bb[us] ^= rook_from_to_bb;
        piece_bb[KING] ^= from_to_bb;
        piece_bb[ROOK] ^= rook_from_to_bb;
        aux_info.castling_rights[us][KINGSIDE] = false;
        aux_info.castling_rights[us][QUEENSIDE] = false;
    } else if(move.is_ep_capture()) {
        // En-passent is weird too.
        const Square captured_square = move.origin.rank() | move.target.file();
        // Update the bitboard.
        occupied_bb ^= from_to_bb;
        occupied_bb ^= sq_to_bb(captured_square);
        colour_bb[us] ^= from_to_bb;
        colour_bb[them] ^= sq_to_bb(captured_square);
        piece_bb[PAWN] ^= from_to_bb;
        piece_bb[PAWN] ^= sq_to_bb(captured_square);
        // Make sure to lookup and record the piece captured 
        move.captured_peice = pieces_array[captured_square];
        pieces_array[move.target] = pieces_array[move.origin];
        pieces_array[move.origin] = Pieces::Blank;
        pieces_array[captured_square] = Pieces::Blank;
    } else if (move.is_capture()){
        // Make sure to lookup and record the piece captured 
        move.captured_peice = pieces_array[move.target];
        // Update the bitboard.
        occupied_bb ^= from_bb;
        colour_bb[us] ^= from_to_bb;
        colour_bb[them] ^= to_bb;
        piece_bb[p] ^= from_to_bb;
        piece_bb[to_enum_piece(move.captured_peice)] ^= to_bb;
        pieces_array[move.target] = pieces_array[move.origin];
        pieces_array[move.origin] = Pieces::Blank;
    } else {
        // Quiet move
        pieces_array[move.target] = pieces_array[move.origin];
        pieces_array[move.origin] = Pieces::Blank;
        occupied_bb ^= from_to_bb;
        colour_bb[us] ^= from_to_bb;
        piece_bb[p] ^= from_to_bb;
    }

    // And now do the promotion if it is one.
    if (move.is_knight_promotion()) {
        pieces_array[move.target] = Piece(us, KNIGHT);
        piece_bb[PAWN] ^= to_bb;
        piece_bb[KNIGHT] ^= to_bb;
    } else if (move.is_bishop_promotion()){
        pieces_array[move.target] = Piece(us, BISHOP);
        piece_bb[PAWN] ^= to_bb;
        piece_bb[BISHOP] ^= to_bb;
    } else if (move.is_rook_promotion()){
        pieces_array[move.target] = Piece(us, ROOK);
        piece_bb[PAWN] ^= to_bb;
        piece_bb[ROOK] ^= to_bb;
    } else if (move.is_queen_promotion()){
        pieces_array[move.target] = Piece(us, QUEEN);
        piece_bb[PAWN] ^= to_bb;
        piece_bb[QUEEN] ^= to_bb;
    }
    

    // Check if we've moved our rook.
    if (move.origin == RookSquare[us][KINGSIDE]) {
        aux_info.castling_rights[us][KINGSIDE]  = false;
    } else if (move.origin == RookSquare[us][QUEENSIDE]) {
        aux_info.castling_rights[us][QUEENSIDE]  = false;
    }
    // Check for rook captures.
    if (move.target == RookSquare[them][KINGSIDE]) {
        aux_info.castling_rights[them][KINGSIDE] = false;
    }
    if (move.target == RookSquare[them][QUEENSIDE]) {
        aux_info.castling_rights[them][QUEENSIDE] = false;
    }

    // Switch whos turn it is to play
    whos_move = ~ whos_move;

    update_checkers();
    ply_counter ++;
}

void Board::unmake_move(const Move move) {
    // Iterate counters.
    if (whos_move == Colour::WHITE) {fullmove_counter--;}
    ply_counter--;
    aux_info = aux_history[ply_counter];

    // Switch whos turn it is to play
    whos_move = ~ whos_move;
    Colour us = whos_move;
    Colour them = ~ us;

    const Piece moving_piece = pieces(move.target);
    const PieceEnum p = to_enum_piece(moving_piece);

    if (p == KING) {
        king_square[us] = move.origin;
    }

    const Bitboard from_bb = sq_to_bb(move.origin);
    const Bitboard to_bb = sq_to_bb(move.target);
    const Bitboard from_to_bb = from_bb ^ to_bb;
    if (move.is_king_castle()) {
        pieces_array[move.origin] = moving_piece;
        pieces_array[move.target] = Pieces::Blank;
        pieces_array[RookSquare[us][KINGSIDE]] = Piece(us, ROOK);
        pieces_array[move.origin + Direction::E] = Pieces::Blank;
        const Bitboard rook_from_to_bb = sq_to_bb(RookSquare[us][KINGSIDE]) ^ sq_to_bb(move.origin + Direction::E);
        // Update the bitboard.
        occupied_bb ^= from_to_bb;
        occupied_bb ^= rook_from_to_bb;
        colour_bb[us] ^= from_to_bb;
        colour_bb[us] ^= rook_from_to_bb;
        piece_bb[KING] ^= from_to_bb;
        piece_bb[ROOK] ^= rook_from_to_bb;
    } else if (move.is_queen_castle()) {
        pieces_array[move.origin] = moving_piece;
        pieces_array[move.target] = Pieces::Blank;
        pieces_array[RookSquare[us][QUEENSIDE]] = Piece(us, ROOK);
        pieces_array[move.origin + Direction::W] = Pieces::Blank;
        const Bitboard rook_from_to_bb = sq_to_bb(RookSquare[us][QUEENSIDE]) ^ sq_to_bb(move.origin + Direction::W);
        // Update the bitboard.
        occupied_bb ^= from_to_bb;
        occupied_bb ^= rook_from_to_bb;
        colour_bb[us] ^= from_to_bb;
        colour_bb[us] ^= rook_from_to_bb;
        piece_bb[KING] ^= from_to_bb;
        piece_bb[ROOK] ^= rook_from_to_bb;
    } else if(move.is_ep_capture()) {
        // En-passent is weird too.
        const Square captured_square = move.origin.rank() | move.target.file();   
        occupied_bb ^= from_to_bb;
        occupied_bb ^= sq_to_bb(captured_square);
        colour_bb[us] ^= from_to_bb;
        colour_bb[them] ^= sq_to_bb(captured_square);
        piece_bb[PAWN] ^= from_to_bb;
        piece_bb[PAWN] ^= sq_to_bb(captured_square);
        // Make sure to lookup and record the piece captured 
        pieces_array[move.origin] = moving_piece;
        pieces_array[move.target] = Pieces::Blank;
        pieces_array[captured_square] = move.captured_peice;
    } else if (move.is_capture()){
        // Make sure to lookup and record the piece captured 
        pieces_array[move.origin] = moving_piece;
        pieces_array[move.target] = move.captured_peice;
        // Update the bitboard.
        occupied_bb ^= from_bb;
        colour_bb[us] ^= from_to_bb;
        colour_bb[them] ^= to_bb;
        piece_bb[p] ^= from_to_bb;
        piece_bb[to_enum_piece(move.captured_peice)] ^= to_bb;
    } else {
        pieces_array[move.origin] = moving_piece;
        pieces_array[move.target] = Pieces::Blank;
        occupied_bb ^= from_to_bb;
        colour_bb[us] ^= from_to_bb;
        piece_bb[p] ^= from_to_bb;
    }
    // And now do the promotion if it is one.
    if (move.is_promotion()) {
        pieces_array[move.origin] = Piece(us, PAWN);
        // The quiet move method above moved a (promotion) from to->from
        // We need to remove the (promotion) piece and add a pawn
        piece_bb[PAWN] ^= from_bb;
    } 
    if (move.is_knight_promotion()) {
        piece_bb[KNIGHT] ^= from_bb;
    } else if (move.is_bishop_promotion()){
        piece_bb[BISHOP] ^= from_bb;
    } else if (move.is_rook_promotion()){
        piece_bb[ROOK] ^= from_bb;
    } else if (move.is_queen_promotion()){
        piece_bb[QUEEN] ^= from_bb;
    }
    pinned_bb = aux_info.pinned;
    _number_checkers = aux_info.number_checkers;
    _checkers = aux_info.checkers;
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

bool Board::is_attacked(const Square origin, const Colour us) const{
    // Want to (semi-efficiently) see if a square is attacked, ignoring pins.
    // First off, if the square is attacked by a knight, it's definitely in check.
    const Colour them = ~us;
    
    if (Bitboards::attacks(KNIGHT, origin) & pieces(them, KNIGHT)) { return true;}
    if (Bitboards::attacks(KING, origin) & pieces(them, KING)) { return true;}
    // Our colour pawn attacks are the same as attacked-by pawn
    if (Bitboards::pawn_attacks(us, origin) & pieces(them, PAWN)) { return true;}

    // Sliding moves.
    // 
    // Sliding attacks should xray the king.
    const Bitboard occ = pieces() ^ pieces(us, KING);

    if ((rook_attacks(occ, origin)) & (pieces(them, ROOK) | pieces(them, QUEEN))) { return true;}
    if ((bishop_attacks(occ, origin)) & (pieces(them, BISHOP) | pieces(them, QUEEN))) { return true;}
    
    // Nothing so far, that's good, no checks.
    return false;
}

void Board::search_kings() {
    for (Square::square_t i = 0; i < 64; i++) {
        if (pieces(i).is_king()) { 
            if (pieces(i).is_white()) {
                king_square[WHITE] = i;
            } else {
                king_square[BLACK] = i;
            }
        }
    }
}


Square Board::find_king(const Colour colour) const{
    return king_square[colour];
}


bool Board::is_pinned(const Square origin) const{
    return pinned_bb & origin;
}

void Board::update_checkers() {
    const Square origin = find_king(whos_move);
    const Colour us = whos_move;
    const Colour them = ~us;
    // Want to (semi-efficiently) see if a square is attacked, ignoring pins.
    // First off, if the square is attacked by a knight, it's definitely in check.
    _number_checkers = 0;
    Bitboard atk = Bitboards::attacks(KNIGHT, origin) & pieces(them, KNIGHT);
    if (atk) { 
        _checkers[_number_checkers] = lsb(atk);
        _number_checkers++;
    }

    atk = Bitboards::pawn_attacks(us, origin) & pieces(them, PAWN);
    if (atk) { 
        _checkers[_number_checkers] = lsb(atk);
        _number_checkers++;
    }

    // Sliding moves.
    
    //int test_c = _number_checkers;
    atk = (rook_attacks(pieces(), origin)) & (pieces(them, ROOK) | pieces(them, QUEEN));
    if (atk) { 
        _checkers[_number_checkers] = lsb(atk);
        _number_checkers++;
    }
    atk = (bishop_attacks(pieces(), origin)) & (pieces(them, BISHOP) | pieces(them, QUEEN));
    if (atk) { 
        _checkers[_number_checkers] = lsb(atk);
        _number_checkers++;
    }

    pinned_bb = 0;
    Bitboard pinner = (rook_xrays(pieces(), origin)) & (pieces(them, ROOK) | pieces(them, QUEEN));
    while (pinner) { 
        Square sq = pop_lsb(&pinner);
        Bitboard pinned = (Bitboards::between(origin, sq) & pieces(us));
        pinned_bb |= pinned;
    }
    pinner = (bishop_xrays(pieces(), origin)) & (pieces(them, BISHOP) | pieces(them, QUEEN));
    while (pinner) { 
        Square sq = pop_lsb(&pinner);
        Bitboard pinned = (Bitboards::between(origin, sq) & pieces(us));
        pinned_bb |= pinned;
    }
    aux_info.is_check = _number_checkers > 0;
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

long int zobrist_table[N_COLOUR][N_PIECE][N_SQUARE];
long int zobrist_table_cr[N_COLOUR][2];
long int zobrist_table_move[N_COLOUR];
long int zobrist_table_ep[8];

void init_zobrist() {
    std::mt19937_64 generator(0x3243f6a8885a308d);
    std::uniform_int_distribution<unsigned long> distribution;
    // Fill table with random bitstrings
    for (int c = 0; c < N_COLOUR; c++) {
        for (int p = 0; p < N_PIECE; p++) {
            for (int sq = 0; sq < N_SQUARE; sq++) {
                zobrist_table[c][p][sq] = distribution(generator);
            }
        }
    }
    // en-passent files
    for (int i = 0; i < 8; i++) {
        zobrist_table_ep[i] = distribution(generator);
    }
    // who's move
    zobrist_table_move[WHITE] = distribution(generator);
    zobrist_table_move[BLACK] = distribution(generator);
    zobrist_table_cr[WHITE][KINGSIDE] = distribution(generator);
    zobrist_table_cr[WHITE][QUEENSIDE] = distribution(generator);
    zobrist_table_cr[BLACK][KINGSIDE] = distribution(generator);
    zobrist_table_cr[BLACK][QUEENSIDE] = distribution(generator);
}

long int hash_zobrist(const Board& board) {
    long int hash = 0;
    for (int sq = 0; sq < N_SQUARE; sq++){
        if (!board.is_free(sq)) {
            Piece p = board.pieces(sq);
            hash ^= zobrist_table[to_enum_colour(p)][to_enum_piece(p)][sq];
        }
    }
    hash ^= zobrist_table_move[board.who_to_play()];
    // Castling rights
    if (board.can_castle(WHITE, KINGSIDE)) {
        hash ^= zobrist_table_cr[WHITE][KINGSIDE];
    }
    if (board.can_castle(WHITE, QUEENSIDE)) {
        hash ^= zobrist_table_cr[WHITE][QUEENSIDE];
    }
    if (board.can_castle(BLACK, KINGSIDE)) {
        hash ^= zobrist_table_cr[BLACK][KINGSIDE];
    }
    if (board.can_castle(BLACK, QUEENSIDE)) {
        hash ^= zobrist_table_cr[BLACK][QUEENSIDE];
    }
    // en-passent
    if (board.en_passent()) {
        hash ^= zobrist_table_ep[board.en_passent().file_index()];
    }
    return hash;
} 

long diff_zobrist(const Move move, const Piece piece) {
    // Calculate the change in hash value caused by a move, without iterating through the entire board.
    long hash = 0;
    // Pieces moving
    // Piece off origin square
    const PieceEnum p = to_enum_piece(piece);
    const Colour c = to_enum_colour(piece);
    hash ^= zobrist_table[c][p][move.origin];
    // Piece to target square
    if (move.is_knight_promotion()) {
        hash ^= zobrist_table[c][KNIGHT][move.target];
    } else if (move.is_bishop_promotion()) {
        hash ^= zobrist_table[c][BISHOP][move.target];
    } else if (move.is_rook_promotion()) {
        hash ^= zobrist_table[c][ROOK][move.target];
    } else if (move.is_queen_promotion()) {
        hash ^= zobrist_table[c][QUEEN][move.target];
    } else {
        hash ^= zobrist_table[c][p][move.target];
    }
    // Captured piece
    if (move.is_ep_capture()) {
        const Square captured_square = move.origin.rank() | move.target.file();
        hash ^= zobrist_table[to_enum_colour(move.captured_peice)][to_enum_piece(move.captured_peice)][captured_square];
    } else if (move.is_capture()) {
        hash ^= zobrist_table[to_enum_colour(move.captured_peice)][to_enum_piece(move.captured_peice)][move.target];
    }
    // How do we do castling rights? hmm
}


long int Board::hash() const{
    return hash_zobrist(*this);
}