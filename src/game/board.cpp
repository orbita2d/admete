#include <map>
#include <iostream>
#include <istream>
#include <sstream>
#include <cstdint> //for uint8_t
#include <stdexcept>
#include <random>

#include "board.hpp"
#include "evaluate.hpp"


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
    ply_counter = 0;
    aux_info = aux_history.begin();
    uint N = fen.length(), board_position;
    uint rank = 0, file = 0;
    char my_char;
    std::array<Piece, N_SQUARE> pieces_array;
    
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

    occupied_bb = 0;
    colour_bb[WHITE] = 0;
    colour_bb[BLACK] = 0;
    for (int i = 0; i < N_PIECE; i++) {
        piece_bb[i] = 0;
    }
    for (uint i = 0; i < 64; i++) {
        if (pieces_array.at(i).is_blank()) { continue; }
        occupied_bb |= sq_to_bb(i);
        if (pieces_array.at(i).is_colour(WHITE)) {
            colour_bb[WHITE] |= sq_to_bb(i);
        } else {
            colour_bb[BLACK] |= sq_to_bb(i);
        }
        piece_bb[to_enum_piece(pieces_array.at(i))] |= sq_to_bb(i);

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

    aux_info->castling_rights[WHITE][KINGSIDE] = false;
    aux_info->castling_rights[WHITE][QUEENSIDE] = false;
    aux_info->castling_rights[BLACK][KINGSIDE] = false;
    aux_info->castling_rights[BLACK][QUEENSIDE] = false;
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
                aux_info->castling_rights[BLACK][QUEENSIDE] = true;
                break;

            case 'Q':
                aux_info->castling_rights[WHITE][QUEENSIDE] = true;
                break;

            case 'k':
                aux_info->castling_rights[BLACK][KINGSIDE] = true;
                break;

            case 'K':
                aux_info->castling_rights[WHITE][KINGSIDE] = true;
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
        aux_info->en_passent_target = 0;
    } else {
        aux_info->en_passent_target = Square(en_passent);
    }

    // Halfmove clock
    aux_info->halfmove_clock = halfmove;
    // Fullmove counter
    fullmove_counter = counter;
    initialise();
};

// Geometry

bool in_line(const Square origin, const Square target){
    return Bitboards::line(origin, target);
}

bool in_line(const Square p1, const Square p2, const Square p3){
    return Bitboards::line(p1, p2) == Bitboards::line(p1, p3);
}

bool interposes(const Square origin, const Square target, const Square query) {
    return Bitboards::between(origin, target) & sq_to_bb(query);
}

