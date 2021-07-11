#include "movegen.hpp"
#include "board.hpp"
#include "evaluate.hpp"
#include <algorithm>

// Move Generation

// Pawn moves are the most complicated in a legal movegen.

// Generate a single pawn push from origin and add it to the movelist.
template <Colour us> void gen_pawn_push(const Square origin, MoveList &moves) {
    const Square target = origin + forwards(us);
    Move move(PAWN, origin, target);
    moves.push_back(move);
}

// Generate a single pawn push that promotes.
template <Colour us> void gen_pawn_push_prom(const Square origin, MoveList &moves) {
    const Square target = origin + forwards(us);
    Move move(PAWN, origin, target);

    move.make_queen_promotion();
    moves.push_back(move);
    move.make_knight_promotion();
    moves.push_back(move);
    move.make_rook_promotion();
    moves.push_back(move);
    move.make_bishop_promotion();
    moves.push_back(move);
}

template <Colour us> void gen_pawn_double_push(const Square origin, MoveList &moves) {
    Square target;
    Move move;
    target = origin + (forwards(us) + forwards(us));
    move = Move(PAWN, origin, target);
    move.make_double_push();
    moves.push_back(move);
}

// Generate en-passent captures with no restraint on where to go.
template <Colour us> void gen_pawn_ep(const Board &board, const Square origin, MoveList &moves) {
    const Square target = Square(relative_rank(us, RANK6), board.en_passent());
    const Square ks = board.find_king(us);
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
        if (!(r_atk & board.pieces(~us, ROOK, QUEEN))) {
            Move move(PAWN, origin, target);
            move.make_en_passent();
            moves.push_back(move);
        }
    } else {
        Move move(PAWN, origin, target);
        move.make_en_passent();
        moves.push_back(move);
    }
}

// Generate en-passent captures, where the pawn is pinned to a diagonal.
template <Colour us> void gen_pawn_ep(const Board &board, const Square origin, MoveList &moves, Bitboard target_mask) {
    Square target = Square(relative_rank(us, RANK6), board.en_passent());
    if (!(target_mask & target)) {
        return;
    }
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
        if (!(r_atk & board.pieces(~us, ROOK, QUEEN))) {
            Move move(PAWN, origin, target);
            move.make_en_passent();
            moves.push_back(move);
        }
    } else {
        Move move(PAWN, origin, target);
        move.make_en_passent();
        moves.push_back(move);
    }
}

// Construct a simple pawn capture (no ep, no promotion) in given direction.
template <Colour us, Direction dir> void gen_pawn_cap(const Square origin, MoveList &moves) {
    assert(dir == W || dir == E);
    const Square target = (origin + forwards(us)) + dir;
    Move move(PAWN, origin, target);
    move.make_capture();
    moves.push_back(move);
}

// Construct a simple pawn capture (no ep, no promotion) in given direction, but with the capture square constrained.
template <Colour us, Direction dir>
void gen_pawn_cap(const Square origin, MoveList &moves, const Bitboard target_mask) {
    assert(dir == W || dir == E);
    const Square target = (origin + forwards(us)) + dir;
    if (!(target_mask & target)) {
        return;
    }
    Move move(PAWN, origin, target);
    move.make_capture();
    moves.push_back(move);
}

// Construct a promoting pawn capture in given direction.
template <Colour us, Direction dir> void gen_pawn_prom_cap(const Square origin, MoveList &moves) {
    assert(origin.rank() == relative_rank(us, RANK7));
    assert(dir == W || dir == E);
    const Square target = (origin + forwards(us)) + dir;
    Move move(PAWN, origin, target);
    move.make_capture();
    move.make_queen_promotion();
    moves.push_back(move);
    move.make_knight_promotion();
    moves.push_back(move);
    move.make_rook_promotion();
    moves.push_back(move);
    move.make_bishop_promotion();
    moves.push_back(move);
}

// Construct a promoting pawn capture in given direction, but with the capture square constrained.
template <Colour us, Direction dir>
void gen_pawn_prom_cap(const Square origin, MoveList &moves, const Bitboard target_mask) {
    assert(origin.rank() == relative_rank(us, RANK7));
    assert(dir == W || dir == E);
    const Square target = (origin + forwards(us)) + dir;
    if (!(target_mask & target)) {
        return;
    }
    Move move(PAWN, origin, target);
    move.make_capture();
    move.make_queen_promotion();
    moves.push_back(move);
    move.make_knight_promotion();
    moves.push_back(move);
    move.make_rook_promotion();
    moves.push_back(move);
    move.make_bishop_promotion();
    moves.push_back(move);
}

