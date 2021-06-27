#include <time.h>
#include <chrono>
#include "search.hpp"
#include "uci.hpp"
#include "../game/evaluate.hpp"
#include "transposition.hpp"
#include <iostream>

score_t null_window_search(Board& board, const depth_t depth, const score_t alpha, long &nodes,
 my_clock::time_point time_cutoff, bool &kill_flag, bool allow_cutoff) {
    // perform alpha-beta pruning search.
    score_t beta = alpha + 1;
    
    MoveList legal_moves = board.get_moves();
    if (board.is_draw()) { return 0; }
    if (legal_moves.size() == 0) { 
        nodes++;
        return evaluate(board, legal_moves); 
    }
    if (depth == 0) { return quiesce(board, alpha, beta, nodes); }

    // Lookup position in transposition table.
    DenseMove hash_move = NULL_DMOVE;
    KillerTableRow killer_move = Cache::killer_table.probe(board.ply());
    const long hash = board.hash();
    if (Cache::transposition_table.probe(hash)) {
        const Cache::TransElement hit = Cache::transposition_table.last_hit();
        if (hit.depth() >= depth) {
            if (hit.lower()) {
                // The saved score is a lower bound for the score of the sub tree
                if (hit.eval(board.ply()) >= beta) {
                    // beta cutoff
                    return hit.eval(board.ply());
                }
            } else if (hit.upper()) {
                // The saved score is an upper bound for the score of the subtree.
                if (hit.eval(board.ply()) <= alpha) {
                    // rare negamax alpha-cutoff
                    return hit.eval(board.ply());
                }
            } else {
                // The saved score is an exact value for the subtree
                return hit.eval(board.ply());
            }
        }
        hash_move = hit.move();
    }
    
    if (allow_cutoff && (nodes % 1000 == 0)) {
        // Check if we've passed our time cutoff
        my_clock::time_point time_now = my_clock::now();
        if (time_now > time_cutoff) {
            kill_flag = true;
            return MAX_SCORE;
        }
    }

    board.sort_moves(legal_moves, hash_move, killer_move);
    score_t best_score = MIN_SCORE;
    Move best_move;
    for (Move move : legal_moves) {
        PrincipleLine temp_line;
        temp_line.reserve(16);
        board.make_move(move);
        score_t score = -null_window_search(board, depth - 1, -alpha -1, nodes, time_cutoff, kill_flag, allow_cutoff);
        board.unmake_move(move);
        if (kill_flag) { return MAX_SCORE; }
        if (score > best_score) {
            best_score = score;
            best_move = move;
        }
        if (best_score == MATING_SCORE) {
            // Mate in 1.
            break;
        }
        if (best_score >= beta) {
            break; // beta-cutoff
        }
    }

    Cache::killer_table.store(board.ply(), best_move);
    Cache::transposition_table.store(hash, best_score, alpha, beta, depth, best_move, board.ply());
    return best_score;
}


score_t alphabeta(Board& board, const depth_t depth, const score_t alpha_start, const score_t beta, PrincipleLine& line, long &nodes,
 my_clock::time_point time_cutoff, bool &kill_flag, bool allow_cutoff) {
    // perform alpha-beta pruning search.
    score_t alpha = alpha_start;
    
    MoveList legal_moves = board.get_moves();
    if (board.is_draw()) { return 0; }
    if (legal_moves.size() == 0) { 
        nodes++;
        return evaluate(board, legal_moves); 
    }
    if (depth == 0) { return quiesce(board, alpha, beta, nodes); }

    // Lookup position in transposition table.
    DenseMove hash_move = NULL_DMOVE;
    KillerTableRow killer_move = Cache::killer_table.probe(board.ply());
    const long hash = board.hash();
    if (Cache::transposition_table.probe(hash)) {
        const Cache::TransElement hit = Cache::transposition_table.last_hit();
        hash_move = hit.move();
    }
    
    if (allow_cutoff && (nodes % 1000 == 0)) {
        // Check if we've passed our time cutoff
        my_clock::time_point time_now = my_clock::now();
        if (time_now > time_cutoff) {
            kill_flag = true;
            return MAX_SCORE;
        }
    }
    board.sort_moves(legal_moves, hash_move, killer_move);
    bool is_first_child = true;
    PrincipleLine best_line;
    score_t best_score = MIN_SCORE;
    for (Move move : legal_moves) {
        PrincipleLine temp_line;
        temp_line.reserve(16);
        board.make_move(move);
        score_t score;
        if (is_first_child) {
            score = -alphabeta(board, depth - 1, -beta, -alpha, temp_line, nodes, time_cutoff, kill_flag, allow_cutoff);
        } else {
            // Search with a null window
            score = -null_window_search(board, depth - 1, -alpha -1, nodes, time_cutoff, kill_flag, allow_cutoff);
            if (score > alpha && score < beta && depth > 1) {
                // Do a full search
                score = -alphabeta(board, depth - 1, -beta, -alpha, temp_line, nodes, time_cutoff, kill_flag, allow_cutoff);
            }
        }
        board.unmake_move(move);
        if (kill_flag) { return MAX_SCORE; }
        if (score > best_score) {
            best_score = score;
            best_line = temp_line;
            best_line.push_back(move);
        }
        if (score == MATING_SCORE) {
            // Mate in 1.
            break;
        }
        alpha = std::max(alpha, score);
        if (alpha >= beta) {
            break; // beta-cutoff
        }
        is_first_child = false;
    }
    line = best_line;

    Cache::killer_table.store(board.ply(), best_line.back());
    Cache::transposition_table.store(hash, best_score, alpha_start, beta, depth, best_line.back(), board.ply());
    return best_score;
}

