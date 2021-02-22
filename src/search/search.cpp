#include "search.hpp"

bool is_mating(int score) {
    return (score > (mating_score-500));
}

int minimax(Board &board, const uint depth) {
    int max = INT32_MIN;
    std::vector<Move> legal_moves = board.get_moves();
    if (depth == 0) { return board.evaluate(legal_moves); }
    if (legal_moves.size() == 0) { return board.evaluate(legal_moves); }
    for (Move move : legal_moves) {
        board.make_move(move);
        int score = -minimax(board, depth - 1);
        board.unmake_move(move);
        max = std::max(max, score);
    }
    return max;
}


int find_best(Board& board, const uint depth, PrincipleLine& line) {
    std::vector<Move> legal_moves = board.get_moves();
    int max = INT32_MIN;
    PrincipleLine best_line;
    if (depth == 0) { return board.evaluate(legal_moves); }
    if (legal_moves.size() == 0) { return board.evaluate(legal_moves); }
    for (Move move : legal_moves) {
        PrincipleLine temp_line;
        board.make_move(move);
        int score = -find_best(board, depth - 1, temp_line);
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