template <Colour us, GenType gen> void gen_pawn_moves(const Board &board, MoveList &moves) {
    // Pawns which aren't pinned
    assert(gen == QUIET || gen == CAPTURES || gen == EVASIONS);
    Square ks = board.find_king(us);

    if constexpr (gen == QUIET) {
        const Bitboard pawns = board.pieces(us, PAWN);

        // Single pawn pushes.
        Bitboard q_pin;
        Bitboard q_nopin;
        // Ignore squares directly behind an occupied square.
        Bitboard occ = pawns;
        occ &= ~Bitboards::shift<backwards(us)>(board.pieces());

        // Unpinned pawns on ranks 2--> 6
        q_nopin = occ & ~board.pinned() & ~Bitboards::rank(relative_rank(us, RANK7));
        while (q_nopin) {
            const Square sq = pop_lsb(&q_nopin);
            gen_pawn_push<us>(sq, moves);
        }
        // Pinned pawns on ranks 2--> 6
        q_pin = occ & board.pinned() & ~Bitboards::rank(relative_rank(us, RANK7));
        while (q_pin) {
            const Square sq = pop_lsb(&q_pin);
            // Can push only if the pin is down a file.
            if (sq.file() == ks.file()) {
                gen_pawn_push<us>(sq, moves);
            }
        }

        // Unpinned pieces on rank 7, pinned pawns on rank 7 cannot push.
        q_nopin = occ & ~board.pinned() & Bitboards::rank(relative_rank(us, RANK7));
        while (q_nopin) {
            const Square sq = pop_lsb(&q_nopin);
            gen_pawn_push_prom<us>(sq, moves);
        }

        // Double pawn pushes.
        occ = pawns;
        // Consider only squares on the 2nd rank.
        occ &= Bitboards::rank(relative_rank(us, RANK2));
        // Ignores squares where either of the next two (forwards) squares are occupied.
        Bitboard blk = Bitboards::shift<backwards(us)>(board.pieces());
        blk |= Bitboards::shift<backwards(us)>(blk);
        occ &= ~blk;

        q_pin = occ & board.pinned();
        q_nopin = occ & ~board.pinned();
        while (q_nopin) {
            const Square sq = pop_lsb(&q_nopin);
            gen_pawn_double_push<us>(sq, moves);
        }
        while (q_pin) {
            const Square sq = pop_lsb(&q_pin);
            if (sq.file() == ks.file()) {
                gen_pawn_double_push<us>(sq, moves);
            }
        }

    } else if constexpr (gen == CAPTURES) {
        const Bitboard pawns = board.pieces(us, PAWN);

        // Simple captures
        Bitboard occ = pawns;

        // Relative southwest
        constexpr Direction rSW = (Direction)(backwards(us) + W);
        occ &= Bitboards::shift<rSW>(board.pieces(~us));

        // Non-promoting captures
        Bitboard q_nopin = occ & ~board.pinned() & ~Bitboards::rank(relative_rank(us, RANK7));
        while (q_nopin) {
            const Square sq = pop_lsb(&q_nopin);
            gen_pawn_cap<us, E>(sq, moves);
        }
        Bitboard q_pin = occ & board.pinned() & ~Bitboards::rank(relative_rank(us, RANK7));
        while (q_pin) {
            const Square sq = pop_lsb(&q_pin);
            const Bitboard target_squares = Bitboards::line(sq, ks);
            gen_pawn_cap<us, E>(sq, moves, target_squares);
        }

        // Promoting captures
        q_nopin = occ & ~board.pinned() & Bitboards::rank(relative_rank(us, RANK7));
        while (q_nopin) {
            const Square sq = pop_lsb(&q_nopin);
            gen_pawn_prom_cap<us, E>(sq, moves);
        }
        q_pin = occ & board.pinned() & Bitboards::rank(relative_rank(us, RANK7));
        while (q_pin) {
            const Square sq = pop_lsb(&q_pin);
            const Bitboard target_squares = Bitboards::line(sq, ks);
            gen_pawn_prom_cap<us, E>(sq, moves, target_squares);
        }

        occ = pawns;
        // Relative southeast
        constexpr Direction rNE = (Direction)(forwards(us) + E);
        constexpr Direction rSE = (Direction)(backwards(us) + E);
        occ &= Bitboards::shift<rSE>(board.pieces(~us));

        // Non-promoting captures
        q_nopin = occ & ~board.pinned() & ~Bitboards::rank(relative_rank(us, RANK7));
        while (q_nopin) {
            const Square sq = pop_lsb(&q_nopin);
            gen_pawn_cap<us, W>(sq, moves);
        }
        q_pin = occ & board.pinned() & ~Bitboards::rank(relative_rank(us, RANK7));
        while (q_pin) {
            const Square sq = pop_lsb(&q_pin);
            const Bitboard target_squares = Bitboards::line(sq, ks);
            gen_pawn_cap<us, W>(sq, moves, target_squares);
        }

        // Promoting captures
        q_nopin = occ & ~board.pinned() & Bitboards::rank(relative_rank(us, RANK7));
        while (q_nopin) {
            const Square sq = pop_lsb(&q_nopin);
            gen_pawn_prom_cap<us, W>(sq, moves);
        }
        q_pin = occ & board.pinned() & Bitboards::rank(relative_rank(us, RANK7));
        while (q_pin) {
            const Square sq = pop_lsb(&q_pin);
            const Bitboard target_squares = Bitboards::line(sq, ks);
            gen_pawn_prom_cap<us, W>(sq, moves, target_squares);
        }

        // ep-captures
        if (board.en_passent() != NO_FILE) {
            Bitboard occ = pawns;
            const Bitboard ep_file = Bitboards::file(board.en_passent());
            occ &= Bitboards::shift<W>(ep_file) | Bitboards::shift<E>(ep_file);
            occ &= Bitboards::rank(relative_rank(us, RANK5));
            q_pin = occ & board.pinned();
            q_nopin = occ & ~board.pinned();
            while (q_nopin) {
                const Square sq = pop_lsb(&q_nopin);
                gen_pawn_ep<us>(board, sq, moves);
            }
            while (q_pin) {
                const Square sq = pop_lsb(&q_pin);
                const Bitboard target_squares = Bitboards::line(sq, ks);
                gen_pawn_ep<us>(board, sq, moves, target_squares);
            }
        }
    } else if constexpr (gen == EVASIONS) {
        const Bitboard pawns = board.pieces(us, PAWN) & ~board.pinned();

        // We can capture the checker
        const Square ts = board.checkers(0);
        const Bitboard target_square = sq_to_bb(ts);
        const bool on_eighth = ts.rank() == relative_rank(us, RANK8);

        Bitboard occ = pawns;

        // Relative southwest
        constexpr Direction rSW = (Direction)(backwards(us) + W);
        occ &= Bitboards::shift<rSW>(target_square);

        while (occ) {
            const Square sq = pop_lsb(&occ);
            if (on_eighth) {
                gen_pawn_prom_cap<us, E>(sq, moves);
            } else {
                gen_pawn_cap<us, E>(sq, moves);
            }
        }

        occ = pawns;
        // Relative southwest
        constexpr Direction rSE = (Direction)(backwards(us) + E);
        occ &= Bitboards::shift<rSE>(target_square);

        while (occ) {
            const Square sq = pop_lsb(&occ);
            if (on_eighth) {
                gen_pawn_prom_cap<us, W>(sq, moves);
            } else {
                gen_pawn_cap<us, W>(sq, moves);
            }
        }

        if (board.en_passent() != NO_FILE) {
            const Square to_capture = Square(relative_rank(us, RANK5), board.en_passent());
            if (to_capture == ts) {
                const Bitboard ep_file = Bitboards::file(board.en_passent());
                occ = pawns;
                occ &= Bitboards::shift<W>(ep_file) | Bitboards::shift<E>(ep_file);
                occ &= Bitboards::rank(relative_rank(us, RANK5));
                while (occ) {
                    const Square sq = pop_lsb(&occ);
                    gen_pawn_ep<us>(board, sq, moves);
                }
            }
        }

        // We can block the check.
        const Bitboard between_squares = Bitboards::between(ks, ts);
        occ = pawns;
        // All of our pawns that can move that can block the check by pushing.
        occ &= Bitboards::shift<backwards(us)>(between_squares);

        while (occ) {
            const Square sq = pop_lsb(&occ);
            gen_pawn_push<us>(sq, moves);
        }

        // generate double pawn pushes.
        occ = pawns;
        // Relative south-south.
        constexpr Direction rSS = (Direction)(backwards(us) + backwards(us));
        occ &= Bitboards::shift<rSS>(between_squares);
        occ &= Bitboards::rank(relative_rank(us, RANK2));
        // There's no way a piece on the second square could be "between" the checker and king, so no need to verify.
        Bitboard blk = Bitboards::shift<backwards(us)>(board.pieces());
        occ &= ~blk;

        while (occ) {
            const Square sq = pop_lsb(&occ);
            gen_pawn_double_push<us>(sq, moves);
        }
    }
}