score_t quiesce(Board& board, const score_t alpha_start, const score_t beta, long &nodes) {
    // perform quiesence search to evaluate only quiet positions.
    score_t alpha = alpha_start;
    int stand_pat = negamax_heuristic(board);
    if (stand_pat >= beta) {
        nodes++;
        return stand_pat;
    }
    if (alpha < stand_pat) {
        alpha = stand_pat;
    }
    if (board.is_draw()) { return 0; }
    // Delta_pruning
    if (stand_pat < alpha - 900) {
        return stand_pat;
    }
    MoveList captures = board.get_captures();
    board.sort_moves(captures, NULL_DMOVE, NULL_KROW);
    for (Move move : captures) {
        board.make_move(move);
        score_t score = -quiesce(board, -beta, -alpha, nodes);
        board.unmake_move(move);
        alpha = std::max(alpha, score);
        if (alpha >= beta) {
            break; // beta-cutoff
        }
    }
    return alpha;
}

score_t pv_search(Board& board, const depth_t depth, const score_t alpha_start, const score_t beta, PrincipleLine& principle, const uint pv_depth,
 PrincipleLine& line, long& nodes, my_clock::time_point time_cutoff, bool &kill_flag, bool allow_cutoff) {
    // perform alpha-beta pruning search with principle variation optimisation.
    score_t alpha = alpha_start;
    MoveList legal_moves = board.get_moves();
    if (legal_moves.size() == 0) { 
        nodes++;
        return evaluate(board, legal_moves); 
    }
    if (depth == 0) { nodes++; return evaluate(board, legal_moves); }
    if (board.is_draw()) { return 0; }

    PrincipleLine best_line;
    score_t best_score = MIN_SCORE;
    Move pv_move = NULL_MOVE;
    if (pv_depth > 0) {
        // -1 here is because we are indexing at 0. If there is 1 move left, that's at index 0;
        pv_move = principle.at(pv_depth - 1);
        board.make_move(pv_move);
        best_score = -pv_search(board, depth - 1, -beta, -alpha, principle, pv_depth - 1, best_line, nodes, time_cutoff, kill_flag, allow_cutoff);
        alpha = best_score;
        best_line.push_back(pv_move);
        board.unmake_move(pv_move);
    } else {
        return alphabeta(board, depth, alpha, beta, line, nodes, time_cutoff, kill_flag, allow_cutoff);
    }
    if (kill_flag) { return MAX_SCORE; }
    board.sort_moves(legal_moves, NULL_DMOVE, NULL_KROW);
    for (Move move : legal_moves) {
        if (move == pv_move) {
            continue;
        }
        PrincipleLine temp_line;
        temp_line.reserve(16);
        board.make_move(move);

        // Search with a null window
        score_t score = -null_window_search(board, depth - 1, -alpha -1, nodes, time_cutoff, kill_flag, allow_cutoff);
        if (score > alpha && score < beta && depth > 1) {
            // Do a full search
            temp_line.clear();
            temp_line.reserve(16);
            score = -alphabeta(board, depth - 1, -beta, -alpha, temp_line, nodes, time_cutoff, kill_flag, allow_cutoff);
        }
        board.unmake_move(move);
        if (kill_flag) { return MAX_SCORE; }
        if (score > best_score) {
            best_score = score;
            best_line = temp_line;
            best_line.push_back(move);
        }
        if (score == MATING_SCORE) {
            // Mate in 1.
            break;
        }
        alpha = std::max(alpha, score);
        if (alpha >= beta) {
            break; // beta-cutoff
        }
    }
    line = best_line;
    Cache::transposition_table.store(board.hash(), best_score, alpha_start, beta, depth, best_line.back(), board.ply());
    return best_score;
}