void Board::initialise() {
    search_kings();
    update_checkers();
    update_check_squares();
    aux_info->material = count_material(*this);
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
    aux_info->pinned = pinned_bb;
    aux_info->checkers = _checkers;
    aux_info->number_checkers = _number_checkers;
    aux_history[ply_counter + 1] = *aux_info;
    ply_counter ++;
    aux_info = &aux_history[ply_counter];
    const Colour us = whos_move;
    const Colour them = ~ us;
    const PieceEnum p = move.moving_peice;

    if (move.is_capture() | (p == PAWN)) {
        aux_info->halfmove_clock = 0;
    } else {aux_info->halfmove_clock++ ;}
    // Track en-passent square
    if (move.is_double_push()) {
        // Little hacky but this is the square in between.
        aux_info->en_passent_target = (move.origin.get_value() + move.target.get_value())/2;
    } else {
        aux_info->en_passent_target = 0;
    }

    if (p == KING) {
        king_square[us] = move.target;
        aux_info->castling_rights[us][KINGSIDE]  = false;
        aux_info->castling_rights[us][QUEENSIDE]  = false;
    }

    const Bitboard from_bb = sq_to_bb(move.origin);
    const Bitboard to_bb = sq_to_bb(move.target);
    const Bitboard from_to_bb = from_bb ^ to_bb;
    // Castling is special
    if (move.is_king_castle()) {
        const Bitboard rook_from_to_bb = sq_to_bb(RookSquare[us][KINGSIDE]) ^ sq_to_bb(move.origin + Direction::E);
        // Update the bitboard.
        occupied_bb ^= from_to_bb;
        occupied_bb ^= rook_from_to_bb;
        colour_bb[us] ^= from_to_bb;
        colour_bb[us] ^= rook_from_to_bb;
        piece_bb[KING] ^= from_to_bb;
        piece_bb[ROOK] ^= rook_from_to_bb;
        aux_info->castling_rights[us][KINGSIDE] = false;
        aux_info->castling_rights[us][QUEENSIDE] = false;
    } else if (move.is_queen_castle()) {
        const Bitboard rook_from_to_bb = sq_to_bb(RookSquare[us][QUEENSIDE]) ^ sq_to_bb(move.origin + Direction::W);
        // Update the bitboard.
        occupied_bb ^= from_to_bb;
        occupied_bb ^= rook_from_to_bb;
        colour_bb[us] ^= from_to_bb;
        colour_bb[us] ^= rook_from_to_bb;
        piece_bb[KING] ^= from_to_bb;
        piece_bb[ROOK] ^= rook_from_to_bb;
        aux_info->castling_rights[us][KINGSIDE] = false;
        aux_info->castling_rights[us][QUEENSIDE] = false;
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
        move.captured_peice = pieces(captured_square);
    } else if (move.is_capture()){
        // Make sure to lookup and record the piece captured 
        move.captured_peice = pieces(move.target);
        // Update the bitboard.
        occupied_bb ^= from_bb;
        colour_bb[us] ^= from_to_bb;
        colour_bb[them] ^= to_bb;
        piece_bb[p] ^= from_to_bb;
        piece_bb[to_enum_piece(move.captured_peice)] ^= to_bb;
    } else {
        // Quiet move
        occupied_bb ^= from_to_bb;
        colour_bb[us] ^= from_to_bb;
        piece_bb[p] ^= from_to_bb;
    }

    // And now do the promotion if it is one.
    if (move.is_knight_promotion()) {
        piece_bb[PAWN] ^= to_bb;
        piece_bb[KNIGHT] ^= to_bb;
    } else if (move.is_bishop_promotion()){
        piece_bb[PAWN] ^= to_bb;
        piece_bb[BISHOP] ^= to_bb;
    } else if (move.is_rook_promotion()){
        piece_bb[PAWN] ^= to_bb;
        piece_bb[ROOK] ^= to_bb;
    } else if (move.is_queen_promotion()){
        piece_bb[PAWN] ^= to_bb;
        piece_bb[QUEEN] ^= to_bb;
    }
    

    // Check if we've moved our rook.
    if (move.origin == RookSquare[us][KINGSIDE]) {
        aux_info->castling_rights[us][KINGSIDE]  = false;
    } else if (move.origin == RookSquare[us][QUEENSIDE]) {
        aux_info->castling_rights[us][QUEENSIDE]  = false;
    }
    // Check for rook captures.
    if (move.target == RookSquare[them][KINGSIDE]) {
        aux_info->castling_rights[them][KINGSIDE] = false;
    }
    if (move.target == RookSquare[them][QUEENSIDE]) {
        aux_info->castling_rights[them][QUEENSIDE] = false;
    }

    // Switch whos turn it is to play
    whos_move = ~ whos_move;
    update_checkers();
    update_check_squares();

}

