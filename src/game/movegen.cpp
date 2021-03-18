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
    Colour them = ~us;
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
        Bitboard atk = Bitboards::pawn_attacks(us, origin);
        atk &= board.pieces(them);
        while (atk) {
            Square sq = pop_lsb(&atk);
            Move move = Move(PAWN, origin, sq);
            move.make_capture();
            if (origin.rank() == relative_rank(us, Squares::Rank7)) {
                add_pawn_promotions(move, moves);
            } else {
                moves.push_back(move);
            }
        }

        // En-passent
        if (origin.rank() == relative_rank(us, Squares::Rank5)) {
            atk = Bitboards::pawn_attacks(us, origin) & board.en_passent();
            if (atk) {
                target = lsb(atk);
                Square ks = board.find_king(us);
                    // This can open a rank. if the king is on that rank it could be a problem.
                if (ks.rank() == origin.rank()) {
                    Bitboard mask = Bitboards::line(ks, origin);
                    Bitboard occ = board.pieces();
                    // Rook attacks from the king
                    Bitboard r_atk = rook_attacks(occ, ks);
                    // Rook xrays from the king
                    r_atk = rook_attacks(occ ^ (occ & r_atk), ks);
                    // Rook double xrays from the king
                    r_atk = rook_attacks(occ ^ (occ & r_atk), ks);
                    r_atk &= mask;
                    if (!(r_atk & board.pieces(them, ROOK, QUEEN))) {
                        move = Move(PAWN, origin, board.en_passent()); 
                        move.make_en_passent();
                        moves.push_back(move);
                    } 
                } else {
                        move = Move(PAWN, origin, board.en_passent()); 
                        move.make_en_passent();
                        moves.push_back(move);
                }
            }
        }
    }
}

template<GenType gen>
void gen_pawn_moves(const Board &board, const Square origin, MoveList &moves, Bitboard target_mask){
    Colour us = board.who_to_play();
    Colour them = ~us;
    Square target;
    Move move;
    // Moves are North
    // Look for double pawn push possibility
    if (gen == QUIET) {
        if (origin.rank() == relative_rank(us, Squares::Rank2)) {
            target = origin + (forwards(us) + forwards(us) );
            if (target_mask & target) {
                if (board.is_free(target) & board.is_free(origin + forwards(us))) {
                    move = Move(PAWN, origin, target);
                    move.make_double_push();
                    moves.push_back(move);
                }
            }
        }
        // Normal pushes.
        target = origin + forwards(us);
        if (target_mask & target) {
            if (board.is_free(target)) {
                move = Move(PAWN, origin, target);
                if (origin.rank() == relative_rank(us, Squares::Rank7)) {
                    add_pawn_promotions(move, moves);
                } else {
                    moves.push_back(move);
                }
            }
        }
    } else if (gen == CAPTURES) {
        // Normal captures.
        Bitboard atk = Bitboards::pawn_attacks(us, origin);
        atk &= board.pieces(them);
        atk &= target_mask;
        while (atk) {
            Square sq = pop_lsb(&atk);
            Move move = Move(PAWN, origin, sq);
            move.make_capture();
            if (origin.rank() == relative_rank(us, Squares::Rank7)) {
                add_pawn_promotions(move, moves);
            } else {
                moves.push_back(move);
            }
        }
        // En-passent
        // This is a really weird case where the target square is not where you end up. target_mask is always pieces we need to capture if this is called with captures
        if (origin.rank() == relative_rank(us, Squares::Rank5)) {
            atk = Bitboards::pawn_attacks(us, origin) & board.en_passent();
            target = board.en_passent();
            const Square ep_square = origin.rank()|target.file();
            if ((bool)atk && (bool)(target_mask & ep_square)) {
                Square ks = board.find_king(us);
                // This can open a rank. if the king is on that rank it could be a problem.
                if (ks.rank() == origin.rank()) {
                    Bitboard mask = Bitboards::line(ks, origin);
                    Bitboard occ = board.pieces();
                    // Rook attacks from the king
                    Bitboard r_atk = rook_attacks(occ, ks);
                    // Rook xrays from the king
                    r_atk = rook_attacks(occ ^ (occ & r_atk), ks);
                    // Rook double xrays from the king
                    r_atk = rook_attacks(occ ^ (occ & r_atk), ks);
                    r_atk &= mask;
                    if (!(r_atk & board.pieces(them, ROOK, QUEEN))) {
                        move = Move(PAWN, origin, board.en_passent()); 
                        move.make_en_passent();
                        moves.push_back(move);
                    } 
                } else {
                        move = Move(PAWN, origin, board.en_passent()); 
                        move.make_en_passent();
                        moves.push_back(move);
                }
            }
        }
    }
}

