#include "evaluate.hpp"
#include "board.hpp"
#include "printing.hpp"
#include "zobrist.hpp"
#include <array>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <algorithm>


// Material here is for determining the game phase.
static per_piece<score_t> phase_material = {{
    100, // Pawn
    300, // Knight
    350, // Bishop
    500, // Rook
    900, // Queen,
    0    // King
}};

namespace Evaluation {

static Neural::network_t network = Neural::get_network();

sqt_t reverse_board(sqt_t in) {
    sqt_t pb;
    for (int s = 0; s < 64; s++) {
        pb[s] = in[56 ^ s];
    }
    return pb;
}

score_t piece_phase_material(const PieceType p) {
    assert(p != NO_PIECE);
    return phase_material[p];
}

} // namespace Evaluation

score_t Evaluation::count_phase_material(const Board &board) {
    score_t material_value = 0;
    for (PieceType p = PAWN; p < N_PIECE; p++) {
        material_value += phase_material[p] * (board.count_pieces(WHITE, p) + board.count_pieces(BLACK, p));
    }
    return material_value;
}


score_t Evaluation::eval(const Board &board) {
    // Return the eval from the point of view of the current player.
    Neural::nn_t nn = network.forward(board.accumulator(), board.who_to_play());
    // TODO: Why think about centipawns at all, ideally we'd just map the output to the score_t range.
    auto mapped = std::clamp(static_cast<score_t>(nn*400.), 1-MIN_MATE_SCORE, MIN_MATE_SCORE-1); 
    return mapped;
}

score_t Evaluation::evaluate_white(const Board &board) {
    auto val = Evaluation::eval(board);
    return board.is_white_move() ? val : -val;
}

// The eval for a terminal node.
score_t Evaluation::terminal(const Board &board) {
    if (board.is_check()) {
        // This is checkmate
        return -ply_to_mate_score(board.ply());
    } else {
        // This is stalemate.
        return drawn_score(board);
    }
}

// Get the true static evaluation for any node, does the check if the node is a terminal node.
score_t Evaluation::evaluate_safe(const Board &board) {
    std::vector<Move> legal_moves = board.get_moves();
    if (legal_moves.empty()) {
        return terminal(board);
    } else {
        return eval(board);
    }
}

// Returns a relative score for what we should consider a draw.
// This implements contempt, such that the engine player should try avoid draws.
score_t Evaluation::drawn_score(const Board &board) {
    // If we are an even number of nodes from root, the root player is the current player.
    const bool root_player = (board.height() % 2) == 0;
    if (root_player) {
        return -contempt;
    } else {
        return contempt;
    }
}
