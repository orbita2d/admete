#include "search.hpp"
#include "evaluate.hpp"
#include "ordering.hpp"
#include "tablebase.hpp"
#include "transposition.hpp"
#include "uci.hpp"
#include <algorithm>
#include <assert.h>
#include <chrono>
#include <iostream>
#include <math.h>
#include <time.h>

constexpr depth_t null_move_depth_reduction = 2;
constexpr score_t extended_futility_margins[] = {0, 200, 700};
constexpr score_t reverse_futility_margins[] = {0, 200, 400, 800};
constexpr depth_t efp_max_depth = sizeof(extended_futility_margins) / sizeof(score_t) - 1;
constexpr depth_t rfp_max_depth = sizeof(reverse_futility_margins) / sizeof(score_t) - 1;
constexpr depth_t probcut_depth_reduction = 3;
constexpr depth_t probcut_min_depth = 6;
constexpr score_t probcut_margin = 300;

// Offsets from our score guess for our aspiration window in cp.
// When it gets to the end it just sets the limit it's failing on to MATING_SCORE
constexpr score_t aspration_windows[] = {30, 80, 200, 500};
constexpr size_t n_aw = sizeof(aspration_windows) / sizeof(score_t);

score_t Search::scout_search(Board &board, depth_t depth, const score_t alpha, unsigned int time_cutoff,
                             const bool allow_cutoff, const bool allow_null, NodeType node, SearchOptions &options) {
    /* Perform a null window 'scout' search on a subtree.
     * All nodes examined with this tree are not PV nodes (unless proven otherwise, when they should be re-searched)
     * Has bounds [alpha, alpha + 1]
     */
    assert(node != PVNODE);
    assert(depth < MAX_DEPTH);
    const score_t beta = alpha + 1;

    // Check extentions
    if (board.is_check()) {
        depth++;
    }
    const zobrist_t hash = board.hash();
    // Try to prefetch the transposition table entry.
    Cache::transposition_table.prefetch(hash);

    MoveList legal_moves = board.get_moves();
    // Terminal node.
    if (legal_moves.empty()) {
        return Evaluation::terminal(board);
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

    // Max ply
    if (board.ply() >= MAX_PLY) {
        return Evaluation::eval(board);
    }

    // Leaf node for main tree.
    if (depth == 0) {
        return quiesce(board, alpha, beta, options);
    }

    // Lookup position in transposition table.
    DenseMove hash_dmove = NULL_DMOVE;
    Cache::TransElement tthit;
    if (Cache::transposition_table.probe(hash, tthit)) {
        if (tthit.depth() >= depth) {
            const score_t tt_eval = tthit.eval(board.ply());
            if (tthit.lower()) {
                // The saved score is a lower bound for the score of the sub tree
                if (tt_eval >= beta) {
                    // Fail high
                    return tt_eval;
                }
            } else if (tthit.upper()) {
                // The saved score is an upper bound for the score of the subtree.
                if (tt_eval <= alpha) {
                    // Fail low
                    return tt_eval;
                }
            } else {
                // The saved score is an exact value for the subtree
                return tt_eval;
            }
        }
        hash_dmove = tthit.move();
    }

    // Check if we've passed our time cutoff
    if (allow_cutoff && (options.nodes % (1<<10) == 0)) {
        if (options.get_millis() > time_cutoff) {
            options.set_stop();
            return MAX_SCORE;
        }
    }

    score_t score_ub = MAX_SCORE;
    score_t best_score = MIN_SCORE;
    // Probe the tablebase
    if (options.tbenable) {
        score_t tbresult;
        Bounds bounds;
        if (Tablebase::probe_wdl(board, tbresult, bounds)) {
            options.tbhits++;
            if (bounds == UPPER) {
                // TB result is an upper bound (i.e. TBLOSS)
                if (tbresult <= alpha) {
                    Cache::transposition_table.store(hash, tbresult, bounds, MAX_DEPTH, NULL_MOVE, board.ply());
                    return tbresult;
                } else {
                    score_ub = tbresult;
                }
            } else if (bounds == LOWER) {
                // TB Result is a lower bound
                if (tbresult >= beta) {
                    Cache::transposition_table.store(hash, tbresult, bounds, MAX_DEPTH, NULL_MOVE, board.ply());
                    return tbresult;
                } else {
                    best_score = tbresult;
                }
            } else {
                // The TB score is exact.
                Cache::transposition_table.store(hash, tbresult, bounds, MAX_DEPTH, NULL_MOVE, board.ply());
                return tbresult;
            }
        }
    }
    // Occassionally refresh the neural network accumulator (because floating point error will accumulate)
    if (options.nodes % (1<<14) == 0) {
        board.refresh_accumulator();
    }

    // Calculate the node evaluation heuristic.
    const score_t node_eval = Evaluation::eval(board);

    // Reverse futility pruning
    // Prune if this node is almost certain to fail high.
    if (!board.is_endgame() && allow_null && depth <= rfp_max_depth && !board.is_check()) {
        if (node_eval - reverse_futility_margins[depth] >= beta) {
            return node_eval - reverse_futility_margins[depth];
        }
    }

    // Null move pruning.
    // Making a null move, in most cases, should be the worst option and give us an approximate lower bound on the score for this node.
    if (!board.is_endgame() && allow_null && (depth > null_move_depth_reduction) && !board.is_check()) {
        board.make_nullmove();
        score_t score = -scout_search(board, depth - 1 - null_move_depth_reduction, -alpha - 1, time_cutoff,
                                      allow_cutoff, false, CUTNODE, options);
        board.unmake_nullmove();
        if (score >= beta) {
            // beta cutoff
            return score;
        }
    }

    // Probcut.
    // We expect a search at a lower depth to give us a close score to the real score. If it would beat beta by some
    // margin, then we can probably cut safely.
    if (depth >= probcut_min_depth && beta < TBWIN_MIN && beta > -TBWIN_MIN) {
        // Beta-cut
        const score_t probcut_threshold = beta + probcut_margin;
        const score_t probcut_score = scout_search(board, depth - probcut_depth_reduction, probcut_threshold - 1, time_cutoff, allow_cutoff, allow_null, node, options);
        if (probcut_score >= probcut_threshold) {
            return probcut_score;
        }
    }

    Move best_move = NULL_MOVE;
    Move hash_move = unpack_move(hash_dmove, legal_moves);

    // Do the hash move explicitly to avoid sorting moves if our hash moves provides a beta-cutoff
    if (hash_move != NULL_MOVE) {
        board.make_move(hash_move);
        options.nodes++;
        // We expect first child of a cut node to be an all node, such that it would cause the cut node to fail high.
        best_score = -scout_search(board, depth - 1, -alpha - 1, time_cutoff, allow_cutoff, true,
                                   node == CUTNODE ? ALLNODE : CUTNODE, options);
        board.unmake_move(hash_move);
        best_move = hash_move;

        if (options.stop()) {
            return MAX_SCORE;
        }

        if (best_score >= beta) {
            Cache::killer_table.store(board.ply(), best_move);
            Cache::history_table.store(depth, best_move);
            best_score = std::min(best_score, score_ub);
            Cache::transposition_table.store(hash, best_score, LOWER, depth, best_move, board.ply());
            return best_score;
        }
    }

    Ordering::rank_and_sort_moves(board, legal_moves, hash_dmove);
    uint counter = 0;
    for (Move move : legal_moves) {
        // We've already dealt with the hashmove.
        if (move == hash_move) {
            continue;
        }
        counter++;
        const bool gives_check = board.gives_check(move);
        if ((node == CUTNODE) && (counter >= 5)) {
            // In a cut node, the cut is most likely to happen early, if we get through the hash move and and first few
            // other moves without a cut, this is probably actually an All node.
            node = ALLNODE;
        }
        NodeType child = node == CUTNODE ? ALLNODE : CUTNODE;

        depth_t search_depth = depth - 1;

        // Skip pruning for checks, promotions, or evasions.
        if (!gives_check && !board.is_check() && !move.is_promotion()) {

            // Late move reductions:
            // At an expected All node, the most likely moves to prove us wrong and fail high are
            // one's ranked earliest in move ordering. We can be less careful about proving later moves.
            if ((node == ALLNODE) && (counter >= 2) && move.is_quiet()) {
                search_depth -= reductions_table[0][depth][counter];
            }

            if ((node == ALLNODE) && (counter >= 3) && move.is_capture()) {
                search_depth -= reductions_table[1][depth][counter];
            }

            // SEE reductions
            // If the SEE for a capture is very bad, we can search to a lower depth as it's unlikely to cause a cut.
            if ((node == ALLNODE) && move.is_capture() && !SEE::see(board, move, -100)) {
                search_depth--;
            }

            // History pruning
            // On a quiet move, the score is a history score. If this is low, it's less likely to cause a beta cutoff.
            if ((node == ALLNODE) && (counter > 3) && move.is_quiet() && (search_depth < 3) && move.score < 15) {
                continue;
            }

            // Extended futility pruning
            // At frontier nodes (depth == 1, search_depth == 0), prune moves which have no chance of raising alpha.
            // At pre-frontier nodes (depth == 2), we can prune moves similarly, but with a much higher threshold.
            if ((counter > 1) && move.is_capture() && (depth <= efp_max_depth) &&
                !SEE::see(board, move, alpha - node_eval - extended_futility_margins[depth])) {
                continue;
            }

            if ((counter > 1) && move.is_quiet() && (depth <= efp_max_depth) &&
                (node_eval + extended_futility_margins[depth] <= alpha)) {
                continue;
            }
        }

        search_depth = std::clamp(search_depth, (depth_t)0, (depth_t)(depth - 1));

        board.make_move(move);
        options.nodes++;
        score_t score = -scout_search(board, search_depth, -beta, time_cutoff, allow_cutoff, true, child, options);
        // If our search at lower depth did raise alpha, and this is an All node, re-search at full depth before failing
        // high.
        if ((node == ALLNODE) && (score > alpha) && (search_depth < depth - 1)) {
            score = -scout_search(board, depth - 1, -beta, time_cutoff, allow_cutoff, true, child, options);
        }

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
            Cache::killer_table.store(board.ply(), best_move);
            Cache::history_table.store(depth, best_move);
            break;
        }
    }
    best_score = std::min(best_score, score_ub);
    const Bounds bound = best_score <= alpha ? UPPER : best_score >= beta ? LOWER : EXACT;
    Cache::transposition_table.store(hash, best_score, bound, depth, best_move, board.ply());
    return best_score;
}

