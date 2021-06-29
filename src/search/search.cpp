#include "search.hpp"
#include "evaluate.hpp"
#include "ordering.hpp"
#include "transposition.hpp"
#include "uci.hpp"
#include <chrono>
#include <iostream>
#include <time.h>

constexpr size_t futility_max_depth = 3;
constexpr score_t futility_margins[] = {0, 330, 500, 900};
constexpr depth_t null_move_depth_reduction = 2;

score_t Search::scout_search(Board &board, depth_t depth, const score_t alpha, my_clock::time_point time_cutoff,
                             const bool allow_cutoff, const bool allow_null, SearchOptions &options) {
    // Perform a null window 'scout' search on a subtree.
    // All nodes examined with this tree are not PV nodes (unless proven otherwise, when they should be researched)
    // Has bounds [alpha, alpha + 1]
    const score_t beta = alpha + 1;

    // Check extentions
    if (board.is_check()) {
        depth++;
    }

    MoveList legal_moves = board.get_moves();
    // Terminal node.
    if (legal_moves.size() == 0) {
        return Evaluation::evaluate(board, legal_moves);
    }

    // If this is a draw by repetition or insufficient material, return the drawn score.
    if (board.is_draw()) {
        return Evaluation::drawn_score(board);
    }

    // The absolute upper bound for a score on this node is ply_to_mate_score(board.ply()).
    if (ply_to_mate_score(board.ply()) <= alpha) {
        // We've already found a mate at a lower ply than this node, we can't do better.
        // Fail low.
        return ply_to_mate_score(board.ply());
    }

    // The absolute lower bound for a score on this node is -ply_to_mate_score(board.ply()).
    // (That is, we are being mated on this node)
    if (-ply_to_mate_score(board.ply()) >= beta) {
        // Fail high.
        return -ply_to_mate_score(board.ply());
    }

    // Leaf node for main tree.
    if (depth == 0) {
        return quiesce(board, alpha, beta, options);
    }

    // Lookup position in transposition table.
    DenseMove hash_dmove = NULL_DMOVE;
    const long hash = board.hash();
    if (Cache::transposition_table.probe(hash)) {
        const Cache::TransElement hit = Cache::transposition_table.last_hit();
        if (hit.depth() >= depth) {
            const score_t tt_eval = hit.eval(board.ply());
            if (hit.lower()) {
                // The saved score is a lower bound for the score of the sub tree
                if (tt_eval >= beta) {
                    // beta cutoff
                    return tt_eval;
                }
            } else if (hit.upper()) {
                // The saved score is an upper bound for the score of the subtree.
                if (tt_eval <= alpha) {
                    // rare negamax alpha-cutoff
                    return tt_eval;
                }
            } else {
                // The saved score is an exact value for the subtree
                return tt_eval;
            }
        }
        hash_dmove = hit.move();
    }

    // Check if we've passed our time cutoff
    if (allow_cutoff && (options.nodes % 1000 == 0)) {
        my_clock::time_point time_now = my_clock::now();
        if (time_now > time_cutoff) {
            options.set_stop();
            return MAX_SCORE;
        }
    }

    // Reverse futility pruning
    if (!board.is_endgame() && allow_null && depth <= futility_max_depth && !board.is_check()) {
        const score_t node_heuristic = Evaluation::negamax_heuristic(board);
        if (node_heuristic - futility_margins[depth] >= beta) {
            return node_heuristic - futility_margins[depth];
        }
    }

    // Null move pruning
    if (!board.is_endgame() && allow_null && (depth > null_move_depth_reduction) && !board.is_check()) {
        board.make_nullmove();
        score_t score = -scout_search(board, depth - 1 - null_move_depth_reduction, -alpha - 1, time_cutoff,
                                      allow_cutoff, false, options);
        board.unmake_nullmove();
        if (score >= beta) {
            // beta cutoff
            return score;
        }
    }

    KillerTableRow killer_move = Cache::killer_table.probe(board.ply());

    score_t best_score = MIN_SCORE;
    Move best_move = NULL_MOVE;
    Move hash_move = unpack_move(hash_dmove, legal_moves);

    // Do the hash move explicitly to avoid sorting moves if our hash moves provides a beta-cutoff
    if (!(hash_move == NULL_MOVE)) {
        board.make_move(hash_move);
        options.nodes++;
        best_score = -scout_search(board, depth - 1, -alpha - 1, time_cutoff, allow_cutoff, true, options);
        board.unmake_move(hash_move);
        best_move = hash_move;

        if (options.stop()) {
            return MAX_SCORE;
        }

        if (best_score >= beta) {
            Cache::killer_table.store(board.ply(), best_move);
            Cache::transposition_table.store(hash, best_score, alpha, beta, depth, best_move, board.ply());
            return best_score;
        }
    }

    Ordering::sort_moves(board, legal_moves, hash_dmove, killer_move);
    for (Move move : legal_moves) {
        board.make_move(move);
        options.nodes++;
        score_t score = -scout_search(board, depth - 1, -beta, time_cutoff, allow_cutoff, true, options);
        board.unmake_move(move);
        if (options.stop()) {
            return MAX_SCORE;
        }
        if (score > best_score) {
            best_score = score;
            best_move = move;
        }
        if (best_score >= beta) {
            // beta-cutoff
            break;
        }
    }
    Cache::killer_table.store(board.ply(), best_move);
    Cache::transposition_table.store(hash, best_score, alpha, beta, depth, best_move, board.ply());
    return best_score;
}

