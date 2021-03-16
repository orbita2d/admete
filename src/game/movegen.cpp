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

template<Colour us, GenType gen>
void gen_pawn_moves(const Board &board, const Square origin, MoveList &moves){
    Square target;
    Move move;
    // Moves are North
    // Look for double pawn push possibility
    if (gen == QUIET) {
        if (origin.rank() == relative_rank(us, Squares::Rank2)) {
            target = origin + (forwards(us) + forwards(us) );
            if (board.is_free(target) & board.is_free(origin + forwards(us) )) {
                move = Move(origin, target);
                move.make_double_push();
                moves.push_back(move);
            }
        }
        // Normal pushes.
        target = origin + (forwards(us) );
        if (board.is_free(target)) {
            move = Move(origin, target);
            if (origin.rank() == relative_rank(us, Squares::Rank7)) {
                add_pawn_promotions(move, moves);
            } else {
                moves.push_back(move);
            }
        }
    } else if (gen == CAPTURES) {
        // Normal captures.
        if (origin.to_west() != 0) {
            target = origin + (forwards(us) + Direction::W);
            if (board.is_colour(~us, target)) {
                move = Move(origin, target);
                move.make_capture();
                if (origin.rank() == relative_rank(us, Squares::Rank7)) {
                    add_pawn_promotions(move, moves);
                } else {
                    moves.push_back(move);
                }
            }
        }

        if (origin.to_east() != 0) {
            target = origin + (forwards(us) + Direction::E);
            if (board.is_colour(~us, target)) {
                move = Move(origin, target);
                move.make_capture();
                if (origin.rank() == relative_rank(us, Squares::Rank7)) {
                    add_pawn_promotions(move, moves);
                } else {
                    moves.push_back(move);
                }
            }
        }
        // En-passent
        if (origin.rank() == relative_rank(us, Squares::Rank5)) {
            if (((origin.to_west() != 0) & (board.aux_info.en_passent_target == Square(origin + forwards(us) + Direction::W))) | 
                ((origin.to_east() != 0) & (board.aux_info.en_passent_target == Square(origin + forwards(us) + Direction::E))) ) {
                move = Move(origin, board.aux_info.en_passent_target); 
                move.make_en_passent();
                moves.push_back(move);
            }
        }
    }
}


template<Colour us, GenType gen>
void gen_rook_moves(const Board &board, const Square origin, MoveList &moves) {
    const Colour them = ~us;
    Bitboard atk = rook_attacks(board.pieces(), origin);
    if (gen == QUIET) {
        atk &= ~board.pieces();
        while (atk) {
            Square sq = pop_lsb(&atk);
            Move move = Move(origin, sq);
            moves.push_back(move);
        }
    } else if (gen == CAPTURES) {
        atk &= board.pieces(them);
        while (atk) {
            Square sq = pop_lsb(&atk);
            Move move = Move(origin, sq);
            move.make_capture();
            moves.push_back(move);
        }
    }
}


template<Colour us, GenType gen>
void gen_bishop_moves(const Board &board, const Square origin, MoveList &moves) {
    const Colour them = ~us;
    Bitboard atk = bishop_attacks(board.pieces(), origin);
    if (gen == QUIET) {
        atk &= ~board.pieces();
        while (atk) {
            Square sq = pop_lsb(&atk);
            Move move = Move(origin, sq);
            moves.push_back(move);
        }
    } else if (gen == CAPTURES) {
        atk &= board.pieces(them);
        while (atk) {
            Square sq = pop_lsb(&atk);
            Move move = Move(origin, sq);
            move.make_capture();
            moves.push_back(move);
        }
    }
}
template<Colour us, GenType gen>
void gen_queen_moves(const Board &board, const Square origin, MoveList &moves) {
    // Queen moves are the union superset of rook and bishop moves
    const Colour them = ~us;
    Bitboard atk = bishop_attacks(board.pieces(), origin) | rook_attacks(board.pieces(), origin);
    if (gen == QUIET) {
        atk &= ~board.pieces();
        while (atk) {
            Square sq = pop_lsb(&atk);
            Move move = Move(origin, sq);
            moves.push_back(move);
        }
    } else if (gen == CAPTURES) {
        atk &= board.pieces(them);
        while (atk) {
            Square sq = pop_lsb(&atk);
            Move move = Move(origin, sq);
            move.make_capture();
            moves.push_back(move);
        }
    }
}