score_t Search::pv_search(Board &board, const depth_t start_depth, const score_t alpha_start, const score_t beta,
                          PrincipleLine &line, unsigned int time_cutoff, const bool allow_cutoff,
                          SearchOptions &options) {
    // Perform an alpha-beta pruning tree search.
    // The use of this function implies that the node is a PV-node.
    // For non-PV nodes, use scout_search.
    // Has bounds [alpha, beta]

    score_t alpha = alpha_start;
    depth_t depth = start_depth;

    // Check extentions
    if (board.is_check()) {
        depth++;
    }
    const zobrist_t hash = board.hash();
    Cache::transposition_table.prefetch(hash);

    MoveList legal_moves = board.get_moves();
    // Terminal node
    if (legal_moves.empty()) {
        return Evaluation::terminal(board);
    }

    // Probe the tablebase for the winning move at root.
    if (options.tbenable && board.is_root()) {
        if (Tablebase::probe_root(board, legal_moves)) {
            assert(!legal_moves.empty());
            options.tbhits++;
            // Only move in legal_moves will be the best move from the tablebase. Its score is set to the eval.
            line.push_back(legal_moves.front());
            return legal_moves.front().score;
        }
    }

    // If this is a draw by repetition, 50 moves, or insufficient material, return the drawn score.
    if (board.is_draw() && !board.is_root()) {
        return Evaluation::drawn_score(board);
    }

    // The absolute upper bound for a score on this node is ply_to_mate_score(board.ply()).
    if (ply_to_mate_score(board.ply()) <= alpha) {
        // Alpha is a mate at a lower ply than this node, we can't do better.
        // Fail low.
        return ply_to_mate_score(board.ply());
    }

    // The absolute lower bound for a score on this node is -ply_to_mate_score(board.ply()).
    // (That is, we are being mated on this node)
    if (-ply_to_mate_score(board.ply()) >= beta) {
        // Beta is us being mated at a lower ply than this node, we can't do worse.
        // Fail high.
        return -ply_to_mate_score(board.ply());
    }

    // Max ply
    if (board.ply() >= MAX_PLY) {
        return Evaluation::eval(board);
    }

    // Leaf node for the main tree.
    if (depth == 0) {
        return quiesce(board, alpha, beta, options);
    }

    // Lookup position in transposition table for hashmove.
    DenseMove hash_dmove = NULL_DMOVE;
    Cache::TransElement tthit;
    if (Cache::transposition_table.probe(hash, tthit)) {
        hash_dmove = tthit.move();
    }

    // Check if we've passed our time cutoff
    if (allow_cutoff && (options.nodes % (1<<10) == 0)) {
        if (options.get_millis() > time_cutoff) {
            options.set_stop();
            return MAX_SCORE;
        }
    }

    // The TB can bound our score < TBLOSS. At the end, the best score should be compared to this, and the lower
    // taken.
    score_t score_ub = MAX_SCORE;
    score_t best_score = MIN_SCORE;
    // Probe the tablebase for WDL
    if (options.tbenable && !board.is_root()) {
        score_t tbresult;
        Bounds bounds;
        if (Tablebase::probe_wdl(board, tbresult, bounds)) {
            options.tbhits++;
            if (bounds == UPPER) {
                // TB result is an upper bound (i.e. TBLOSS)
                if (tbresult <= alpha) {
                    Cache::transposition_table.store(hash, tbresult, bounds, MAX_DEPTH, NULL_MOVE, board.ply());
                    return tbresult;
                } else {
                    score_ub = tbresult;
                }
            } else if (bounds == LOWER) {
                // TB Result is a lower bound
                if (tbresult >= beta) {
                    Cache::transposition_table.store(hash, tbresult, bounds, MAX_DEPTH, NULL_MOVE, board.ply());
                    return tbresult;
                } else {
                    best_score = tbresult;
                    alpha = std::max(alpha, tbresult);
                }
            } else {
                // The TB score is exact.
                Cache::transposition_table.store(hash, tbresult, bounds, MAX_DEPTH, NULL_MOVE, board.ply());
                return tbresult;
            }
        }
    }

    bool is_first_child = true;
    PrincipleLine pv;
    Move hash_move = unpack_move(hash_dmove, legal_moves);

    // Do the hash move explicitly to avoid sorting moves if our hash moves provides a beta-cutoff
    if (hash_move != NULL_MOVE) {
        board.make_move(hash_move);
        options.nodes++;
        score_t score = -pv_search(board, depth - 1, -beta, -alpha, pv, time_cutoff, allow_cutoff, options);
        board.unmake_move(hash_move);
        pv.push_back(hash_move);

        if (options.stop()) {
            return MAX_SCORE;
        }
        best_score = std::max(best_score, score);
        alpha = std::max(alpha, score);
        if (alpha >= beta) {
            line = pv;
            Cache::killer_table.store(board.ply(), pv.back());
            Cache::history_table.store(depth, pv.back());
            best_score = std::min(best_score, score_ub);
            Cache::transposition_table.store(hash, best_score, LOWER, depth, pv.back(), board.ply());
            return best_score;
        }
        is_first_child = false;
    }
    // Sort the remaining moves, and remove the hash move if it exists
    Ordering::rank_and_sort_moves(board, legal_moves, hash_dmove);

    for (Move move : legal_moves) {
        // We've already dealt with the hashmove.
        if (move == hash_move) {
            continue;
        }
        PrincipleLine temp_line;
        temp_line.reserve(16);

        board.make_move(move);
        options.nodes++;
        score_t score;
        if (is_first_child) {
            score = -pv_search(board, depth - 1, -beta, -alpha, temp_line, time_cutoff, allow_cutoff, options);
        } else {
            // Search with a null window
            score = -scout_search(board, depth - 1, -alpha - 1, time_cutoff, allow_cutoff, true, CUTNODE, options);
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
            Cache::killer_table.store(board.ply(), pv.back());
            Cache::history_table.store(depth, move);
            break; // beta-cutoff
        }
        is_first_child = false;
    }
    line = pv;
    if (!pv.empty()) {
        best_score = std::min(best_score, score_ub);
        const Bounds bound = best_score <= alpha_start ? UPPER : best_score >= beta ? LOWER : EXACT;
        Cache::transposition_table.store(hash, best_score, bound, depth, pv.back(), board.ply());
    }
    return best_score;
}