score_t Search::pv_search(Board &board, depth_t depth, const score_t alpha_start, const score_t beta,
                          PrincipleLine &line, my_clock::time_point time_cutoff, const bool allow_cutoff,
                          SearchOptions &options) {
    // Perform an alpha-beta pruning tree search.
    // The use of this function implies that the node is a PV-node.
    // For non-PV nodes, use scout_search.
    // Has bounds [alpha, beta]

    score_t alpha = alpha_start;

    // Check extentions
    if (board.is_check()) {
        depth++;
    }

    MoveList legal_moves = board.get_moves();
    // Terminal node
    if (legal_moves.size() == 0) {
        return Evaluation::evaluate(board, legal_moves);
    }

    // If this is a draw by repetition or insufficient material, return the drawn score.
    if (board.is_draw() && !board.is_root()) {
        return Evaluation::drawn_score(board);
    }

    // The absolute upper bound for a score on this node is ply_to_mate_score(board.ply()).
    if (ply_to_mate_score(board.ply()) <= alpha) {
        // We've already found a mate at a lower ply than this node, we can't do better.
        // Fail low.
        return ply_to_mate_score(board.ply());
    }

    // The absolute lower bound for a score on this node is -ply_to_mate_score(board.ply()).
    // (That is, we are being mated on this node)
    if (-ply_to_mate_score(board.ply()) >= beta) {
        // Fail high.
        return -ply_to_mate_score(board.ply());
    }

    // Leaf node for the main tree.
    if (depth == 0) {
        return quiesce(board, alpha, beta, options);
    }

    // Lookup position in transposition table for hashmove.
    DenseMove hash_dmove = NULL_DMOVE;
    KillerTableRow killer_move = Cache::killer_table.probe(board.ply());
    const long hash = board.hash();
    if (Cache::transposition_table.probe(hash)) {
        const Cache::TransElement hit = Cache::transposition_table.last_hit();
        hash_dmove = hit.move();
    }

    // Check if we've passed our time cutoff
    if (allow_cutoff && (options.nodes % 1000 == 0)) {
        my_clock::time_point time_now = my_clock::now();
        if (time_now > time_cutoff) {
            options.set_stop();
            return MAX_SCORE;
        }
    }

    bool is_first_child = true;
    PrincipleLine pv;
    score_t best_score = MIN_SCORE;
    Move hash_move = unpack_move(hash_dmove, legal_moves);

    // Do the hash move explicitly to avoid sorting moves if our hash moves provides a beta-cutoff
    if (!(hash_move == NULL_MOVE)) {
        PrincipleLine temp_line;
        temp_line.reserve(16);
        board.make_move(hash_move);
        options.nodes++;
        best_score = -pv_search(board, depth - 1, -beta, -alpha, pv, time_cutoff, allow_cutoff, options);
        board.unmake_move(hash_move);
        pv.push_back(hash_move);

        if (options.stop()) {
            return MAX_SCORE;
        }
        alpha = std::max(alpha, best_score);
        if (alpha >= beta) {
            line = pv;
            Cache::killer_table.store(board.ply(), pv.back());
            Cache::transposition_table.store(hash, best_score, alpha_start, beta, depth, pv.back(), board.ply());
            return best_score;
        }
        is_first_child = false;
    }

    // Sort the remaining moves, and remove the hash move if it exists
    Ordering::sort_moves(board, legal_moves, hash_dmove, killer_move);

    for (Move move : legal_moves) {
        PrincipleLine temp_line;
        temp_line.reserve(16);

        board.make_move(move);
        options.nodes++;
        score_t score;
        if (is_first_child) {
            score = -pv_search(board, depth - 1, -beta, -alpha, temp_line, time_cutoff, allow_cutoff, options);
        } else {
            // Search with a null window
            score = -scout_search(board, depth - 1, -alpha - 1, time_cutoff, allow_cutoff, true, options);
            if (score > alpha && score < beta) {
                // Do a full search
                score = -pv_search(board, depth - 1, -beta, -alpha, temp_line, time_cutoff, allow_cutoff, options);
            }
        }
        board.unmake_move(move);

        if (options.stop()) {
            return MAX_SCORE;
        }

        if (score > best_score) {
            best_score = score;
            pv = temp_line;
            pv.push_back(move);
        }
        alpha = std::max(alpha, score);
        if (alpha >= beta) {
            break; // beta-cutoff
        }
        is_first_child = false;
    }
    line = pv;

    Cache::killer_table.store(board.ply(), pv.back());
    Cache::transposition_table.store(hash, best_score, alpha_start, beta, depth, pv.back(), board.ply());
    return best_score;
}