template<Colour colour>
void gen_castle_moves(const Board &board, MoveList &moves) {
    // Check if castling is legal, and if so add it to the move list. Assumes we aren't in check
    Move move;
    // You can't castle through check, or while in check
    if (board.aux_info.castling_rights[colour][QUEENSIDE]) 
    {
        // Check for overlap of squares that need to be free, and occupied bb.
        // In the future we should keep a 
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

template<Colour us, GenType gen>
void gen_king_moves(const Board &board, const Square origin, MoveList &moves) {
    const Colour them = ~us;
    // We should really be careful that we aren't moving into check here.
    // Look to see if we are on an edge.
    Bitboard atk =  Bitboards::attacks(KING, origin);
    if (gen == QUIET) {
        atk &= ~board.pieces();
        while (atk) {
            Square sq = pop_lsb(&atk);
            Move move = Move(origin, sq);
            moves.push_back(move);
        }
    } else if (gen == CAPTURES) {
        atk &= board.pieces(them);
        while (atk) {
            Square sq = pop_lsb(&atk);
            Move move = Move(origin, sq);
            move.make_capture();
            moves.push_back(move);
        }
    }
}

template<Colour us, GenType gen>
void gen_knight_moves(const Board &board, const Square origin, MoveList &moves) {
    const Colour them = ~us;
    Bitboard atk = Bitboards::attacks(KNIGHT, origin);
    if (gen == QUIET) {
        atk &= ~board.pieces();
        while (atk) {
            Square sq = pop_lsb(&atk);
            Move move = Move(origin, sq);
            moves.push_back(move);
        }
    } else if (gen == CAPTURES) {
        atk &= board.pieces(them);
        while (atk) {
            Square sq = pop_lsb(&atk);
            Move move = Move(origin, sq);
            move.make_capture();
            moves.push_back(move);
        }
    }
};

template<Colour colour, GenType gen>
void generate_pseudolegal_moves(const Board &board, MoveList &moves) {
    Bitboard occ;
    occ = board.pieces(colour, PAWN);
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_pawn_moves<colour, gen>(board, sq, moves);
    }
    occ = board.pieces(colour, KNIGHT);
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_knight_moves<colour, gen>(board, sq, moves);
    }
    occ = board.pieces(colour, BISHOP);
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_bishop_moves<colour, gen>(board, sq, moves);
    }
    occ = board.pieces(colour, ROOK);
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_rook_moves<colour, gen>(board, sq, moves);
    }
    occ = board.pieces(colour, QUEEN);
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_queen_moves<colour, gen>(board, sq, moves);
    }
    occ = board.pieces(colour, KING);
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_king_moves<colour, gen>(board, sq, moves);
    }
}


void Board::get_pseudolegal_moves(MoveList &quiet_moves, MoveList &captures) const {  
    if (whos_move == WHITE) {
        generate_pseudolegal_moves<WHITE, CAPTURES>(*this, captures);
        generate_pseudolegal_moves<WHITE, QUIET>(*this, quiet_moves);
    } else {
        generate_pseudolegal_moves<BLACK, CAPTURES>(*this, captures);
        generate_pseudolegal_moves<BLACK, QUIET>(*this, quiet_moves);
    }
}

template<Colour colour, GenType gen>
void generate_moves(Board &board, MoveList &moves) {
    // Generate all legal moves
    moves.reserve(MAX_MOVES);
    Square king_square = board.find_king(colour);
    MoveList pseudolegal_moves;
    pseudolegal_moves.reserve(MAX_MOVES);
    if (gen == CAPTURES) {
        generate_pseudolegal_moves<colour, CAPTURES>(board, pseudolegal_moves);
    } else if (gen == LEGAL) {
        generate_pseudolegal_moves<colour, CAPTURES>(board, pseudolegal_moves);
        generate_pseudolegal_moves<colour, QUIET>(board, pseudolegal_moves);
    }
    if (board.is_check()) {
        const std::array<Square, 2> checkers = board.checkers();
        const int number_checkers = board.number_checkers();
        for (Move move : pseudolegal_moves) {
            if (move.origin == king_square) {
                // King moves have to be very careful.
                if (!board.is_attacked(move.target, colour)) {moves.push_back(move); }
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
                if (!board.is_attacked(move.target, colour)) {moves.push_back(move); }
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
        gen_castle_moves<colour>(board, moves);
        return;
    }

}

MoveList Board::get_moves(){
    MoveList moves;
    Colour colour = who_to_play();
    if (colour == WHITE) {
        generate_moves<WHITE, LEGAL>(*this, moves);
    } else {
        generate_moves<BLACK, LEGAL>(*this, moves);
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
        generate_moves<WHITE, CAPTURES>(*this, captures);
    } else {
        generate_moves<BLACK, CAPTURES>(*this, captures);
    }
    return captures;
}