#include "board.hpp"
#include "evaluate.hpp"
#include "printing.hpp"
#include "types.hpp"
#include <algorithm>
#include <assert.h>
#include <iostream>

bool cmp(Move &m1, Move &m2) {
    // Comparison for the sort function. We want to sort in reverse order, so better scoring moves come first.
    return m1.score > m2.score;
}

namespace Ordering {

Bitboard get_smallest_attacker(Board &board, const Square origin, const Bitboard mask, const Colour side) {
    // Return the smallest attacker on a given square, considering only pieces in the mask.
    // Has some issues with en-passent.

    Bitboard atk = Bitboards::pawn_attacks(~side, origin) & board.pieces(side, PAWN) & mask;
    if (atk) {
        return atk;
    }

    atk = Bitboards::pseudo_attacks(KNIGHT, origin) & board.pieces(side, KNIGHT) & mask;
    if (atk) {
        return atk;
    }

    // Sliding moves.
    // The handling of xraying will be done by the masking out some pieces as they are 'removed' by SEE.
    Bitboard occ = board.pieces() & mask;
    atk = bishop_attacks(occ, origin) & board.pieces(side, BISHOP) & mask;
    if (atk) {
        return atk;
    }

    atk = rook_attacks(occ, origin) & board.pieces(side, ROOK) & mask;
    if (atk) {
        return atk;
    }

    atk = (rook_attacks(occ, origin) | bishop_attacks(occ, origin)) & board.pieces(side, QUEEN) & mask;
    if (atk) {
        return atk;
    }

    atk = Bitboards::pseudo_attacks(KING, origin) & board.pieces(side, KING) & mask;
    if (atk) {
        return atk;
    }

    return Bitboards::null;
}

score_t see(Board &board, const Square target, Colour side, PieceType pt, Bitboard mask) {
    // Static exchange evaluation. Looks at the possible gain to be made by trading on square 'target'.
    // Side is the colour of the side to move.
    // PT is the piece type that is on that square (not looked up as we want to evaluate moves statically)
    // Mask is a bitboard mask for pieces to consider in the evaluation.
    Bitboard smallest_atk = get_smallest_attacker(board, target, mask, side);
    // Vector the the relative gain on the square for each iteration of exchanges.
    std::vector<score_t> gain;
    gain.reserve(16);
    depth_t depth = 1;
    score_t value = Evaluation::piece_material(pt);
    gain.push_back(value);
    while (smallest_atk) {
        const Square atksq = lsb(smallest_atk);
        PieceType p = board.piece_type(atksq);
        value = Evaluation::piece_material(p);
        gain.push_back(value - gain.back());
        // Remove the smallest attacker
        mask ^= smallest_atk;
        // Switch the sides
        side = ~side;
        depth++;
        smallest_atk = get_smallest_attacker(board, target, mask, side);
    }
    // Gain has (depth) elements, we start at depth - 2 because this is the second to last element.
    // The last element is the gain if the final piece were udner attack, but it's not (that's why it's last).

    if (depth == 1) {
        // There were no captures to be made.
        return 0;
    } else {
        for (depth_t d = depth - 2; d > 0; d--) {
            // The oponent won't give us material, so the gain at depth d-1 is going to be whichever is worse of if they
            // capture or don't.
            gain[d - 1] = std::min(gain[d - 1], (score_t)-gain[d]);
        }

        // Can chose not to capture, with gain 0.
        return std::max(gain[0], (score_t)0);
    }
};

score_t see_capture(Board &board, const Move move) {
    assert(move.is_capture());
    assert(move != NULL_MOVE);
    Bitboard mask = Bitboards::omega ^ move.origin;
    score_t see_gain = see(board, move.target, ~board.who_to_play(), move.moving_piece, mask);
    score_t cap_gain;
    if (move.is_ep_capture()) {
        cap_gain = Evaluation::piece_material(PAWN);
    } else {
        cap_gain = Evaluation::piece_material(board.piece_type(move.target));
    }
    score_t gain = cap_gain - see_gain;
    return gain;
}

void sort_moves(Board &board, MoveList &legal_moves, const DenseMove hash_dmove, const KillerTableRow killer_moves) {
    MoveList checks, quiet_moves, good_captures, bad_captures, sorted_moves, killer;
    size_t n_moves = legal_moves.size();
    checks.reserve(n_moves);
    quiet_moves.reserve(n_moves);
    good_captures.reserve(n_moves);
    // The moves from get_moves already have captures first
    for (Move move : legal_moves) {
        if (move == hash_dmove) {
            // The search handles the hash move itself. Here we just make sure it doesn't end up in the final move
            // list.
        } else if (move == killer_moves) {
            killer.push_back(move);
        } else if (board.gives_check(move)) {
            checks.push_back(move);
        } else if (move.is_ep_capture()) {
            // En-passent is weird too.
            move.captured_piece = PAWN;
            good_captures.push_back(move);
        } else if (move.is_capture()) {
            // Make sure to lookup and record the piece captured
            move.captured_piece = board.piece_type(move.target);
            move.score =
                Evaluation::piece_material(move.captured_piece) - Evaluation::piece_material(move.moving_piece);
            if (move.score >= 0) {
                // Captures of a more valuable piece with a less valuable piece are almost always good.
                good_captures.push_back(move);
            } else {
                bad_captures.push_back(move);
            }
        } else {
            quiet_moves.push_back(move);
        }
    }
    legal_moves.clear();
    legal_moves.insert(legal_moves.end(), checks.begin(), checks.end());
    std::sort(good_captures.begin(), good_captures.end(), cmp);
    std::sort(bad_captures.begin(), bad_captures.end(), cmp);
    legal_moves.insert(legal_moves.end(), good_captures.begin(), good_captures.end());
    legal_moves.insert(legal_moves.end(), killer.begin(), killer.end());
    legal_moves.insert(legal_moves.end(), bad_captures.begin(), bad_captures.end());
    legal_moves.insert(legal_moves.end(), quiet_moves.begin(), quiet_moves.end());
}
} // namespace Ordering
