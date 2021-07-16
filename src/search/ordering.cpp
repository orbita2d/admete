#include "ordering.hpp"
#include "board.hpp"
#include "evaluate.hpp"
#include "printing.hpp"
#include "transposition.hpp"
#include "types.hpp"
#include <algorithm>
#include <assert.h>
#include <iostream>

bool cmp(const Move &m1, const Move &m2) {
    // Comparison for the sort function. Should return true if m1 goes before m2.
    return m1.score > m2.score;
}

namespace SEE {
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

// Static exchange evaluation. Looks at the possible gain to be made by trading on square 'target'.
// Side is the colour of the side to move.
// PT is the piece type that is on that square (not looked up as we want to evaluate moves statically)
// Mask is a bitboard mask for pieces to consider in the evaluation.
score_t see(Board &board, const Square target, Colour side, PieceType pt, Bitboard mask) {
    Bitboard smallest_atk = get_smallest_attacker(board, target, mask, side);
    // Vector the the relative gain on the square for each iteration of exchanges.
    std::vector<score_t> gain;
    gain.reserve(16);
    int depth = 1;
    score_t value = SEE::material[pt];
    gain.push_back(value);
    while (smallest_atk) {
        const Square atksq = lsb(smallest_atk);
        PieceType p = board.piece_type(atksq);
        value = SEE::material[p];
        gain.push_back(value - gain.back());
        // Remove the smallest attacker
        mask ^= smallest_atk;
        // Switch the sides
        side = ~side;
        depth++;
        smallest_atk = get_smallest_attacker(board, target, mask, side);
    }

    if (depth == 1) {
        // There were no captures to be made.
        return 0;
    } else {
        // Gain has (depth) elements, we start at depth - 3 because this compares to the second to last element.
        // The last element is the gain if the final piece were under attack, but it's not (that's why it's last)
        for (int d = depth - 3; d >= 0; d--) {
            // The oponent won't give us material, so the gain at depth d-1 is going to be whichever is worse (for us)
            // of if they capture or don't.
            // Their gain at depth d, is (gain[d]), our gain if they do that is -gain[d]. (That is, if they capture)
            // If they don't capture, our gain is gain[d-1], because they do something else.
            gain[d] = std::min(gain[d], (score_t)-gain[d + 1]);
        }

        // Can chose not to capture, with gain 0.
        return std::max(gain[0], (score_t)0);
    }
};

score_t see_capture(Board &board, const Move move) {
    assert(move.is_capture());
    assert(move != NULL_MOVE);
    Bitboard mask = Bitboards::omega ^ move.origin;
    score_t cap_gain;
    if (move.is_ep_capture()) {
        const Square captured_square(move.origin.rank(), move.target.file());
        mask ^= sq_to_bb(captured_square);
        cap_gain = SEE::material[PAWN];
    } else {
        cap_gain = SEE::material[board.piece_type(move.target)];
    }
    const score_t see_gain = see(board, move.target, ~board.who_to_play(), move.moving_piece, mask);
    return cap_gain - see_gain;
}

score_t see_quiet(Board &board, const Move move) {
    assert(move.is_quiet());
    assert(move != NULL_MOVE);
    Bitboard mask = Bitboards::omega ^ move.origin;
    const score_t see_gain = see(board, move.target, ~board.who_to_play(), move.moving_piece, mask);
    return -see_gain;
}

bool see(Board &board, const Move move, const int threshold) {
    assert(move != NULL_MOVE);
    assert(move.is_capture());
    Bitboard mask = Bitboards::omega ^ move.origin;
    int last_atk_material = SEE::material[move.moving_piece];

    // Initial gain is just capturing the piece, this is an upper bound.
    // Gain is gain if our last moves piece is recaptured.
    int gain = SEE::material[board.piece_type(move.target)] - threshold;
    if (move.is_ep_capture()) {
        gain = SEE::material[PAWN] - threshold;
        const Square captured_square(move.origin.rank(), move.target.file());
        mask ^= sq_to_bb(captured_square);
    }

    if (gain < 0) {
        return false;
    }
    // Gain after our piece being recaptured is a lower bound.
    if (gain >= last_atk_material) {
        return true;
    }
    Colour side = ~board.who_to_play();
    const Square target = move.target;
    Bitboard smallest_atk = get_smallest_attacker(board, target, mask, side);
    while (smallest_atk) {
        const Square atksq = lsb(smallest_atk);
        PieceType p = board.piece_type(atksq);
        // The gain from a capture is the capture - the other side's gain so far.
        gain = last_atk_material - gain - 1;
        last_atk_material = SEE::material[p];
        if (gain - last_atk_material >= 0) {
            // Even if our piece is recaptured, we are still winning, early exit.
            side = ~side;
            break;
        }
        // Remove the smallest attacker
        mask ^= smallest_atk;
        // Switch the sides
        side = ~side;
        smallest_atk = get_smallest_attacker(board, target, mask, side);
    }
    // At the end, if we didn't early exit, side is set to the player who couldn't recap.
    return side != board.who_to_play();
}

} // namespace SEE

namespace Ordering {
void sort_moves(MoveList &legal_moves) { std::sort(legal_moves.begin(), legal_moves.end(), cmp); }
void rank_and_sort_moves(Board &board, MoveList &legal_moves, const DenseMove hash_dmove) {

    KillerTableRow killer_moves = Cache::killer_table.probe(board.ply());
    for (Move &move : legal_moves) {
        if (move == hash_dmove) {
            // The search handles the hash move itself. Here we just make sure it doesn't end up in the final move
            // list.
            move.score = 1000000;
        } else if (move == killer_moves) {
            move.score = 200000;
        } else if (move.is_capture()) {
            // Make sure to lookup and record the piece captured
            move.captured_piece = board.piece_type(move.target);
            move.score = SEE::see_capture(board, move);
            if (board.gives_check(move)) {
                move.score += 5000;
            }
            if (SEE::see(board, move, 50)) {
                move.score = 400000;
            } else if (SEE::see(board, move, -50)) {
                move.score = 150000;
            } else {
                move.score = -400000;
            }
        } else if (move.is_promotion()) {
            move.score = 100000 + SEE::material[get_promoted(move)];
        } else {
            // Quiet move.
            move.score = std::min(Cache::history_table.probe(move), 100000u);
            if (board.gives_check(move)) {
                move.score += 100000;
            }
        }
    }

    sort_moves(legal_moves);
    // Move order is:
    // Captures with SEE > 50
    // Killer moves
    // Captures -50 < SEE < 50
    // Quiet moves, sorted by history heuristic
    // Captures with SEE < -50
}
} // namespace Ordering