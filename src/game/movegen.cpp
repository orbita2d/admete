#include "movegen.hpp"
#include "board.hpp"

// Move Generation

void add_pawn_promotions(const Move move, MoveList &moves) {
    // Add all variations of promotions to a move.
    Move my_move = move;
    my_move.make_queen_promotion();
    moves.push_back(my_move);
    my_move.make_rook_promotion();
    moves.push_back(my_move);
    my_move.make_knight_promotion();
    moves.push_back(my_move);
    my_move.make_bishop_promotion();
    moves.push_back(my_move);
}

template<Colour colour>
void get_pawn_moves(const Board &board, const Square origin, MoveList &quiet_moves, MoveList &captures){
    Square target;
    Move move;
    // Moves are North
    // Look for double pawn push possibility
    if (origin.rank() == relative_rank(colour, Squares::Rank2)) {
        target = origin + (forwards(colour) + forwards(colour) );
        if (board.is_free(target) & board.is_free(origin + forwards(colour) )) {
            move = Move(origin, target);
            move.make_double_push();
            quiet_moves.push_back(move);
        }
    }
    // Normal pushes.
    target = origin + (forwards(colour) );
    if (board.is_free(target)) {
        move = Move(origin, target);
        if (origin.rank() == relative_rank(colour, Squares::Rank7)) {
            add_pawn_promotions(move, quiet_moves);
        } else {
            quiet_moves.push_back(move);
        }
    }
    // Normal captures.
    if (origin.to_west() != 0) {
        target = origin + (forwards(colour) + Direction::W);
        if (board.is_colour(~colour, target)) {
            move = Move(origin, target);
            move.make_capture();
            if (origin.rank() == relative_rank(colour, Squares::Rank7)) {
                add_pawn_promotions(move, captures);
            } else {
                captures.push_back(move);
            }
        }
    }

    if (origin.to_east() != 0) {
        target = origin + (forwards(colour) + Direction::E);
        if (board.is_colour(~colour, target)) {
            move = Move(origin, target);
            move.make_capture();
            if (origin.rank() == relative_rank(colour, Squares::Rank7)) {
                add_pawn_promotions(move, captures);
            } else {
                captures.push_back(move);
            }
        }
    }
    // En-passent
    if (origin.rank() == relative_rank(colour, Squares::Rank5)) {
        if (((origin.to_west() != 0) & (board.aux_info.en_passent_target == Square(origin + forwards(colour) + Direction::W))) | 
            ((origin.to_east() != 0) & (board.aux_info.en_passent_target == Square(origin + forwards(colour) + Direction::E))) ) {
            move = Move(origin, board.aux_info.en_passent_target); 
            move.make_en_passent();
            captures.push_back(move);
        }
    }
}


template<Colour us>
void get_rook_moves(const Board &board, const Square origin, MoveList &quiet_moves, MoveList &captures) {
    const Colour them = ~us;
    const Bitboard atk = rook_attacks(board.pieces(), origin);
    Bitboard quiet_bb = atk & ~board.pieces();
    Bitboard capt_bb = atk & board.pieces(them);
    while (quiet_bb) {
        Square sq = pop_lsb(&quiet_bb);
        Move move = Move(origin, sq);
        quiet_moves.push_back(move);
    }
    while (capt_bb) {
        Square sq = pop_lsb(&capt_bb);
        Move move = Move(origin, sq);
        move.make_capture();
        captures.push_back(move);
    }
}


template<Colour us>
void get_bishop_moves(const Board &board, const Square origin, MoveList &quiet_moves, MoveList &captures) {
    const Colour them = ~us;
    const Bitboard atk = bishop_attacks(board.pieces(), origin);
    Bitboard quiet_bb = atk & ~board.pieces();
    Bitboard capt_bb = atk & board.pieces(them);
    while (quiet_bb) {
        Square sq = pop_lsb(&quiet_bb);
        Move move = Move(origin, sq);
        quiet_moves.push_back(move);
    }
    while (capt_bb) {
        Square sq = pop_lsb(&capt_bb);
        Move move = Move(origin, sq);
        move.make_capture();
        captures.push_back(move);
    }
}

template<Colour us>
void get_queen_moves(const Board &board, const Square origin, MoveList &quiet_moves, MoveList &captures) {
    // Queen moves are the union superset of rook and bishop moves
    const Colour them = ~us;
    const Bitboard atk = bishop_attacks(board.pieces(), origin) | rook_attacks(board.pieces(), origin);
    Bitboard quiet_bb = atk & ~board.pieces();
    Bitboard capt_bb = atk & board.pieces(them);
    while (quiet_bb) {
        Square sq = pop_lsb(&quiet_bb);
        Move move = Move(origin, sq);
        quiet_moves.push_back(move);
    }
    while (capt_bb) {
        Square sq = pop_lsb(&capt_bb);
        Move move = Move(origin, sq);
        move.make_capture();
        captures.push_back(move);
    }
}