score_t Search::quiesce(Board &board, const score_t alpha_start, const score_t beta, SearchOptions &options) {
    // perform quiesence search to evaluate only quiet positions.
    score_t alpha = alpha_start;

    MoveList moves;

    // Look for checkmate
    if (board.is_check()) {
        // Generates all evasions.
        moves = board.get_moves();
        if (moves.empty()) {
            return Evaluation::terminal(board);
        }
    }

    // If this is a draw by repetition or insufficient material, return the drawn score.
    if (board.is_draw()) {
        return Evaluation::drawn_score(board);
    }

    const score_t stand_pat = Evaluation::eval(board);

    alpha = std::max(alpha, stand_pat);

    // Beta cutoff, but don't allow stand-pat in check.
    if (!board.is_check() && stand_pat >= beta) {
        return stand_pat;
    }

    score_t delta = 900;
    // If pawns are on seventh we could be promoting, delta is higher.
    if (board.pieces(board.who_to_play(), PAWN) & Bitboards::rank(relative_rank(board.who_to_play(), RANK7))) {
        delta += 500;
    }
    // Delta pruning
    if (stand_pat + delta <= alpha) {
        return stand_pat;
    }

    // Get a list of moves for quiessence. If it's check, it we already have all evasions from the checkmate test.
    // Not in check, we generate quiet checks and all captures.
    if (!board.is_check()) {
        moves = board.get_capture_moves();
    }

    // We already know it's not mate, if there are no captures in a position, return stand pat.
    if (moves.empty()) {
        return stand_pat;
    }

    // Sort the captures and record SEE.
    Ordering::rank_and_sort_moves(board, moves, NULL_DMOVE);

    for (Move move : moves) {
        // For a capture, the recorded score is the SEE value.
        // It makes sense to not consider losing captures in qsearch.
        if (!board.is_check() && move.is_capture() && !SEE::see(board, move, 0)) {
            continue;
        }
        constexpr score_t see_margin = 100;
        // In qsearch, only consider moves with a decent chance of raising alpha.
        if (!board.is_check() && move.is_capture() && !SEE::see(board, move, alpha - stand_pat - see_margin)) {
            continue;
        }
        board.make_move(move);
        options.nodes++;
        const score_t score = -quiesce(board, -beta, -alpha, options);
        board.unmake_move(move);
        alpha = std::max(alpha, score);
        if (alpha >= beta) {
            break; // beta-cutoff
        }
    }
    return alpha;
}