template <GenType gen> void gen_king_moves(const Board &board, const Square origin, MoveList &moves) {
    Colour us = board.who_to_play();
    const Colour them = ~us;
    Bitboard atk = Bitboards::attacks<KING>(board.pieces(), origin);
    if constexpr (gen == QUIET) {
        atk &= ~board.pieces();
        while (atk) {
            const Square sq = pop_lsb(&atk);
            // Check legality of the move here.
            if (board.is_attacked(sq, us)) {
                continue;
            }
            Move move = Move(KING, origin, sq);
            moves.push_back(move);
        }
    } else if constexpr (gen == CAPTURES) {
        atk &= board.pieces(them);
        while (atk) {
            const Square sq = pop_lsb(&atk);
            // Check legality of the move here.
            if (board.is_attacked(sq, us)) {
                continue;
            }
            Move move = Move(KING, origin, sq);
            move.make_capture();
            moves.push_back(move);
        }
    }
}
template <GenType gen, PieceType pt> void gen_moves(const Board &board, const Square origin, MoveList &moves) {
    if constexpr (pt == KING) {
        return gen_king_moves<gen>(board, origin, moves);
    } else {
        Colour us = board.who_to_play();
        const Colour them = ~us;
        Bitboard atk = Bitboards::attacks<pt>(board.pieces(), origin);
        if constexpr (gen == QUIET) {
            atk &= ~board.pieces();
            while (atk) {
                const Square sq = pop_lsb(&atk);
                Move move = Move(pt, origin, sq);
                moves.push_back(move);
            }
        } else if constexpr (gen == CAPTURES) {
            atk &= board.pieces(them);
            while (atk) {
                const Square sq = pop_lsb(&atk);
                Move move = Move(pt, origin, sq);
                move.make_capture();
                moves.push_back(move);
            }
        }
    }
}