template<GenType gen>
void gen_king_moves(const Board &board, const Square origin, MoveList &moves) {
    Colour us = board.who_to_play();
    const Colour them = ~us;
    Bitboard atk = Bitboards::attacks<KING>(board.pieces(), origin);
    if (gen == QUIET) {
        atk &= ~board.pieces();
        while (atk) {
            Square sq = pop_lsb(&atk);
            // Check legality of the move here.
            if (board.is_attacked(sq, us)) {continue;}
            Move move = Move(KING, origin, sq);
            moves.push_back(move);
        }
    } else if (gen == CAPTURES) {
        atk &= board.pieces(them);
        while (atk) {
            Square sq = pop_lsb(&atk);
            // Check legality of the move here.
            if (board.is_attacked(sq, us)) { continue;}
            Move move = Move(KING, origin, sq);
            move.make_capture();
            moves.push_back(move);
        }
    }
}
template<GenType gen, PieceType pt>
void gen_moves(const Board &board, const Square origin, MoveList &moves) {
    if (pt == KING) {
        return gen_king_moves<gen>(board, origin, moves);
    } else {
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
}

template<GenType gen, PieceType pt>
void gen_moves(const Board &board, const Square origin, MoveList &moves, Bitboard target_mask) {
    Colour us = board.who_to_play();
    const Colour them = ~us;
    Bitboard atk = Bitboards::attacks<pt>(board.pieces(), origin);
    // Mask the target squares.
    atk &= target_mask;
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
    Square ks = board.find_king(us);
    // Pawns which aren't pinned
    occ = board.pieces(us, PAWN) & ~board.pinned();
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_pawn_moves<gen>(board, sq, moves);
    }
    // Pawns which are
    occ = board.pieces(us, PAWN) & board.pinned();
    while (occ) {
        Square sq = pop_lsb(&occ);
        Bitboard target_squares = Bitboards::line(sq, ks);
        gen_pawn_moves<gen>(board, sq, moves, target_squares);
    }
    // Pinned knights cannot move
    occ = board.pieces(us, KNIGHT) & ~board.pinned();
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_moves<gen, KNIGHT>(board, sq, moves);
    }
    // Bishops which aren't pinned
    occ = board.pieces(us, BISHOP) & ~board.pinned();
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_moves<gen, BISHOP>(board, sq, moves);
    }
    // Bisops which are pinned
    occ = board.pieces(us, BISHOP) & board.pinned();
    while (occ) {
        Square sq = pop_lsb(&occ);
        Bitboard target_squares = Bitboards::line(sq, ks);
        gen_moves<gen, BISHOP>(board, sq, moves, target_squares);
    }
    // Rooks which aren't pinned
    occ = board.pieces(us, ROOK) & ~board.pinned();
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_moves<gen, ROOK>(board, sq, moves);
    }
    // Rooks which are pinned
    occ = board.pieces(us, ROOK) & board.pinned();
    while (occ) {
        Square sq = pop_lsb(&occ);
        Bitboard target_squares = Bitboards::line(sq, ks);
        gen_moves<gen, ROOK>(board, sq, moves, target_squares);
    }
    occ = board.pieces(us, QUEEN) & ~board.pinned();
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_moves<gen, QUEEN>(board, sq, moves);
    }
    occ = board.pieces(us, QUEEN) & board.pinned();
    while (occ) {
        Square sq = pop_lsb(&occ);
        Bitboard target_squares = Bitboards::line(sq, ks);
        gen_moves<gen, QUEEN>(board, sq, moves, target_squares);
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
    const Bitboard can_move = ~board.pinned();
    occ = board.pieces(us, KING);
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_moves<QUIET, KING>(board, sq, moves);
        gen_moves<CAPTURES, KING>(board, sq, moves);
    }
    // We can block the check
    Bitboard between_squares = Bitboards::between(board.find_king(us), board.checkers(0));
    // We can capture the checker
    Bitboard target_square = sq_to_bb(board.checkers(0));
    occ = board.pieces(us, PAWN) & can_move;
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_pawn_moves<QUIET>(board, sq, moves, between_squares);
        gen_pawn_moves<CAPTURES>(board, sq, moves, target_square);
    }
    occ = board.pieces(us, KNIGHT) & can_move;
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_moves<QUIET, KNIGHT>(board, sq, moves, between_squares);
        gen_moves<CAPTURES, KNIGHT>(board, sq, moves, target_square);
    }
    occ = board.pieces(us, BISHOP) & can_move;
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_moves<QUIET, BISHOP>(board, sq, moves, between_squares);
        gen_moves<CAPTURES, BISHOP>(board, sq, moves, target_square);
    }
    occ = board.pieces(us, ROOK) & can_move;
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_moves<QUIET, ROOK>(board, sq, moves, between_squares);
        gen_moves<CAPTURES, ROOK>(board, sq, moves, target_square);
    }
    occ = board.pieces(us, QUEEN) & can_move;
    while (occ) {
        Square sq = pop_lsb(&occ);
        gen_moves<QUIET, QUEEN>(board, sq, moves, between_squares);
        gen_moves<CAPTURES, QUEEN>(board, sq, moves, target_square);
    }
}

template<GenType gen>
void generate_moves(Board &board, MoveList &moves) {
    // Generate all legal non-evasion moves
    Colour us = board.who_to_play();
    moves.reserve(MAX_MOVES);
    if (gen == CAPTURES) {
        generate_pseudolegal_moves<CAPTURES>(board, moves);
    } else if (gen == LEGAL) {
        generate_pseudolegal_moves<CAPTURES>(board, moves);
        generate_pseudolegal_moves<QUIET>(board, moves);
    }    
    if (us == WHITE) {
        gen_castle_moves<WHITE>(board, moves);
    } else {
        gen_castle_moves<BLACK>(board, moves);
    }
    return;

}

template<>
void generate_moves<EVASIONS>(Board &board, MoveList &moves) {
    // Generate all legal moves
    Colour us = board.who_to_play();
    Square king_square = board.find_king(us);
    MoveList pseudolegal_moves;
    moves.reserve(MAX_MOVES);
    const int number_checkers = board.number_checkers();
    if (number_checkers == 2) {
        // Double check. Generate king moves.
        gen_moves<QUIET, KING>(board, king_square, moves);
        gen_moves<CAPTURES, KING>(board, king_square, moves);
    } else {
        generate_pseudolegal_moves<EVASIONS>(board, moves);
    }
    return;
}

MoveList Board::get_moves(){
    MoveList moves;
    if (is_check()) {
        generate_moves<EVASIONS>(*this, moves);
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
    MoveList moves;
    if (is_check()) {
        generate_moves<EVASIONS>(*this, moves);
    } else {
        generate_moves<CAPTURES>(*this, moves);
    }
    return moves;
}