score_t Search::quiesce(Board &board, const score_t alpha_start, const score_t beta, SearchOptions &options) {
    // perform quiesence search to evaluate only quiet positions.
    score_t alpha = alpha_start;
    score_t stand_pat = Evaluation::negamax_heuristic(board);

    // Beta cutoff
    if (stand_pat >= beta) {
        return stand_pat;
    }

    alpha = std::max(alpha, stand_pat);

    // If this is a draw by repetition or insufficient material, return the drawn score.
    if (board.is_draw()) {
        return Evaluation::drawn_score(board);
    }

    // Delta pruning
    if (stand_pat < alpha - 900) {
        return stand_pat;
    }

    // Get a list of moves for quiessence. If it's check, it generates all check evasions. Otherwise it's just captures.
    MoveList moves = board.get_quiessence_moves();

    if (moves.size() == 0) {
        return stand_pat;
    }

    // Sort the captures.
    Ordering::sort_moves(board, moves, NULL_DMOVE, NULL_KROW);

    for (Move move : moves) {
        board.make_move(move);
        options.nodes++;
        score_t score = -quiesce(board, -beta, -alpha, options);
        board.unmake_move(move);
        alpha = std::max(alpha, score);
        if (alpha >= beta) {
            break; // beta-cutoff
        }
    }
    return alpha;
}