template <GenType gen, PieceType pt>
void gen_moves(const Board &board, const Square origin, MoveList &moves, Bitboard target_mask) {
    Colour us = board.who_to_play();
    const Colour them = ~us;
    Bitboard atk = Bitboards::attacks<pt>(board.pieces(), origin);
    // Mask the target squares.
    atk &= target_mask;
    if constexpr (gen == QUIET) {
        atk &= ~board.pieces();
        while (atk) {
            const Square sq = pop_lsb(&atk);
            Move move = Move(pt, origin, sq);
            moves.push_back(move);
        }
    } else if constexpr (gen == CAPTURES) {
        atk &= board.pieces(them);
        while (atk) {
            const Square sq = pop_lsb(&atk);
            Move move = Move(pt, origin, sq);
            move.make_capture();
            moves.push_back(move);
        }
    }
}

template <Colour us, CastlingSide side> void gen_castle_moves(const Board &board, MoveList &moves) {
    // Check if castling is legal, and if so add it to the move list. Assumes we aren't in check
    Move move;
    // You can't castle through check, or while in check
    if (board.can_castle(us, QUEENSIDE) && (side == QUEENSIDE)) {
        // Check for overlap of squares that need to be free, and occupied bb.
        if (!(Bitboards::castle(us, QUEENSIDE) & board.pieces())) {
            if (!board.is_attacked(Square(back_rank(us), FILED), us) &
                !board.is_attacked(Square(back_rank(us), FILEC), us)) {
                move = Move(KING, Square(back_rank(us), FILEE), Square(back_rank(us), FILEC));
                move.make_queen_castle();
                moves.push_back(move);
            }
        }
    }
    if (board.can_castle(us, KINGSIDE) && (side == KINGSIDE)) {
        if (!(Bitboards::castle(us, KINGSIDE) & board.pieces())) {
            if (!board.is_attacked(Square(back_rank(us), FILEF), us) &
                !board.is_attacked(Square(back_rank(us), FILEG), us)) {
                move = Move(KING, Square(back_rank(us), FILEE), Square(back_rank(us), FILEG));
                move.make_king_castle();
                moves.push_back(move);
            }
        }
    }
}

