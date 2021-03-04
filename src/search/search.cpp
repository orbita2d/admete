#include <time.h>
#include <chrono>
#include "search.hpp"
#include <iostream>

int alphabeta(Board& board, const uint depth, PrincipleLine& line) {
    return alphabeta(board, depth, NEG_INF, POS_INF, line);
}


int alphabeta(Board& board, const uint depth, int alpha, int beta, PrincipleLine& line) {
    // perform alpha-beta pruning search.
    std::vector<Move> legal_moves = board.get_sorted_moves();
    if (depth == 0) { return board.evaluate_negamax(legal_moves); }
    if (legal_moves.size() == 0) { 
        return board.evaluate_negamax(legal_moves); 
    }
    PrincipleLine best_line;
    int best_score = NEG_INF;
    for (Move move : legal_moves) {
        PrincipleLine temp_line;
        board.make_move(move);
        int score = -alphabeta(board, depth - 1, -beta, -alpha, temp_line);
        board.unmake_move(move);
        if (score > best_score) {
            best_score = score;
            best_line = temp_line;
            best_line.push_back(move);
        }
        if (score == mating_score - depth) {
            // Mate in 1.
            break;
        }
        alpha = std::max(alpha, score);
        if (alpha >= beta) {
            break; // beta-cutoff
        }
    }
    line = best_line;
    return is_mating(best_score) ? best_score - 1 : best_score;
}

int pv_search(Board& board, const uint depth, int alpha, int beta, PrincipleLine& principle, const uint pv_depth, PrincipleLine& line) {
    // perform alpha-beta pruning search with principle variation optimisation.
    std::vector<Move> legal_moves = board.get_sorted_moves();
    if (depth == 0) { return board.evaluate_negamax(legal_moves); }
    if (legal_moves.size() == 0) { 
        return board.evaluate_negamax(legal_moves); 
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
    best_line.push_back(pv_move);
    board.unmake_move(pv_move);

    for (Move move : legal_moves) {
        if (move == pv_move) {
            continue;
        }
        PrincipleLine temp_line;
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
    return is_mating(best_score) ? best_score - 1 : best_score;
}


int iterative_deepening(Board& board, const uint max_depth, const int max_millis, PrincipleLine& line) {
    PrincipleLine principle;
    int score = alphabeta(board, 2, NEG_INF, POS_INF, principle);
    // We want to limit our search to a fixed time.
    std::chrono::high_resolution_clock::time_point time_origin, time_now;
    time_origin = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> time_span;
    // Start at 2 ply for a best guess first move.
    for (int depth = 4; depth <= max_depth; depth+=2) {
        PrincipleLine temp_line;
        temp_line.clear();
        score = pv_search(board, depth, NEG_INF, POS_INF, principle, principle.size(), temp_line);
        principle = temp_line;
        if (is_mating(score)) { break; }
        time_now = std::chrono::high_resolution_clock::now();
        time_span = time_now - time_origin;
        // We've run out of time to calculate.
        std::cerr << "depth: " << depth << std::endl;
        std::cerr << int(time_span.count()) << "millis " << std::endl;
        if (int(time_span.count()) > max_millis) { break;}
    }
    line = principle;
    return score;
}

int iterative_deepening(Board& board, const uint depth, PrincipleLine& line) {
    return iterative_deepening(board, depth, POS_INF, line);
}

int find_best_random(Board& board, const uint max_depth, const int random_weight, const int max_millis, PrincipleLine& line) {
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

    for (int depth = 4; depth <= max_depth; depth +=2) {
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
        std::cerr << "depth: " << depth << std::endl;
        std::cerr << time_span.count() << "millis " << std::endl;
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