score_t iterative_deepening(Board& board, const depth_t max_depth, const int max_millis, PrincipleLine& line, long &nodes) {
    // Initialise the transposition table.
    Cache::transposition_table.set_delete();
    //Cache::transposition_table.clear();

    // Check for a principle line in the TT
    PrincipleLine principle = unroll_tt_line(board);
    board.set_root();
    
    int score, new_score;

    // We want to limit our search to a fixed time.
    my_clock::time_point time_origin, time_now, time_cutoff;
    time_origin = my_clock::now();
    time_cutoff = time_origin + std::chrono::milliseconds((int) (1.2 * max_millis));
    std::chrono::duration<double, std::milli> time_span, time_span_last, t_est;
    int branching_factor = 10;
    int counter = 0;

    uint start_depth = 2;
    bool kill_flag = false;
    bool allow_cutoff = false;

    for (depth_t depth = start_depth; depth <= max_depth; depth+=1) {
        PrincipleLine temp_line;
        temp_line.reserve(depth);
        nodes = 0ul;
        new_score = pv_search(board, depth, MIN_SCORE, MAX_SCORE, principle, principle.size(), temp_line, nodes, time_cutoff, kill_flag, allow_cutoff);
        allow_cutoff = true;
        if (kill_flag) { break; }
        // Use this to not save an invalid score if we reach the time cutoff
        score = new_score;
        principle = unroll_tt_line(board, temp_line);
        time_now = my_clock::now();
        time_span = time_now - time_origin;

        unsigned long nps = int(1000*(nodes / time_span.count()));
        uci_info(depth, score, nodes, nps, principle, (int) time_span.count());

        t_est = branching_factor * time_span;
        // Calculate the last branching factor
        if (counter >= 3) {
            branching_factor = int(time_span.count() / time_span_last.count());
        }
        // We've run out of time to calculate.
        if (int(t_est.count()) > max_millis) { break;}
        time_span_last = time_span;
        counter++;
    }
    line = principle;
    return score;
}

score_t iterative_deepening(Board& board, const depth_t depth, PrincipleLine& line) {
    long nodes = 0;
    return iterative_deepening(board, depth, POS_INF, line, nodes);
}


PrincipleLine unroll_tt_line(Board& board) {
    PrincipleLine reverse_line, line;
    reverse_line.reserve(32);
    // We need to be very carful not to get stuck in a loop.
    // Record starting ply of line.
    ply_t start = board.ply();
    while (true) {
        long hash = board.hash();
        if (board.repetitions(start) > 0) {
            // If we have seen this position before in the unrolling, we're in a loop, so break.
            break;
        }
        if (Cache::transposition_table.probe(hash)) {
            MoveList legal_moves = board.get_moves();
            Cache::TransElement hit = Cache::transposition_table.last_hit(); 
            Move best_move = unpack_move(hit.move(), board);
            if (best_move == NULL_MOVE) {
                break;
            }
            board.make_move(best_move);
            reverse_line.push_back(best_move);
        } else { 
            break; }
    }
    line.reserve(reverse_line.size());
    for (PrincipleLine::reverse_iterator it = reverse_line.rbegin(); it != reverse_line.rend(); ++it) {
        board.unmake_move(*it);
        line.push_back(*it);
    }
    return line;
}

PrincipleLine unroll_tt_line(Board& board, PrincipleLine principle) {
    PrincipleLine line, extenstion;
    for (PrincipleLine::reverse_iterator it = principle.rbegin(); it != principle.rend(); ++it) {
        std::vector<Move> legal_moves = board.get_moves();
        board.make_move(*it);
    }
    extenstion = unroll_tt_line(board);
    line = extenstion;
    line.insert(line.end(), principle.begin(), principle.end());
    for (Move move : principle) {
        std::vector<Move> legal_moves = board.get_moves();
        board.unmake_move(move);
    }
    return line;
}