template <Colour us> void gen_castle_moves(const Board &board, MoveList &moves) {
    // Check if castling is legal, and if so add it to the move list. Assumes we aren't in check
    gen_castle_moves<us, KINGSIDE>(board, moves);
    gen_castle_moves<us, QUEENSIDE>(board, moves);
}

template <GenType gen> void gen_moves(const Board &board, MoveList &moves) {
    Colour us = board.who_to_play();
    Bitboard occ;
    Square ks = board.find_king(us);

    // If we are generating quiet moves, add castling
    if constexpr (gen == QUIET) {
        if (us == WHITE) {
            gen_castle_moves<WHITE>(board, moves);
        } else {
            gen_castle_moves<BLACK>(board, moves);
        }
    }

    if (us == WHITE) {
        gen_pawn_moves<WHITE, gen>(board, moves);
    } else {
        gen_pawn_moves<BLACK, gen>(board, moves);
    }

    // Pinned knights cannot move
    occ = board.pieces(us, KNIGHT) & ~board.pinned();
    while (occ) {
        const Square sq = pop_lsb(&occ);
        gen_moves<gen, KNIGHT>(board, sq, moves);
    }
    // Bishops which aren't pinned
    occ = board.pieces(us, BISHOP) & ~board.pinned();
    while (occ) {
        const Square sq = pop_lsb(&occ);
        gen_moves<gen, BISHOP>(board, sq, moves);
    }
    // Bisops which are pinned
    occ = board.pieces(us, BISHOP) & board.pinned();
    while (occ) {
        const Square sq = pop_lsb(&occ);
        const Bitboard target_squares = Bitboards::line(sq, ks);
        gen_moves<gen, BISHOP>(board, sq, moves, target_squares);
    }
    // Rooks which aren't pinned
    occ = board.pieces(us, ROOK) & ~board.pinned();
    while (occ) {
        const Square sq = pop_lsb(&occ);
        gen_moves<gen, ROOK>(board, sq, moves);
    }
    // Rooks which are pinned
    occ = board.pieces(us, ROOK) & board.pinned();
    while (occ) {
        const Square sq = pop_lsb(&occ);
        const Bitboard target_squares = Bitboards::line(sq, ks);
        gen_moves<gen, ROOK>(board, sq, moves, target_squares);
    }
    occ = board.pieces(us, QUEEN) & ~board.pinned();
    while (occ) {
        const Square sq = pop_lsb(&occ);
        gen_moves<gen, QUEEN>(board, sq, moves);
    }
    occ = board.pieces(us, QUEEN) & board.pinned();
    while (occ) {
        const Square sq = pop_lsb(&occ);
        const Bitboard target_squares = Bitboards::line(sq, ks);
        gen_moves<gen, QUEEN>(board, sq, moves, target_squares);
    }
    gen_moves<gen, KING>(board, ks, moves);
}

