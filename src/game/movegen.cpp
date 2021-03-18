#include "movegen.hpp"
#include "board.hpp"
#include "evaluate.hpp"
#include <algorithm>

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

template<GenType gen>
void gen_pawn_moves(const Board &board, const Square origin, MoveList &moves){
    Colour us = board.who_to_play();
    Square target;
    Move move;
    // Moves are North
    // Look for double pawn push possibility
    if (gen == QUIET) {
        if (origin.rank() == relative_rank(us, Squares::Rank2)) {
            target = origin + (forwards(us) + forwards(us) );
            if (board.is_free(target) & board.is_free(origin + forwards(us) )) {
                move = Move(PAWN, origin, target);
                move.make_double_push();
                moves.push_back(move);
            }
        }
        // Normal pushes.
        target = origin + (forwards(us) );
        if (board.is_free(target)) {
            move = Move(PAWN, origin, target);
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
                move = Move(PAWN, origin, target);
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
                move = Move(PAWN, origin, target);
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
            if (((origin.to_west() != 0) & (board.en_passent() == Square(origin + forwards(us) + Direction::W))) | 
                ((origin.to_east() != 0) & (board.en_passent() == Square(origin + forwards(us) + Direction::E))) ) {
                move = Move(PAWN, origin, board.en_passent()); 
                move.make_en_passent();
                moves.push_back(move);
            }
        }
    }
}


template<GenType gen, PieceType pt>
void gen_moves(const Board &board, const Square origin, MoveList &moves) {
    Colour us = board.who_to_play();
    const Colour them = ~us;
    Bitboard atk = Bitboards::attacks<pt>(board.pieces(), origin);
    if (gen == QUIET) {
        atk &= ~board.pieces();
        while (atk) {
            Square sq = pop_lsb(&atk);
            Move move = Move(pt, origin, sq);
            moves.push_back(move);
        }
    } else if (gen == CAPTURES) {
        atk &= board.pieces(them);
        while (atk) {
            Square sq = pop_lsb(&atk);
            Move move = Move(pt, origin, sq);
            move.make_capture();
            moves.push_back(move);
        }
    }
}

template<Colour us>
void gen_castle_moves(const Board &board, MoveList &moves) {
    // Check if castling is legal, and if so add it to the move list. Assumes we aren't in check
    Move move;
    // You can't castle through check, or while in check
    if (board.can_castle(us, QUEENSIDE)) 
    {
        // Check for overlap of squares that need to be free, and occupied bb.
        // In the future we should keep a 
        if (!(Bitboards::castle(us, QUEENSIDE) & board.pieces())) {
            if (!board.is_attacked(Squares::FileD | back_rank(us), us)
                & !board.is_attacked(Squares::FileC | back_rank(us), us)) 
            {
                move = Move(KING, Squares::FileE | back_rank(us), Squares::FileC | back_rank(us));
                move.make_queen_castle();
                moves.push_back(move);
            }
        }
    }
    if (board.can_castle(us, KINGSIDE)) 
    {
        if (!(Bitboards::castle(us, KINGSIDE) & board.pieces())) {
            if (!board.is_attacked(Squares::FileF | back_rank(us), us)
                & !board.is_attacked(Squares::FileG | back_rank(us), us)) 
            {
                move = Move(KING, Squares::FileE | back_rank(us), Squares::FileG | back_rank(us));
                move.make_king_castle();
                moves.push_back(move);
            }
        }
    }
}

template<GenType gen>
void generate_pseudolegal_moves(const Board &board, MoveList &moves) {
    Colour us = board.who_to_play();
    Bitboard occ;
    occ = board.pieces(us, PAWN);
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_pawn_moves<gen>(board, sq, moves);
    }
    occ = board.pieces(us, KNIGHT);
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_moves<gen, KNIGHT>(board, sq, moves);
    }
    occ = board.pieces(us, BISHOP);
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_moves<gen, BISHOP>(board, sq, moves);
    }
    occ = board.pieces(us, ROOK);
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_moves<gen, ROOK>(board, sq, moves);
    }
    occ = board.pieces(us, QUEEN);
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_moves<gen, QUEEN>(board, sq, moves);
    }
    occ = board.pieces(us, KING);
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_moves<gen, KING>(board, sq, moves);
    }
}

template<>
void generate_pseudolegal_moves<EVASIONS>(const Board &board, MoveList &moves) {
    Colour us = board.who_to_play();
    Bitboard occ;
    occ = board.pieces(us, PAWN);
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_pawn_moves<QUIET>(board, sq, moves);
        gen_pawn_moves<CAPTURES>(board, sq, moves);
    }
    occ = board.pieces(us, KNIGHT);
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_moves<QUIET, KNIGHT>(board, sq, moves);
        gen_moves<CAPTURES, KNIGHT>(board, sq, moves);
    }
    occ = board.pieces(us, BISHOP);
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_moves<QUIET, BISHOP>(board, sq, moves);
        gen_moves<CAPTURES, BISHOP>(board, sq, moves);
    }
    occ = board.pieces(us, ROOK);
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_moves<QUIET, ROOK>(board, sq, moves);
        gen_moves<CAPTURES, ROOK>(board, sq, moves);
    }
    occ = board.pieces(us, QUEEN);
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_moves<QUIET, QUEEN>(board, sq, moves);
        gen_moves<CAPTURES, QUEEN>(board, sq, moves);
    }
    occ = board.pieces(us, KING);
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_moves<QUIET, KING>(board, sq, moves);
        gen_moves<CAPTURES, KING>(board, sq, moves);
    }
}