score_t Search::search(Board &board, const depth_t max_depth, int soft_cutoff, const int hard_cutoff,
                       PrincipleLine &line, SearchOptions &options) {
    // Initialise the transposition table.
    Cache::transposition_table.set_delete();
    Cache::history_table.clear();
    PrincipleLine principle;
    board.set_root();

    const ply_t mate_in_ply = 2 * options.mate_depth;

    score_t score = 0;

    // Initialise variables for time control.
    options.set_origin();
    // Reference time vs search start
    int millis_now, millis_last, millis_next;
    // Estimate effective branching factor empirically
    double branching_factor = 2.5;
    bool allow_cutoff = false;
    options.tbhits = 0;
    options.nodes = 0;

    Move last_best_move = NULL_MOVE;
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
            temp_line.clear();
            new_score = pv_search(board, depth, alpha, beta, temp_line, hard_cutoff, allow_cutoff, options);

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
        millis_now = 1 + options.get_millis();
        const unsigned long nps = ((1000 *options.nodes) / millis_now);

        // Send the info for the search to uci
        UCI::uci_info(depth, score, options.nodes, options.tbhits, nps, principle, millis_now, board.get_root());

        // Estimate the next time span.
        millis_next = branching_factor * millis_now;

        // In simple positions, reduce our thinking time.
        if (principle.back() == last_best_move) {
            soft_cutoff -= soft_cutoff / 10;
        } else {
            last_best_move = principle.back();
        }
        // Check if our estimate of the next depth will take us over our time limit.
        if (millis_next > hard_cutoff) {
            break;
        }
        // Check if we've passed our soft cutoff.
        if (millis_now > soft_cutoff) {
            break;
        }

        // Calculate the last effective branching factor
        if (depth >= 5) {
            constexpr double weight = .5;
            branching_factor = (1 - weight) * branching_factor + weight * (millis_now / millis_last);
        }

        // Break if reached mate in N depth.
        if (is_mating(score)) {
            if (mate_score_to_ply(score) <= mate_in_ply) {
                break;
            }
        }

        millis_last = millis_now;
    }
    line = principle;
    return score;
}

score_t Search::search(Board &board, const depth_t depth, PrincipleLine &line) {
    SearchOptions options = SearchOptions();
    return search(board, depth, POS_INF, POS_INF, line, options);
}

void Search::init() {
    // Initialise the reductions table.
    for (depth_t depth = 0; depth < MAX_DEPTH; depth++) {
        reductions_table[0][depth][0] = 0;
        reductions_table[1][depth][0] = 0;
        for (int move_count = 1; move_count < MAX_MOVES; move_count++) {
            // Quiet moves
            reductions_table[0][depth][move_count] =
                static_cast<depth_t>(std::log(depth) * std::log(move_count) * 0.4 + 1);

            // Captures
            reductions_table[1][depth][move_count] =
                static_cast<depth_t>(std::log(depth) * std::log(move_count) * 0.25);
        }
    }
}