template <> void gen_moves<EVASIONS>(const Board &board, MoveList &moves) {
    Colour us = board.who_to_play();
    Bitboard occ;
    const Bitboard can_move = ~board.pinned();
    occ = board.pieces(us, KING);
    Square ks = board.find_king(us);
    gen_moves<QUIET, KING>(board, ks, moves);
    gen_moves<CAPTURES, KING>(board, ks, moves);

    Bitboard pieces_bb = board.pieces();
    // We can capture the checker
    Square ts = board.checkers(0);
    occ = Bitboards::attacks<KNIGHT>(pieces_bb, ts) & board.pieces(us, KNIGHT) & can_move;
    while (occ) {
        const Square sq = pop_lsb(&occ);
        Move move = Move(KNIGHT, sq, ts);
        move.make_capture();
        moves.push_back(move);
    }
    const Bitboard bq_atk = Bitboards::attacks<BISHOP>(pieces_bb, ts) & can_move;
    occ = bq_atk & board.pieces(us, BISHOP);
    while (occ) {
        const Square sq = pop_lsb(&occ);
        Move move = Move(BISHOP, sq, ts);
        move.make_capture();
        moves.push_back(move);
    }
    const Bitboard rq_atk = Bitboards::attacks<ROOK>(pieces_bb, ts) & can_move;
    occ = rq_atk & board.pieces(us, ROOK);
    while (occ) {
        const Square sq = pop_lsb(&occ);
        Move move = Move(ROOK, sq, ts);
        move.make_capture();
        moves.push_back(move);
    }
    occ = (rq_atk | bq_atk) & board.pieces(us, QUEEN);
    while (occ) {
        const Square sq = pop_lsb(&occ);
        Move move = Move(QUEEN, sq, ts);
        move.make_capture();
        moves.push_back(move);
    }

    if (us == WHITE) {
        gen_pawn_moves<WHITE, EVASIONS>(board, moves);
    } else {
        gen_pawn_moves<BLACK, EVASIONS>(board, moves);
    }

    // Or we can block the check
    Bitboard between_squares = Bitboards::between(ks, ts);

    occ = board.pieces(us, KNIGHT) & can_move;
    while (occ) {
        const Square sq = pop_lsb(&occ);
        gen_moves<QUIET, KNIGHT>(board, sq, moves, between_squares);
    }
    occ = board.pieces(us, BISHOP) & can_move;
    while (occ) {
        const Square sq = pop_lsb(&occ);
        gen_moves<QUIET, BISHOP>(board, sq, moves, between_squares);
    }
    occ = board.pieces(us, ROOK) & can_move;
    while (occ) {
        const Square sq = pop_lsb(&occ);
        gen_moves<QUIET, ROOK>(board, sq, moves, between_squares);
    }
    occ = board.pieces(us, QUEEN) & can_move;
    while (occ) {
        const Square sq = pop_lsb(&occ);
        gen_moves<QUIET, QUEEN>(board, sq, moves, between_squares);
    }
}

template <GenType gen> void generate_moves(const Board &board, MoveList &moves) {
    // Generate all legal non-evasion moves
    if constexpr (gen == CAPTURES) {
        gen_moves<CAPTURES>(board, moves);
    } else if constexpr (gen == LEGAL) {
        gen_moves<CAPTURES>(board, moves);
        gen_moves<QUIET>(board, moves);
    }
    return;
}

template <> void generate_moves<EVASIONS>(const Board &board, MoveList &moves) {
    // Generate all legal moves
    const Colour us = board.who_to_play();
    const Square king_square = board.find_king(us);
    const int number_checkers = board.number_checkers();
    if (number_checkers == 2) {
        // Double check. Generate king moves.
        gen_moves<QUIET, KING>(board, king_square, moves);
        gen_moves<CAPTURES, KING>(board, king_square, moves);
    } else {
        gen_moves<EVASIONS>(board, moves);
    }
    return;
}

MoveList Board::get_moves() const {
    MoveList moves;
    moves.reserve(MAX_MOVES);
    if (is_check()) {
        generate_moves<EVASIONS>(*this, moves);
    } else {
        generate_moves<LEGAL>(*this, moves);
    }
    return moves;
}

MoveList Board::get_quiessence_moves() const {
    MoveList moves;
    moves.reserve(MAX_MOVES);
    if (is_check()) {
        generate_moves<EVASIONS>(*this, moves);
    } else {
        // For later, currently unfortunately increases branching factor too much to be useful.
        // gen_moves<QUIETCHECKS>(*this, moves);
        generate_moves<CAPTURES>(*this, moves);
    }
    return moves;
}

MoveList Board::get_capture_moves() const {
    MoveList moves;
    moves.reserve(MAX_MOVES);
    generate_moves<CAPTURES>(*this, moves);
    return moves;
}

MoveList Board::get_evasion_moves() const {
    MoveList moves;
    moves.reserve(MAX_MOVES);
    generate_moves<EVASIONS>(*this, moves);
    return moves;
}

void Board::get_moves(MoveList &moves) const {
    if (is_check()) {
        generate_moves<EVASIONS>(*this, moves);
    } else {
        generate_moves<LEGAL>(*this, moves);
    }
}

void Board::get_capture_moves(MoveList &moves) const { generate_moves<CAPTURES>(*this, moves); }

void Board::get_evasion_moves(MoveList &moves) const { generate_moves<EVASIONS>(*this, moves); }

void Board::get_quiessence_moves(MoveList &moves) const {
    if (is_check()) {
        generate_moves<EVASIONS>(*this, moves);
    } else {
        // For later, currently unfortunately increases branching factor too much to be useful.
        // gen_moves<QUIETCHECKS>(*this, moves);
        generate_moves<CAPTURES>(*this, moves);
    }
}
