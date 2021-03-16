#include <time.h>
#include <chrono>
#include "search.hpp"
#include "evaluate.hpp"
#include "transposition.hpp"
#include <iostream>

int alphabeta(Board& board, const unsigned int depth, PrincipleLine& line) {
    return alphabeta(board, depth, NEG_INF, POS_INF, line);
}

int alphabeta(Board& board, const unsigned int depth, const int alpha_start, const int beta_start, PrincipleLine& line) {
    // perform alpha-beta pruning search.
    int alpha = alpha_start;
    int beta = beta_start;
    std::vector<Move> legal_moves = board.get_sorted_moves();
    if (legal_moves.size() == 0) { 
        return evaluate(board, legal_moves); 
    }
    if (depth == 0) { return quiesce(board, alpha, beta); }
    
    const long hash = board.hash();
    if (transposition_table.probe(hash)) {
        const TransElement hit = transposition_table.last_hit();
        if (hit.depth() >= depth) {
            if (hit.lower()) {
                // The saved score is a lower bound for the score of the sub tree
                if (hit.eval() >= beta) {
                    // beta cutoff
                    return hit.eval();
                }
            } else if (hit.upper()) {
                // The saved score is an upper bound for the score of the subtree.
                if (hit.eval() <= alpha) {
                    // rare negamax alpha-cutoff
                    return hit.eval();
                }
            } else {
                // The saved score is an exact value for the subtree
                return hit.eval();
            }
        }
    }
    
    PrincipleLine best_line;
    int best_score = NEG_INF;
    for (Move move : legal_moves) {
        PrincipleLine temp_line;
        temp_line.reserve(16);
        board.make_move(move);
        int score = -alphabeta(board, depth - 1, -beta, -alpha, temp_line);
        board.unmake_move(move);
        if (score > best_score) {
            best_score = score;
            best_line = temp_line;
            best_line.push_back(move);
        }
        if (score == mating_score) {
            // Mate in 1.
            break;
        }
        alpha = std::max(alpha, score);
        if (alpha >= beta) {
            break; // beta-cutoff
        }
    }
    line = best_line;
    transposition_table.store(hash, best_score, alpha_start, beta, depth);
    return best_score;
}

int quiesce(Board& board, int alpha, int beta) {
    // perform quiesence search to evaluate only quiet positions.
    int stand_pat = negamax_heuristic(board);
    if (stand_pat >= beta) {
        return stand_pat;
    }
    // Delta pruning
    constexpr int DELTA = 900;
    if (stand_pat < alpha - DELTA) {
        return alpha;
    }
    if (alpha < stand_pat) {
        alpha = stand_pat;
    }
    
    std::vector<Move> captures = board.get_captures();
    for (Move move : captures) {
        board.make_move(move);
        int score = -quiesce(board, -beta, -alpha);
        board.unmake_move(move);
        alpha = std::max(alpha, score);
        if (alpha >= beta) {
            break; // beta-cutoff
        }
    }
    return alpha;
}

int pv_search(Board& board, const unsigned int depth, const int alpha_start, int beta, PrincipleLine& principle, const uint pv_depth, PrincipleLine& line) {
    // perform alpha-beta pruning search with principle variation optimisation.
    int alpha = alpha_start;
    std::vector<Move> legal_moves = board.get_sorted_moves();
    if (depth == 0) { return evaluate(board, legal_moves); }
    if (legal_moves.size() == 0) { 
        return evaluate(board, legal_moves); 
    }
    if (pv_depth == 0) {
        // End of the principle variation, just evaluate this node using alphabeta()
        return alphabeta(board, depth, alpha, beta, line);
    }
    PrincipleLine best_line;
    // -1 here is because we are indexing at 0. If there is 1 move left, that's at index 0;
    Move pv_move = principle.at(pv_depth - 1);
    board.make_move(pv_move);
    int best_score = -pv_search(board, depth - 1, -beta, -alpha, principle, pv_depth - 1, best_line);
    alpha = best_score;
    best_line.push_back(pv_move);
    board.unmake_move(pv_move);

    for (Move move : legal_moves) {
        if (move == pv_move) {
            continue;
        }
        PrincipleLine temp_line;
        temp_line.reserve(16);
        board.make_move(move);
        int score = -alphabeta(board, depth - 1, -beta, -alpha, temp_line);
        board.unmake_move(move);
        if (score > best_score) {
            best_score = score;
            best_line = temp_line;
            best_line.push_back(move);
        }
        if (score == mating_score) {
            // Mate in 1.
            break;
        }
        alpha = std::max(alpha, score);
        if (alpha >= beta) {
            break; // beta-cutoff
        }
    }
    line = best_line;
    transposition_table.store(board.hash(), best_score, alpha_start, beta, depth);
    return best_score;
}