void Board::unmake_move(const Move move) {
    // Iterate counters.
    if (whos_move == Colour::WHITE) {fullmove_counter--;}
    ply_counter--;
    aux_info = &aux_history[ply_counter];

    // Switch whos turn it is to play
    whos_move = ~ whos_move;
    Colour us = whos_move;
    Colour them = ~ us;

    const PieceEnum p =  move.moving_peice;
    if (p == KING) {
        king_square[us] = move.origin;
    }

    const Bitboard from_bb = sq_to_bb(move.origin);
    const Bitboard to_bb = sq_to_bb(move.target);
    const Bitboard from_to_bb = from_bb ^ to_bb;
    if (move.is_king_castle()) {
        const Bitboard rook_from_to_bb = sq_to_bb(RookSquare[us][KINGSIDE]) ^ sq_to_bb(move.origin + Direction::E);
        // Update the bitboard.
        occupied_bb ^= from_to_bb;
        occupied_bb ^= rook_from_to_bb;
        colour_bb[us] ^= from_to_bb;
        colour_bb[us] ^= rook_from_to_bb;
        piece_bb[KING] ^= from_to_bb;
        piece_bb[ROOK] ^= rook_from_to_bb;
    } else if (move.is_queen_castle()) {
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
    } else if (move.is_capture()){
        // Make sure to lookup and record the piece captured 
        // Update the bitboard.
        occupied_bb ^= from_bb;
        colour_bb[us] ^= from_to_bb;
        colour_bb[them] ^= to_bb;
        piece_bb[p] ^= from_to_bb;
        piece_bb[to_enum_piece(move.captured_peice)] ^= to_bb;
    } else {
        occupied_bb ^= from_to_bb;
        colour_bb[us] ^= from_to_bb;
        piece_bb[p] ^= from_to_bb;
    }
    // And now do the promotion if it is one.
    // The quiet move method above moved a pawn from to->from
    // We need to remove the promoted piece and add a pawn
    if (move.is_knight_promotion()) {
        piece_bb[KNIGHT] ^= to_bb;
        piece_bb[PAWN] ^= to_bb; 
    } else if (move.is_bishop_promotion()){
        piece_bb[BISHOP] ^= to_bb;
        piece_bb[PAWN] ^= to_bb; 
    } else if (move.is_rook_promotion()){
        piece_bb[ROOK] ^= to_bb;
        piece_bb[PAWN] ^= to_bb; 
    } else if (move.is_queen_promotion()){
        piece_bb[QUEEN] ^= to_bb;
        piece_bb[PAWN] ^= to_bb; 
    }
    pinned_bb = aux_info->pinned;
    _number_checkers = aux_info->number_checkers;
    _checkers = aux_info->checkers;
}

