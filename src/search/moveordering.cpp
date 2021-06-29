#include "board.hpp"
#include "evaluate.hpp"
#include "types.hpp"
#include <algorithm>

bool cmp(Move &m1, Move &m2) {
    // Comparison for the sort function. We want to sort in reverse order, so better scoring moves come first.
    return m1.score > m2.score;
}

namespace Ordering {
void sort_moves(Board &board, MoveList &legal_moves, const DenseMove hash_dmove, const KillerTableRow killer_moves) {
    MoveList checks, quiet_moves, good_captures, bad_captures, sorted_moves, killer;
    size_t n_moves = legal_moves.size();
    checks.reserve(n_moves);
    quiet_moves.reserve(n_moves);
    good_captures.reserve(n_moves);
    // The moves from get_moves already have captures first
    for (Move move : legal_moves) {
        if (move == hash_dmove) {
            // The search handles the hash move itself. Here we just make sure it doesn't end up in the final move list.
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