int iterative_deepening(Board& board, const unsigned int max_depth, const int max_millis, PrincipleLine& line) {
    // Initialise the transposition table.
    transposition_table.clear();
    transposition_table.min_depth(0);
    PrincipleLine principle;
    // We want to limit our search to a fixed time.
    std::chrono::high_resolution_clock::time_point time_origin, time_now;
    time_origin = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> time_span, time_span_last;
    int branching_factor = 40;
    int score = alphabeta(board, 2, NEG_INF, POS_INF, principle);
    time_now = std::chrono::high_resolution_clock::now();
    time_span = time_now - time_origin;
    std::chrono::duration<double, std::milli> t_est = branching_factor * time_span;
    time_span_last = time_span;
    // Start at 2 ply for a best guess first move.
    for (unsigned int depth = 4; depth <= max_depth; depth+=1) {
        PrincipleLine temp_line;
        temp_line.reserve(depth);
        score = pv_search(board, depth, NEG_INF, POS_INF, principle, principle.size(), temp_line);
        principle = temp_line;
        if (is_mating(score)) { break; }
        time_now = std::chrono::high_resolution_clock::now();
        time_span = time_now - time_origin;
        t_est = branching_factor * time_span;
        // We've run out of time to calculate.
        if (int(t_est.count()) > max_millis) { break;}
        // Calculate the last branching factor
        if (depth >= 6) {
            branching_factor = int(time_span.count() / time_span_last.count());
        }
        time_span_last = time_span;
    }
    line = principle;
    if (is_mating(score)) { score -= (line.size() + 1) / 2; }
    if (is_mating(-score)) { score += (line.size()) / 2; }
    return score;
}

int iterative_deepening(Board& board, const unsigned int depth, PrincipleLine& line) {
    return iterative_deepening(board, depth, POS_INF, line);
}

int find_best_random(Board& board, const unsigned int max_depth, const int random_weight, const int max_millis, PrincipleLine& line) {
    std::vector<Move> legal_moves = board.get_sorted_moves();
    int n = legal_moves.size();
    std::vector<PrincipleLine> line_array(legal_moves.size());
    std::vector<int> score_array(legal_moves.size());
    std::fill(score_array.begin(), score_array.end(), NEG_INF);

    for ( int i = 0; i < n ; i++) {
        PrincipleLine principle;
        Move move = legal_moves[i];
        board.make_move(move);
        int score = -alphabeta(board, 1, NEG_INF, POS_INF, principle);
        board.unmake_move(move);
        score_array[i] = score;
        line_array[i] = principle;
    }
    // We want to limit our search to a fixed time.
    std::chrono::high_resolution_clock::time_point time_origin, time_now;
    time_origin = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> time_span;

    for (unsigned int depth = 4; depth <= max_depth; depth +=2) {
        bool break_flag = false;
        for ( int i = 0; i < n ; i++) {
            PrincipleLine temp_line;
            temp_line.clear();
            PrincipleLine principle = line_array.at(i);
            Move move = legal_moves.at(i);
            board.make_move(move);
            int score = -pv_search(board, depth - 1, NEG_INF, POS_INF, principle, principle.size(), temp_line);
            board.unmake_move(move);
            principle = temp_line;
            if (is_mating(score)) { score--; }
            score_array[i] = score;
            line_array[i] = principle;
            if (is_mating(score)) { break_flag = true; break; }
            time_now = std::chrono::high_resolution_clock::now();
            time_span = time_now - time_origin;
            if (int(time_span.count()) > max_millis) { break;}
        }
        if (break_flag) { break;}
        time_now = std::chrono::high_resolution_clock::now();
        time_span = time_now - time_origin;
        // We've run out of time to calculate.
        if (int(time_span.count()) > max_millis) { break;}
    }
    std::srand(time(NULL));
    int best_score = NEG_INF;
    PrincipleLine best_line;
    for ( int i = 0; i< n ; i++) {
        int score = score_array[i];
        int offset = is_mating(score) ? 0 : ((std::rand() % (2*random_weight)) - random_weight);
        if (score + offset > best_score) {
            best_line = line_array[i];
            best_line.push_back(legal_moves[i]);
            best_score = score + offset;
        }
    }
    line = best_line;
    return best_score;
}