template<Colour colour>
void get_castle_moves(const Board &board, MoveList &moves) {
    Move move;
    // You can't castle through check, or while in check
    if (board.aux_info.castling_rights[colour][QUEENSIDE]) 
    {
        if (!(Bitboards::castle(colour, QUEENSIDE) & board.pieces())) {
            if (!board.is_attacked(Squares::FileD | back_rank(colour), colour)
                & !board.is_attacked(Squares::FileC | back_rank(colour), colour)) 
            {
                move = Move(Squares::FileE | back_rank(colour), Squares::FileC | back_rank(colour));
                move.make_queen_castle();
                moves.push_back(move);
            }
        }
    }
    if (board.aux_info.castling_rights[colour][KINGSIDE]) 
    {
        if (!(Bitboards::castle(colour, KINGSIDE) & board.pieces())) {
            if (!board.is_attacked(Squares::FileF | back_rank(colour), colour)
                & !board.is_attacked(Squares::FileG | back_rank(colour), colour)) 
            {
                move = Move(Squares::FileE | back_rank(colour), Squares::FileG | back_rank(colour));
                move.make_king_castle();
                moves.push_back(move);
            }
        }
    }
}

template<Colour us>
void get_king_moves(const Board &board, const Square origin, MoveList &quiet_moves, MoveList &captures) {
    const Colour them = ~us;
    // We should really be careful that we aren't moving into check here.
    // Look to see if we are on an edge.
    const Bitboard my_attacks = Bitboards::attacks(KING, origin);
    Bitboard quiet_bb = my_attacks & ~board.pieces();
    Bitboard capt_bb = my_attacks & board.pieces(them);
    while (quiet_bb) {
        Square sq = pop_lsb(&quiet_bb);
        Move move = Move(origin, sq);
        quiet_moves.push_back(move);
    }
    while (capt_bb) {
        Square sq = pop_lsb(&capt_bb);
        Move move = Move(origin, sq);
        move.make_capture();
        captures.push_back(move);
    }
}

template<Colour us>
void get_knight_moves(const Board &board, const Square origin, MoveList &quiet_moves, MoveList &captures) {
    const Colour them = ~us;
    const Bitboard my_attacks = Bitboards::attacks(KNIGHT, origin);
    Bitboard quiet_bb = my_attacks & ~board.pieces();
    Bitboard capt_bb = my_attacks & board.pieces(them);
    while (quiet_bb) {
        Square sq = pop_lsb(&quiet_bb);
        Move move = Move(origin, sq);
        quiet_moves.push_back(move);
    }
    while (capt_bb) {
        Square sq = pop_lsb(&capt_bb);
        Move move = Move(origin, sq);
        move.make_capture();
        captures.push_back(move);
    }
};

template<Colour colour>
void generate_pseudolegal_moves(const Board &board, MoveList &quiet_moves, MoveList &captures) {
    Bitboard occ;
    occ = board.pieces(colour, PAWN);
    while (occ) {
        Square sq = pop_lsb(&occ);
        get_pawn_moves<colour>(board, sq, quiet_moves, captures);
    }
    occ = board.pieces(colour, KNIGHT);
    while (occ) {
        Square sq = pop_lsb(&occ);
        get_knight_moves<colour>(board, sq, quiet_moves, captures);
    }
    occ = board.pieces(colour, BISHOP);
    while (occ) {
        Square sq = pop_lsb(&occ);
        get_bishop_moves<colour>(board, sq, quiet_moves, captures);
    }
    occ = board.pieces(colour, ROOK);
    while (occ) {
        Square sq = pop_lsb(&occ);
        get_rook_moves<colour>(board, sq, quiet_moves, captures);
    }
    occ = board.pieces(colour, QUEEN);
    while (occ) {
        Square sq = pop_lsb(&occ);
        get_queen_moves<colour>(board, sq, quiet_moves, captures);
    }
    occ = board.pieces(colour, KING);
    while (occ) {
        Square sq = pop_lsb(&occ);
        get_king_moves<colour>(board, sq, quiet_moves, captures);
    }
}


void Board::get_pseudolegal_moves(MoveList &quiet_moves, MoveList &captures) const {  
    if (whos_move == Colour::WHITE) {
        generate_pseudolegal_moves<Colour::WHITE>(*this, quiet_moves, captures);
    } else {
        generate_pseudolegal_moves<Colour::BLACK>(*this, quiet_moves, captures);
    }
}

