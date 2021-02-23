#include "search.hpp"

int minimax(Board &board, const uint depth) {
    int max = INT32_MIN;
    std::vector<Move> legal_moves = board.get_moves();
    if (depth == 0) { return board.evaluate_negamax(legal_moves); }
    if (legal_moves.size() == 0) { return board.evaluate_negamax(legal_moves); }
    for (Move move : legal_moves) {
        board.make_move(move);
        int score = -minimax(board, depth - 1);
        board.unmake_move(move);
        max = std::max(max, score);
    }
    return max;
}

int minimax(Board& board, const uint depth, PrincipleLine& line) {
    std::vector<Move> legal_moves = board.get_moves();
    int max = INT32_MIN;
    PrincipleLine best_line;
    if (depth == 0) { return board.evaluate_negamax(legal_moves); }
    if (legal_moves.size() == 0) { return board.evaluate_negamax(legal_moves); }
    for (Move move : legal_moves) {
        PrincipleLine temp_line;
        board.make_move(move);
        int score = -minimax(board, depth - 1, temp_line);
        board.unmake_move(move);
        if (score > max) {
            max = is_mating(score) ? score - 1 : score; 
            temp_line.push_back(move);
            best_line = temp_line;
            // If we've found a mate, just break the search.
            if (is_mating(score)) {break;}
        }
    }
    line = best_line;
    return max;
}

int alphabeta(Board& board, const uint depth, PrincipleLine& line) {
    return alphabeta(board, depth, INT32_MIN, INT32_MAX, board.is_white_move(), line);
}


int alphabeta(Board& board, const uint depth, int alpha, int beta, const bool maximising) {
    // perform alpha-beta pruning search.
    std::vector<Move> legal_moves = board.get_moves();
    if (depth == 0) { return board.evaluate(legal_moves, maximising); }
    if (legal_moves.size() == 0) { return board.evaluate(legal_moves, maximising); }
    if (maximising) {
        int best_score = INT32_MIN;
        for (Move move : legal_moves) {
            board.make_move(move);
            int score = alphabeta(board, depth - 1, alpha, beta, false);
            board.unmake_move(move);
            best_score = std::max(best_score, score);
            if (score == mating_score) {
                // Mate in 1.
                break;
            }
            alpha = std::max(alpha, score);
            if (alpha > beta) {
                break; // beta-cutoff
            }
        }
        
        return is_mating(best_score) ? best_score - 1 : best_score;
    } else {
        int best_score = INT32_MAX;
        for (Move move : legal_moves) {
            board.make_move(move);
            int score = alphabeta(board, depth - 1, alpha, beta, true);
            board.unmake_move(move);
            best_score = std::min(best_score, score);
            if (score == mating_score) {
                // Mate in 1.
                break;
            }
            beta = std::min(beta, score);
            if (beta <= alpha) {
                break; // alpha-cutoff
            }
        }
        return is_mating(-best_score) ? best_score + 1 : best_score;
    }
}

int alphabeta(Board& board, const uint depth, int alpha, int beta, const bool maximising, PrincipleLine& line) {
    // perform alpha-beta pruning search.
    std::vector<Move> legal_moves = board.get_sorted_moves();
    if (depth == 0) { return board.evaluate(legal_moves, maximising); }
    if (legal_moves.size() == 0) { 
        return board.evaluate(legal_moves, maximising); 
    }
    PrincipleLine best_line;
    if (maximising) {
        int best_score = INT32_MIN;
        for (Move move : legal_moves) {
            PrincipleLine temp_line;
            board.make_move(move);
            int score = alphabeta(board, depth - 1, alpha, beta, false, temp_line);
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
            if (alpha > beta) {
                break; // beta-cutoff
            }
        }
        line = best_line;
        return is_mating(best_score) ? best_score - 1 : best_score;
    } else {
        int best_score = INT32_MAX;
        for (Move move : legal_moves) {
            PrincipleLine temp_line;
            board.make_move(move);
            int score = alphabeta(board, depth - 1, alpha, beta, true, temp_line);
            board.unmake_move(move);
            if (score < best_score) {
                best_score = score;
                best_line = temp_line;
                best_line.push_back(move);
            }
            if (score == mating_score) {
                // Mate in 1.
                break;
            }
            beta = std::min(beta, score);
            if (beta <= alpha) {
                break; // alpha-cutoff
            }
        }
        line = best_line;
        return is_mating(-best_score) ? best_score + 1 : best_score;
    }
}