template<GenType gen>
void generate_moves(Board &board, MoveList &moves) {
    // Generate all legal moves
    Colour us = board.who_to_play();
    const Colour them = ~us;
    Square king_square = board.find_king(us);
    MoveList pseudolegal_moves;
    pseudolegal_moves.reserve(MAX_MOVES);
    const int number_checkers = board.number_checkers();
    if (number_checkers == 2) {
        // Double check. Generate king moves.
        gen_moves<QUIET, KING>(board, king_square, pseudolegal_moves);
        gen_moves<CAPTURES, KING>(board, king_square, pseudolegal_moves);
    } else if (number_checkers == 1) {
        //generate_pseudolegal_moves<CAPTURES>(board, pseudolegal_moves);
        //generate_pseudolegal_moves<QUIET>(board, pseudolegal_moves);
        generate_pseudolegal_moves<EVASIONS>(board, pseudolegal_moves);
    } else if (gen == CAPTURES) {
        generate_pseudolegal_moves<CAPTURES>(board, pseudolegal_moves);
    } else if (gen == LEGAL) {
        generate_pseudolegal_moves<CAPTURES>(board, pseudolegal_moves);
        generate_pseudolegal_moves<QUIET>(board, pseudolegal_moves);
    }
    moves.reserve(pseudolegal_moves.size());
    if (board.is_check()) {
        const std::array<Square, 2> checkers = board.checkers();
        for (Move move : pseudolegal_moves) {
            if (move.origin == king_square) {
                // King moves have to be very careful.
                if (!board.is_attacked(move.target, us)) {moves.push_back(move); }
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
                // If it's an enpassent capture, the captured piece isn't at the target.
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
                // This is a see if the square is attacked.
                if (!board.is_attacked(move.target, us)) {moves.push_back(move); }
            } else if (move.is_ep_capture()) {
                // en_passent's are weird.
                if (board.is_pinned(move.origin)) {
                    // If the pawn was pinned to the diagonal or file, the move is definitely illegal.
                    continue;
                } else {
                    // This can open a rank. if the king is on that rank it could be a problem.
                    if (king_square.rank() == move.origin.rank()) {
                        Bitboard mask = Bitboards::line(king_square, move.origin);
                        Bitboard occ = board.pieces();
                        // Rook attacks from the king
                        Bitboard atk = rook_attacks(occ, king_square);
                        // Rook xrays from the king
                        atk = rook_attacks(occ ^ (occ & atk), king_square);
                        // Rook double xrays from the king
                        atk = rook_attacks(occ ^ (occ & atk), king_square);
                        atk &= mask;
                        if (atk & board.pieces(them, ROOK, QUEEN)) {
                            continue;
                        } else {
                            moves.push_back(move); 
                        }
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
        if (us == WHITE) {
            gen_castle_moves<WHITE>(board, moves);
        } else {
            gen_castle_moves<BLACK>(board, moves);
        }
        return;
    }

}

MoveList Board::get_moves(){
    MoveList moves;
    Colour us = who_to_play();
    if (us == WHITE) {
        generate_moves<LEGAL>(*this, moves);
    } else {
        generate_moves<LEGAL>(*this, moves);
    }
    return moves;
}

struct SMove
{
    SMove(Move m) : move(m){};
    // Struct for move with score
    Move move;
    int score = 0;
};

bool cmp(SMove &m1, SMove &m2) {
    return m1.score < m2.score;
}

MoveList Board::get_sorted_moves() {
    const MoveList legal_moves = get_moves();
    MoveList checks, quiet_moves, sorted_moves;
    checks.reserve(MAX_MOVES);
    quiet_moves.reserve(MAX_MOVES);
    // The moves from get_moves already have captures first
    for (Move move : legal_moves) {
        if (gives_check(move)) {
            checks.push_back(move);
        } else {
            quiet_moves.push_back(move);
        }
    }
    sorted_moves = checks;
    sorted_moves.insert(sorted_moves.end(), quiet_moves.begin(), quiet_moves.end());
    return sorted_moves;
}


MoveList Board::get_captures() {
    MoveList captures;
    Colour us = who_to_play();
    if (us == WHITE) {
        generate_moves<CAPTURES>(*this, captures);
    } else {
        generate_moves<CAPTURES>(*this, captures);
    }
    return captures;
}