template<MoveGen gen_type, Colour colour>
void generate_moves(Board &board, MoveList &moves) {
    // By default, generate all legal moves
    moves.reserve(MAX_MOVES);
    Square king_square = board.find_king(colour);
    MoveList quiet_moves, captures, pseudolegal_moves;
    quiet_moves.reserve(MAX_MOVES);
    captures.reserve(MAX_MOVES);
    generate_pseudolegal_moves<colour>(board, quiet_moves, captures);
    if (gen_type == CAPTURES) {
        pseudolegal_moves = captures;
    } else if (gen_type == LEGAL) {
        pseudolegal_moves = captures;
        pseudolegal_moves.insert(pseudolegal_moves.end(), quiet_moves.begin(), quiet_moves.end());
    }
    if (board.is_check()) {
        const std::array<Square, 2> checkers = board.checkers();
        const int number_checkers = board.number_checkers();
        for (Move move : pseudolegal_moves) {
            if (move.origin == king_square) {
                // King moves have to be very careful.
                board.make_move(move);
                if (!board.is_attacked(move.target, colour)) {moves.push_back(move); }
                board.unmake_move(move);
            } else if (number_checkers == 2) {
                // double checks require a king move, which we've just seen this is not.
                continue;
            } else if (board.is_pinned(move.origin)) {
                continue;
            } else if (move.target == checkers[0]) {
                // this captures the checker, it's legal unless the peice in absolutely pinned.
                moves.push_back(move);
            } else if ( interposes(king_square, checkers[0], move.target)) {
                // this interposes the check it's legal unless the peice in absolutely pinned.
                moves.push_back(move);
            } else if (move.is_ep_capture() & ((move.origin.rank()|move.target.file()) == checkers[0])){
                // If it's an enpassent capture, the captures piece isn't at the target.
                moves.push_back(move);
            } else {
                // All other moves are illegal.
                continue;
            }
        }
        return;
    } else {
        // Otherwise we can be smarter.
        for (Move move : pseudolegal_moves) {
            if (move.origin == king_square) {
                // This is a king move, check where he's gone.
                board.make_move(move);
                if (!board.is_attacked(move.target, colour)) {moves.push_back(move); }
                board.unmake_move(move);  
            } else if (move.is_ep_capture()) {
                // en_passent's are weird.
                if (board.is_pinned(move.origin)) {
                    // If the pawn was pinned to the diagonal or file, the move is definitely illegal.
                    continue;
                } else {
                    // This can open a rank. if the king is on that rank it could be a problem.
                    if (king_square.rank() == move.origin.rank()) {
                        board.make_move(move);
                        if (!board.is_attacked(king_square, colour)) {moves.push_back(move); }
                        board.unmake_move(move);
                    } else {
                        moves.push_back(move); 
                    }
                }
            } else if (board.is_pinned(move.origin)) {
                // This piece is absoluetly pinned, only legal moves will maintain the pin or capture the pinner.
                if (in_line(king_square, move.origin, move.target)) {moves.push_back(move); }
            } else {
                // Piece isn't pinned, it can do what it wants. 
                moves.push_back(move);
            }
        }
        get_castle_moves<colour>(board, moves);
        return;
    }

}

MoveList Board::get_moves(){
    MoveList moves;
    Colour colour = who_to_play();
    if (colour == WHITE) {
        generate_moves<LEGAL, WHITE>(*this, moves);
    } else {
        generate_moves<LEGAL, BLACK>(*this, moves);
    }
    return moves;
}

MoveList Board::get_sorted_moves() {
    const MoveList legal_moves = get_moves();
    MoveList checks, non_checks, promotions, sorted_moves;
    checks.reserve(MAX_MOVES);
    non_checks.reserve(MAX_MOVES);
    // The moves from get_moves already have captures first
    for (Move move : legal_moves) {
        make_move(move);
        if (aux_info.is_check) {
            checks.push_back(move);
        } else if (move.is_queen_promotion()){
            promotions.push_back(move);
        } else {
            non_checks.push_back(move);
        }
        unmake_move(move);
    }
    sorted_moves = checks;
    sorted_moves.insert(sorted_moves.end(), promotions.begin(), promotions.end());
    sorted_moves.insert(sorted_moves.end(), non_checks.begin(), non_checks.end());
    return sorted_moves;
}


MoveList Board::get_captures() {
    MoveList captures;
    Colour colour = who_to_play();
    if (colour == WHITE) {
        generate_moves<CAPTURES, WHITE>(*this, captures);
    } else {
        generate_moves<CAPTURES, BLACK>(*this, captures);
    }
    return captures;
}