void Board::try_move(const std::string move_sting) {
    bool flag = false;
    std::vector<Move> legal_moves = get_moves();
    for (Move move : legal_moves) {
        if (move_sting == print_move(move, legal_moves)) {
            make_move(move);
            flag = true;
            break;
        } else if (move_sting == move.pretty()){
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
        if (move_sting == move.pretty()) {
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

    if ((rook_attacks(occ, origin)) & (pieces(them, ROOK, QUEEN))) { return true;}
    if ((bishop_attacks(occ, origin)) & (pieces(them, BISHOP, QUEEN))) { return true;}
    
    // Nothing so far, that's good, no checks.
    return false;
}

void Board::search_kings() {
    king_square[WHITE] = lsb(pieces(WHITE, KING));
    king_square[BLACK] = lsb(pieces(BLACK, KING));
}


Square Board::find_king(const Colour us) const{
    return king_square[us];
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

    const Bitboard occ = pieces();

    Bitboard atk = Bitboards::attacks(KNIGHT, origin) & pieces(them, KNIGHT);
    atk |= Bitboards::pawn_attacks(us, origin) & pieces(them, PAWN);
    atk |= rook_attacks(occ, origin) & (pieces(them, ROOK, QUEEN));
    atk |= bishop_attacks(occ, origin) & (pieces(them, BISHOP, QUEEN));
    while (atk) {
        _checkers[_number_checkers] = pop_lsb(&atk);
        _number_checkers++;
    }

    pinned_bb = 0;
    Bitboard pinner = rook_xrays(occ, origin) & pieces(them, ROOK, QUEEN);
    pinner |=   bishop_xrays(occ, origin) & pieces(them, BISHOP, QUEEN);
    while (pinner) { 
        Square sq = pop_lsb(&pinner);
        Bitboard pinned = (Bitboards::between(origin, sq) & pieces(us));
        pinned_bb |= pinned;
    }
    aux_info->is_check = _number_checkers > 0;
}

void Board::update_check_squares() {
    const Colour us = whos_move;
    const Colour them = ~us;
    const Square origin = find_king(them);

    const Bitboard occ = pieces();

    // Sqaures for direct checks
    aux_info->check_squares[PAWN] = Bitboards::pawn_attacks(them, origin);
    aux_info->check_squares[KNIGHT] = Bitboards::attacks(KNIGHT, origin);
    aux_info->check_squares[BISHOP] = bishop_attacks(occ, origin);
    aux_info->check_squares[ROOK] = rook_attacks(occ, origin);
    aux_info->check_squares[QUEEN] = aux_info->check_squares[BISHOP] | aux_info->check_squares[ROOK];
    aux_info->check_squares[KING] = 0;

    // Blockers
    Bitboard blk = 0;
    Bitboard pinner = rook_xrays(occ, origin) & pieces(us, ROOK, QUEEN);
    pinner |=   bishop_xrays(occ, origin) & pieces(us, BISHOP, QUEEN);
    while (pinner) { 
        Square sq = pop_lsb(&pinner);
        Bitboard pinned = (Bitboards::between(origin, sq) & pieces(us));
        blk |= pinned;
    }
    aux_info->blockers = blk;
}

bool Board::gives_check(Move move){
    // Does a move give check?
    // Check for direct check.
    if (check_squares(move.moving_peice) & move.target) {
        return true;
    }
    Square ks = find_king(~whos_move);
    // Check for discovered check
    if (blockers() & move.origin) {
        // A blocker moved, does it still block the same ray?
        if (!in_line(move.origin, ks, move.target)) { return true;} 
    }

    if (move.is_promotion()) {
        if (check_squares(get_promoted(move)) & move.target) { return true; }
    } else if (move.is_king_castle()) {
        // The only piece that could give check here is the rook.
        if (check_squares(ROOK) & RookSquare[who_to_play()][KINGSIDE]) { return true; }
    } else if (move.is_queen_castle()) {
        if (check_squares(ROOK) & RookSquare[who_to_play()][QUEENSIDE]) { return true; }
    } else if (move.is_ep_capture()) {
        // If the en passent reveals a file, this will be handled by the blocker
        // The only way this could be problematic is if the en-passent unblocks a rank.
        if ( move.origin.rank() == ks.rank()) {
            Bitboard mask = Bitboards::line(ks, move.origin);
            // Look for a rook in the rank
            Bitboard occ = pieces();
            // Rook attacks from the king
            Bitboard atk = rook_attacks(occ, ks);
            // Rook xrays from the king
            atk = rook_attacks(occ ^ (occ & atk), ks);
            // Rook double xrays from the king
            atk = rook_attacks(occ ^ (occ & atk), ks);
            atk &= mask;
            if (atk & pieces(who_to_play(), ROOK, QUEEN)) {
                return true; 
            }
        }
    }

    return false;
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

void Zorbist::init() {
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

long int Zorbist::hash(const Board& board) {
    long int hash = 0;

    for (int p = 0; p < N_PIECE; p++) {
        Bitboard occ = board.pieces(WHITE, (PieceEnum)p);
        while (occ) {
            Square sq = pop_lsb(&occ);
            hash ^= zobrist_table[WHITE][p][sq];
        }
        occ = board.pieces(BLACK, (PieceEnum)p);
        while (occ) {
            Square sq = pop_lsb(&occ);
            hash ^= zobrist_table[BLACK][p][sq];
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


Piece Board::pieces(const Square sq) const{
    Bitboard square_bb = sq_to_bb(sq);
    for (int p = 0; p < N_PIECE; p++) {
        Bitboard occ = pieces(WHITE, (PieceEnum) p);
        if (square_bb & occ) {
            return Piece(WHITE, (PieceEnum) p);
        }
        occ = pieces(BLACK, (PieceEnum) p);
        if (square_bb & occ) {
            return Piece(BLACK, (PieceEnum) p);
        }
    }
    return Pieces::Blank;
}

long int Board::hash() const{
    return Zorbist::hash(*this);
}