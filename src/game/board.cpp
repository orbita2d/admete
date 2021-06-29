#include "board.hpp"
#include "evaluate.hpp"
#include <assert.h>
#include <cstdint> //for uint8_t
#include <iostream>
#include <istream>
#include <map>
#include <sstream>
#include <stdexcept>

std::map<char, Piece> fen_decode_map = {
    {'p', Piece(BLACK, PAWN)},   {'n', Piece(BLACK, KNIGHT)}, {'b', Piece(BLACK, BISHOP)}, {'r', Piece(BLACK, ROOK)},
    {'q', Piece(BLACK, QUEEN)},  {'k', Piece(BLACK, KING)},   {'P', Piece(WHITE, PAWN)},   {'N', Piece(WHITE, KNIGHT)},
    {'B', Piece(WHITE, BISHOP)}, {'R', Piece(WHITE, ROOK)},   {'Q', Piece(WHITE, QUEEN)},  {'K', Piece(WHITE, KING)}};

void Board::fen_decode(const std::string &fen) {
    ply_counter = 0;
    aux_info = aux_history.begin();
    uint N = fen.length(), board_position;
    uint rank = 0, file = 0;
    char my_char;

    // Reset board.
    occupied_bb = 0;
    colour_bb[WHITE] = 0;
    colour_bb[BLACK] = 0;
    for (int i = 0; i < N_PIECE; i++) {
        piece_bb[i] = 0;
    }

    std::stringstream stream;
    // First, go through the board position part of the fen string.
    for (uint i = 0; i < N; i++) {
        my_char = fen[i];
        if (my_char == '/') {
            rank++;
            file = 0;
            continue;
        }
        if (isdigit(my_char)) {
            file += (my_char - '0');
            continue;
        }
        if (my_char == ' ') {
            // Space is at the end of the board position section.
            board_position = i;
            break;
        }
        // Otherwise should be a character for a piece
        const Bitboard square_bb = sq_to_bb(Square(rank, file));
        const Piece p = fen_decode_map[my_char];
        piece_bb[p.get_piece()] |= square_bb;
        colour_bb[p.get_colour()] |= square_bb;
        occupied_bb |= square_bb;
        file++;
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
    switch (side_to_move[0]) {
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
            switch (castling[i]) {
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

bool in_line(const Square origin, const Square target) { return Bitboards::line(origin, target); }

bool in_line(const Square p1, const Square p2, const Square p3) {
    return Bitboards::line(p1, p2) == Bitboards::line(p1, p3);
}

bool interposes(const Square origin, const Square target, const Square query) {
    return Bitboards::between(origin, target) & sq_to_bb(query);
}

void Board::initialise() {
    search_kings();
    update_checkers();
    update_check_squares();
    aux_info->material = Evaluation::count_material(*this);
    hash_history[0] = Zorbist::hash(*this);

    for (int c = WHITE; c < N_COLOUR; c++) {
        for (int p = 0; p < N_PIECE; p++) {
            piece_counts[c][p] = count_bits(pieces((Colour)c, (PieceType)p));
        }
    }
    pawn_atk_bb[WHITE] = Bitboards::pawn_attacks(WHITE, pieces(WHITE, PAWN));
    pawn_atk_bb[BLACK] = Bitboards::pawn_attacks(BLACK, pieces(BLACK, PAWN));
    set_root();
}

bool Board::is_free(const Square target) const { return (occupied_bb & target) == 0; };

bool Board::is_colour(const Colour c, const Square target) const { return (colour_bb[c] & target) != 0; };

void Board::make_move(Move &move) {
    // Iterate counters.
    if (whos_move == Colour::BLACK) {
        fullmove_counter++;
    }
    aux_history[ply_counter + 1] = *aux_info;
    ply_counter++;
    aux_info = &aux_history[ply_counter];
    const Colour us = whos_move;
    const Colour them = ~us;
    const PieceType p = move.moving_piece;
    assert(p != NO_PIECE);
    int last_ep_file = en_passent() ? en_passent().file_index() : -1;

    if (move.is_capture() | (p == PAWN)) {
        aux_info->halfmove_clock = 0;
    } else {
        aux_info->halfmove_clock++;
    }

    // Track en-passent square
    if (move.is_double_push()) {
        // Little hacky but this is the square in between.
        aux_info->en_passent_target = (move.origin.get_value() + move.target.get_value()) / 2;
    } else {
        aux_info->en_passent_target = 0;
    }
    std::array<std::array<bool, N_COLOUR>, N_CASTLE> castling_rights_change = {{{{false, false}}, {{false, false}}}};
    if (p == KING) {
        king_square[us] = move.target;
        if (can_castle(us, KINGSIDE)) {
            castling_rights_change[us][KINGSIDE] = true;
        }
        if (can_castle(us, QUEENSIDE)) {
            castling_rights_change[us][QUEENSIDE] = true;
        }
        aux_info->castling_rights[us][KINGSIDE] = false;
        aux_info->castling_rights[us][QUEENSIDE] = false;
    }

    const Bitboard from_bb = sq_to_bb(move.origin);
    const Bitboard to_bb = sq_to_bb(move.target);
    const Bitboard from_to_bb = from_bb ^ to_bb;

    if (move.is_king_castle()) {
        const Bitboard rook_from_to_bb = sq_to_bb(RookSquare[us][KINGSIDE]) ^ sq_to_bb(move.origin + Direction::E);
        occupied_bb ^= from_to_bb;
        occupied_bb ^= rook_from_to_bb;
        colour_bb[us] ^= from_to_bb;
        colour_bb[us] ^= rook_from_to_bb;
        piece_bb[KING] ^= from_to_bb;
        piece_bb[ROOK] ^= rook_from_to_bb;

    } else if (move.is_queen_castle()) {
        const Bitboard rook_from_to_bb = sq_to_bb(RookSquare[us][QUEENSIDE]) ^ sq_to_bb(move.origin + Direction::W);
        occupied_bb ^= from_to_bb;
        occupied_bb ^= rook_from_to_bb;
        colour_bb[us] ^= from_to_bb;
        colour_bb[us] ^= rook_from_to_bb;
        piece_bb[KING] ^= from_to_bb;
        piece_bb[ROOK] ^= rook_from_to_bb;

    } else if (move.is_ep_capture()) {
        const Square captured_square = move.origin.rank() | move.target.file();
        occupied_bb ^= from_to_bb;
        occupied_bb ^= sq_to_bb(captured_square);
        colour_bb[us] ^= from_to_bb;
        colour_bb[them] ^= sq_to_bb(captured_square);
        piece_bb[PAWN] ^= from_to_bb;

        piece_bb[PAWN] ^= sq_to_bb(captured_square);
        // Make sure to lookup and record the piece captured
        move.captured_piece = PAWN;
        piece_counts[them][PAWN]--;

    } else if (move.is_capture()) {
        // Make sure to lookup and record the piece captured
        move.captured_piece = piece_type(move.target);
        // Update the bitboard.
        occupied_bb ^= from_bb;
        colour_bb[us] ^= from_to_bb;
        colour_bb[them] ^= to_bb;
        piece_bb[p] ^= from_to_bb;
        piece_bb[move.captured_piece] ^= to_bb;
        piece_counts[them][move.captured_piece]--;

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
        piece_counts[us][PAWN]--;
        piece_counts[us][KNIGHT]++;
    } else if (move.is_bishop_promotion()) {
        piece_bb[PAWN] ^= to_bb;
        piece_bb[BISHOP] ^= to_bb;
        piece_counts[us][PAWN]--;
        piece_counts[us][BISHOP]++;
    } else if (move.is_rook_promotion()) {
        piece_bb[PAWN] ^= to_bb;
        piece_bb[ROOK] ^= to_bb;
        piece_counts[us][PAWN]--;
        piece_counts[us][ROOK]++;
    } else if (move.is_queen_promotion()) {
        piece_bb[PAWN] ^= to_bb;
        piece_bb[QUEEN] ^= to_bb;
        piece_counts[us][PAWN]--;
        piece_counts[us][QUEEN]++;
    }

    // Check if we've moved our rook to update our castling rights.
    if ((move.origin == RookSquare[us][KINGSIDE]) & can_castle(us, KINGSIDE)) {
        aux_info->castling_rights[us][KINGSIDE] = false;
        castling_rights_change[us][KINGSIDE] = true;
    } else if ((move.origin == RookSquare[us][QUEENSIDE]) & can_castle(us, QUEENSIDE)) {
        aux_info->castling_rights[us][QUEENSIDE] = false;
        castling_rights_change[us][QUEENSIDE] = true;
    }

    // Check for rook captures to update their castling rights
    if (move.is_capture()) {
        if ((move.target == RookSquare[them][KINGSIDE]) & can_castle(them, KINGSIDE)) {
            aux_info->castling_rights[them][KINGSIDE] = false;
            castling_rights_change[them][KINGSIDE] = true;
        }
        if ((move.target == RookSquare[them][QUEENSIDE]) & can_castle(them, QUEENSIDE)) {
            aux_info->castling_rights[them][QUEENSIDE] = false;
            castling_rights_change[them][QUEENSIDE] = true;
        }
    }

    // Checks that the piece counts (updated incrementally) are being tracked properly
    for (int c = WHITE; c < N_COLOUR; c++) {
        for (int p = 0; p < N_PIECE; p++) {
            assert(count_pieces((Colour)c, (PieceType)p) == count_bits(pieces((Colour)c, (PieceType)p)));
        }
    }

    // Switch whos turn it is to play
    whos_move = ~whos_move;

    // Update the various data structures that are computed for the position
    update_checkers();
    update_check_squares();

    // Precompute the pawn attacks bitboards.
    if ((p == PAWN) | (move.captured_piece == PAWN)) {
        pawn_atk_bb[WHITE] = Bitboards::pawn_attacks(WHITE, pieces(WHITE, PAWN));
        pawn_atk_bb[BLACK] = Bitboards::pawn_attacks(BLACK, pieces(BLACK, PAWN));
    }

    assert(pawn_controlled(WHITE) == Bitboards::pawn_attacks(WHITE, pieces(WHITE, PAWN)));
    assert(pawn_controlled(BLACK) == Bitboards::pawn_attacks(BLACK, pieces(BLACK, PAWN)));

    // Update the zorbist hash
    hash_history[ply_counter] =
        hash_history[ply_counter - 1] ^ Zorbist::diff(move, us, last_ep_file, castling_rights_change);

    assert(hash() == Zorbist::hash(*this));
}

void Board::unmake_move(const Move move) {
    // Iterate counters
    if (whos_move == Colour::WHITE) {
        fullmove_counter--;
    }
    ply_counter--;
    aux_info = &aux_history[ply_counter];

    // Switch whos turn it is to play
    whos_move = ~whos_move;
    Colour us = whos_move;
    Colour them = ~us;

    const PieceType p = move.moving_piece;
    const PieceType cp = move.captured_piece;
    assert(p != NO_PIECE);

    if (p == KING) {
        king_square[us] = move.origin;
    }

    const Bitboard from_bb = sq_to_bb(move.origin);
    const Bitboard to_bb = sq_to_bb(move.target);
    const Bitboard from_to_bb = from_bb ^ to_bb;

    if (move.is_king_castle()) {
        const Bitboard rook_from_to_bb = sq_to_bb(RookSquare[us][KINGSIDE]) ^ sq_to_bb(move.origin + Direction::E);
        occupied_bb ^= from_to_bb;
        occupied_bb ^= rook_from_to_bb;
        colour_bb[us] ^= from_to_bb;
        colour_bb[us] ^= rook_from_to_bb;
        piece_bb[KING] ^= from_to_bb;
        piece_bb[ROOK] ^= rook_from_to_bb;

    } else if (move.is_queen_castle()) {
        const Bitboard rook_from_to_bb = sq_to_bb(RookSquare[us][QUEENSIDE]) ^ sq_to_bb(move.origin + Direction::W);
        occupied_bb ^= from_to_bb;
        occupied_bb ^= rook_from_to_bb;
        colour_bb[us] ^= from_to_bb;
        colour_bb[us] ^= rook_from_to_bb;
        piece_bb[KING] ^= from_to_bb;
        piece_bb[ROOK] ^= rook_from_to_bb;

    } else if (move.is_ep_capture()) {
        const Square captured_square = move.origin.rank() | move.target.file();
        occupied_bb ^= from_to_bb;
        occupied_bb ^= sq_to_bb(captured_square);
        colour_bb[us] ^= from_to_bb;
        colour_bb[them] ^= sq_to_bb(captured_square);
        piece_bb[PAWN] ^= from_to_bb;
        piece_bb[PAWN] ^= sq_to_bb(captured_square);

        // Update piece counts
        piece_counts[them][PAWN]++;

    } else if (move.is_capture()) {
        assert(move.captured_piece != NO_PIECE);
        // Update the bitboards.
        occupied_bb ^= from_bb;
        colour_bb[us] ^= from_to_bb;
        colour_bb[them] ^= to_bb;
        piece_bb[p] ^= from_to_bb;
        piece_bb[move.captured_piece] ^= to_bb;
        // Update piece counts
        piece_counts[them][move.captured_piece]++;
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
        piece_counts[us][PAWN]++;
        piece_counts[us][KNIGHT]--;
    } else if (move.is_bishop_promotion()) {
        piece_bb[BISHOP] ^= to_bb;
        piece_bb[PAWN] ^= to_bb;
        piece_counts[us][PAWN]++;
        piece_counts[us][BISHOP]--;
    } else if (move.is_rook_promotion()) {
        piece_bb[ROOK] ^= to_bb;
        piece_bb[PAWN] ^= to_bb;
        piece_counts[us][PAWN]++;
        piece_counts[us][ROOK]--;
    } else if (move.is_queen_promotion()) {
        piece_bb[QUEEN] ^= to_bb;
        piece_bb[PAWN] ^= to_bb;
        piece_counts[us][PAWN]++;
        piece_counts[us][QUEEN]--;
    }

    if ((p == PAWN) | (cp == PAWN)) {
        pawn_atk_bb[WHITE] = Bitboards::pawn_attacks(WHITE, pieces(WHITE, PAWN));
        pawn_atk_bb[BLACK] = Bitboards::pawn_attacks(BLACK, pieces(BLACK, PAWN));
    }
}

void Board::make_nullmove() {
    // Iterate counters
    if (whos_move == Colour::BLACK) {
        fullmove_counter++;
    }
    aux_history[ply_counter + 1] = *aux_info;
    ply_counter++;
    aux_info = &aux_history[ply_counter];
    const Colour us = whos_move;
    int last_ep_file = en_passent() ? en_passent().file_index() : -1;

    aux_info->halfmove_clock++;

    // Track en-passent square
    aux_info->en_passent_target = 0;

    // Switch whos turn it is to play
    whos_move = ~whos_move;

    // Update the various data structures that are computed for the position
    update_checkers();
    update_check_squares();

    // Update the zorbist hash
    hash_history[ply_counter] = hash_history[ply_counter - 1] ^ Zorbist::nulldiff(us, last_ep_file);
    assert(hash() == Zorbist::hash(*this));
}

void Board::unmake_nullmove() {
    // Iterate counters
    if (whos_move == Colour::WHITE) {
        fullmove_counter--;
    }
    ply_counter--;
    aux_info = &aux_history[ply_counter];

    // Switch whos turn it is to play
    whos_move = ~whos_move;
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

bool Board::is_attacked(const Square origin, const Colour us) const {
    // Checks a square is attacked, ignoring pins.
    // That is, if moving the king there would result in check.
    // First off, if the square is attacked by a knight, it's definitely in check.
    const Colour them = ~us;

    if (Bitboards::pseudo_attacks(KNIGHT, origin) & pieces(them, KNIGHT)) {
        return true;
    }
    if (Bitboards::pseudo_attacks(KING, origin) & pieces(them, KING)) {
        return true;
    }
    // Our colour pawn attacks are the same as attacked-by pawn
    if (Bitboards::pawn_attacks(us, origin) & pieces(them, PAWN)) {
        return true;
    }

    // Sliding moves.
    //
    // Sliding attacks should xray the king.
    const Bitboard occ = pieces() ^ pieces(us, KING);

    if ((rook_attacks(occ, origin)) & (pieces(them, ROOK, QUEEN))) {
        return true;
    }
    if ((bishop_attacks(occ, origin)) & (pieces(them, BISHOP, QUEEN))) {
        return true;
    }

    // Nothing so far, that's good, no checks.
    return false;
}

void Board::search_kings() {
    king_square[WHITE] = lsb(pieces(WHITE, KING));
    king_square[BLACK] = lsb(pieces(BLACK, KING));
}

Square Board::find_king(const Colour us) const { return king_square[us]; }

bool Board::is_pinned(const Square origin) const { return pinned() & origin; }

void Board::update_checkers() {
    // Looks in the current position if the king (of the player to move) is in check, and saves where those checks are.
    // It also looks up what pieces are pinned, which is used in move generation.
    const Square origin = find_king(whos_move);
    const Colour us = whos_move;
    const Colour them = ~us;
    aux_info->number_checkers = 0;

    const Bitboard occ = pieces();

    Bitboard atk = Bitboards::pseudo_attacks(KNIGHT, origin) & pieces(them, KNIGHT);
    atk |= Bitboards::pawn_attacks(us, origin) & pieces(them, PAWN);
    atk |= rook_attacks(occ, origin) & (pieces(them, ROOK, QUEEN));
    atk |= bishop_attacks(occ, origin) & (pieces(them, BISHOP, QUEEN));
    while (atk) {
        aux_info->checkers[aux_info->number_checkers] = pop_lsb(&atk);
        aux_info->number_checkers++;
    }

    aux_info->pinned = 0;
    Bitboard pinner = rook_xrays(occ, origin) & pieces(them, ROOK, QUEEN);
    pinner |= bishop_xrays(occ, origin) & pieces(them, BISHOP, QUEEN);
    while (pinner) {
        Square sq = pop_lsb(&pinner);
        Bitboard pinned = (Bitboards::between(origin, sq) & pieces(us));
        aux_info->pinned |= pinned;
    }
    aux_info->is_check = aux_info->number_checkers > 0;
}

void Board::update_check_squares() {
    // Looks at what piece placements would put the enemy king in check. For instance, what squares a bishop could be on
    // and give check. Also looks at what pieces are blocking checks, such that if they moved they could cause a
    // discovered check.
    const Colour us = whos_move;
    const Colour them = ~us;
    const Square origin = find_king(them);

    const Bitboard occ = pieces();

    // Sqaures for direct checks
    aux_info->check_squares[PAWN] = Bitboards::pawn_attacks(them, origin);
    aux_info->check_squares[KNIGHT] = Bitboards::pseudo_attacks(KNIGHT, origin);
    aux_info->check_squares[BISHOP] = bishop_attacks(occ, origin);
    aux_info->check_squares[ROOK] = rook_attacks(occ, origin);
    aux_info->check_squares[QUEEN] = aux_info->check_squares[BISHOP] | aux_info->check_squares[ROOK];
    aux_info->check_squares[KING] = 0;

    // Blockers
    Bitboard blk = 0;
    Bitboard pinner = rook_xrays(occ, origin) & pieces(us, ROOK, QUEEN);
    pinner |= bishop_xrays(occ, origin) & pieces(us, BISHOP, QUEEN);
    while (pinner) {
        Square sq = pop_lsb(&pinner);
        Bitboard pinned = (Bitboards::between(origin, sq) & pieces(us));
        blk |= pinned;
    }
    aux_info->blockers = blk;
}

bool Board::gives_check(const Move move) {
    // Returns true if a given move would cause check in the current position. Relies on the information generated by
    // update_check_squares(); Check for direct check.
    if (check_squares(move.moving_piece) & move.target) {
        return true;
    }
    Square ks = find_king(~whos_move);
    // Check for discovered check
    if (blockers() & move.origin) {
        // A blocker moved, does it still block the same ray?
        if (!in_line(move.origin, ks, move.target)) {
            return true;
        }
    }

    if (move.is_promotion()) {
        if (check_squares(get_promoted(move)) & move.target) {
            return true;
        }
    } else if (move.is_king_castle()) {
        // The only piece that could give check here is the rook.
        if (check_squares(ROOK) & RookSquare[who_to_play()][KINGSIDE]) {
            return true;
        }
    } else if (move.is_queen_castle()) {
        if (check_squares(ROOK) & RookSquare[who_to_play()][QUEENSIDE]) {
            return true;
        }
    } else if (move.is_ep_capture()) {
        // If the en passent reveals a file, this will be handled by the blocker
        // The only way this could be problematic is if the en-passent unblocks a
        // rank.
        if (move.origin.rank() == ks.rank()) {
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

PieceType Board::piece_type(const Square sq) const {
    Bitboard square_bb = sq_to_bb(sq);
    for (int p = 0; p < N_PIECE; p++) {
        Bitboard occ = pieces((PieceType)p);
        if (square_bb & occ) {
            return (PieceType)p;
        }
    }
    return NO_PIECE;
}

Piece Board::pieces(const Square sq) const {
    Bitboard square_bb = sq_to_bb(sq);
    for (int p = 0; p < N_PIECE; p++) {
        Bitboard occ = pieces(WHITE, (PieceType)p);
        if (square_bb & occ) {
            return Piece(WHITE, (PieceType)p);
        }
        occ = pieces(BLACK, (PieceType)p);
        if (square_bb & occ) {
            return Piece(BLACK, (PieceType)p);
        }
    }
    return Piece(WHITE, NO_PIECE);
}

bool Board::is_draw() const {
    // Check to see if current position is draw by repetition
    long current = hash_history[ply_counter];
    int repetition_before_root = 0;
    int repetition_after_root = 0;

    // Look for draw by repetition
    for (unsigned int i = 0; i < get_root(); i++) {
        if (hash_history[i] == current) {
            repetition_before_root++;
        }
    }
    for (unsigned int i = get_root(); i < ply_counter; i++) {
        if (hash_history[i] == current) {
            repetition_after_root++;
        }
    }
    if (repetition_after_root > 0) {
        // Consider any repeating moves after the root to be a draw
        return true;
    }
    if (repetition_before_root + repetition_after_root > 1) {
        // 2 repetitions is 3 fold draw
        return true;
    }

    // 50 move rule
    if (halfmove_clock() >= 100) {
        return true;
    }

    // Check draw by insufficient material
    if (pieces(QUEEN)) {
        return false;
    } else if (pieces(ROOK)) {
        return false;
    } else if (pieces(PAWN)) {
        return false;
    } else if (count_pieces(WHITE, BISHOP) > 1) {
        return false;
    } else if (count_pieces(WHITE, KNIGHT) > 2) {
        return false;
    } else if ((count_pieces(WHITE, KNIGHT) >= 1) & (count_pieces(WHITE, BISHOP) >= 1)) {
        return false;
    } else if (count_pieces(BLACK, BISHOP) > 1) {
        return false;
    } else if (count_pieces(BLACK, KNIGHT) > 2) {
        return false;
    } else if ((count_pieces(BLACK, KNIGHT) >= 1) & (count_pieces(BLACK, BISHOP) >= 1)) {
        return false;
    } else {
        return true;
    }
};

ply_t Board::repetitions(const ply_t start) const {
    // Check to see if current position is draw by repetition
    long current = hash_history[ply_counter];
    ply_t n = 0;

    for (unsigned int i = start; i < ply_counter; i++) {
        if (hash_history[i] == current) {
            n++;
        }
    }
    return n;
}

bool Board::is_endgame() const { return Evaluation::count_material(*this) < Evaluation::ENDGAME_MATERIAL; }

void Board::flip() {
    whos_move = ~whos_move;
    occupied_bb = Bitboards::flip_vertical(occupied_bb);
    colour_bb[WHITE] = Bitboards::flip_vertical(colour_bb[WHITE]);
    colour_bb[BLACK] = Bitboards::flip_vertical(colour_bb[BLACK]);
    std::swap(colour_bb[WHITE], colour_bb[BLACK]);
    for (int p = PAWN; p < N_PIECE; p++) {
        piece_bb[p] = Bitboards::flip_vertical(piece_bb[p]);
    }

    std::swap(aux_info->castling_rights[WHITE], aux_info->castling_rights[BLACK]);
    initialise();
}