score_t Search::search(Board &board, const depth_t max_depth, const int max_millis, PrincipleLine &line,
                       SearchOptions &options) {
    // Initialise the transposition table.
    Cache::transposition_table.set_delete();

    PrincipleLine principle;
    board.set_root();

    score_t score = 0;

    // Initialise variables for time control.
    my_clock::time_point time_origin, time_now, time_cutoff;
    std::chrono::duration<double, std::milli> time_span, time_span_last, t_est;
    // Time at start of search.
    time_origin = my_clock::now();
    // Time when the search should be stopped
    time_cutoff = time_origin + std::chrono::milliseconds((int)(1.2 * max_millis));
    // Estimate effective branching factor empirically
    int branching_factor = 4;

    bool allow_cutoff = false;

    // Offsets from our score guess for our aspiration window in cp.
    // When it gets to the end it just sets the limit it's failing on to MATING_SCORE
    constexpr score_t aspration_windows[] = {30, 80, 200, 500};
    constexpr size_t n_aw = sizeof(aspration_windows) / sizeof(score_t);

    // Iterative deepening
    for (depth_t depth = 2; depth <= max_depth; depth++) {
        PrincipleLine temp_line;
        temp_line.reserve(depth);
        score_t new_score;

        // Aspiration windows.
        // 'score', from the previous iteration, acts as a guess we set our bounds around.
        score_t alpha = score - aspration_windows[0];
        score_t beta = score + aspration_windows[0];
        for (size_t aw = 0; aw <= n_aw; aw++) {
            new_score = pv_search(board, depth, alpha, beta, temp_line, time_cutoff, allow_cutoff, options);

            // Exit search if we've been asked to stop.
            if (options.stop()) {
                break;
            }

            // Check the score against the bounds
            if (new_score <= alpha) {
                // Score is an upper bound
                if (is_mating(-new_score)) {
                    // We can skip some steps here
                    alpha = -MATING_SCORE;
                } else if (aw < n_aw - 1) {
                    alpha = score - aspration_windows[aw + 1];
                    alpha = std::max(alpha, (score_t)-MATING_SCORE);
                } else {
                    // If we are at the end of the listed bounds, just set the limit to the mating score.
                    alpha = -MATING_SCORE;
                }
            } else if (new_score >= beta) {
                // Score is a lower bound.
                if (is_mating(new_score)) {
                    // We can skip some steps here
                    beta = MATING_SCORE;
                } else if (aw < n_aw - 1) {
                    beta = score + aspration_windows[aw + 1];
                    beta = std::min(beta, (score_t)MATING_SCORE);
                } else {
                    // If we are at the end of the listed bounds, just set the limit to the mating score.
                    beta = MATING_SCORE;
                }
            } else {
                // Score is exact.
                break;
            }
        }

        // Check if we've been sent a stop signal.
        if (options.stop()) {
            break;
        }
        // Allow a forced stop after at least some calculation has been done.
        allow_cutoff = true;
        // Score is saved to temporary variable new_score so that we still have the last valid score if the search is
        // stopped by force.
        score = new_score;
        principle = temp_line;

        // Calculate the time spent so far.
        time_now = my_clock::now();
        time_span = time_now - time_origin;
        unsigned long nps = int(1000 * (options.nodes / time_span.count()));

        // Send the info for the search to uci
        uci_info(depth, score, options.nodes, nps, principle, (int)time_span.count(), board.get_root());

        // If we've found a forced mate, just play it.
        if (is_mating(score)) {
            break;
        }

        // Estimate the next time span.
        t_est = branching_factor * time_span;

        // Check if our estimate of the next depth will take us over our time limit
        if (int(t_est.count()) > max_millis) {
            break;
        }

        // Calculate the last effective branching factor
        if (depth >= 5) {
            branching_factor = int(time_span.count() / time_span_last.count());
        }

        time_span_last = time_span;
    }
    line = principle;
    return score;
}

score_t Search::search(Board &board, const depth_t depth, PrincipleLine &line) {
    SearchOptions options = SearchOptions();
    return search(board, depth, POS_INF, line, options);
}