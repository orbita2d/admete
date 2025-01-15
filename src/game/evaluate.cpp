#include "evaluate.hpp"
#include "board.hpp"
#include "cache.hpp"
#include "printing.hpp"
#include "zobrist.hpp"
#include <array>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <algorithm>

// clang-format off


constexpr per_square<score_t> lightsquare_corner_distance = {
    0, 1, 2, 3, 4, 5, 6, 7,
    1, 2, 3, 4, 5, 6, 7, 6,
    2, 3, 4, 5, 6, 7, 6, 5,
    3, 4, 5, 6, 7, 6, 5, 4,
    4, 5, 6, 7, 6, 5, 4, 3,
    5, 6, 7, 6, 5, 4, 3, 2,
    6, 7, 6, 5, 2, 3, 2, 1,
    7, 6, 5, 4, 3, 2, 1, 0
    };

constexpr per_square<score_t> darksquare_corner_distance = {
    7, 6, 5, 4, 3, 2, 1, 0,
    6, 7, 6, 5, 4, 3, 2, 1,
    5, 6, 7, 6, 5, 4, 3, 2,
    4, 5, 6, 7, 6, 5, 4, 3,
    3, 4, 5, 6, 7, 6, 5, 4,
    2, 3, 4, 5, 6, 7, 6, 5,
    1, 2, 3, 4, 5, 6, 7, 6,
    0, 1, 2, 3, 4, 5, 6, 7
    };

// clang-format on

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

void init() {
    constants();
}

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


// Pawn structure bonuses.
Score eval_pawns(const Board &board) {
    zobrist_t hash = Zobrist::pawns(board);
    Score score;
    GameCache::PawnCacheElem hit;
    if (GameCache::pawn_cache.probe(hash, hit)) {
        return hit.eval();
    }

    const Bitboard white_pawns = board.pieces(WHITE, PAWN);
    const Bitboard black_pawns = board.pieces(BLACK, PAWN);

    // Add bonus for passed pawns for each side.
    Bitboard occ = board.passed_pawns(WHITE);
    while (occ) {
        Square sq = pop_lsb(&occ);
        score += pb_passed[sq.reverse()];
    }

    occ = board.passed_pawns(BLACK);
    while (occ) {
        Square sq = pop_lsb(&occ);
        score -= pb_passed[sq];
    }

    // Bonus for connected passers.
    occ = board.connected_passed_pawns(WHITE);
    score += connected_passed * count_bits(occ);

    occ = board.connected_passed_pawns(BLACK);
    score -= connected_passed * count_bits(occ);

    // Bonus for defended passers.
    occ = board.passed_pawns(WHITE) && board.pawn_controlled(WHITE);
    score += defended_passed * count_bits(occ);

    occ = board.passed_pawns(BLACK) && board.pawn_controlled(BLACK);
    score -= defended_passed * count_bits(occ);

    // Penalty for weak pawns.
    occ = board.weak_pawns(WHITE);
    score += weak_pawn * count_bits(occ);

    occ = board.weak_pawns(BLACK);
    score -= weak_pawn * count_bits(occ);

    // Penalty for isolated pawns.
    occ = board.isolated_pawns(WHITE);
    score += isolated_pawn * count_bits(occ);

    occ = board.isolated_pawns(BLACK);
    score -= isolated_pawn * count_bits(occ);

    // Penalty for doubled pawns
    occ = Bitboards::forward_span<WHITE>(white_pawns) & white_pawns;
    score += doubled_pawns * count_bits(occ);

    occ = Bitboards::forward_span<BLACK>(black_pawns) & black_pawns;
    score -= doubled_pawns * count_bits(occ);

    GameCache::pawn_cache.store(hash, score);
    return score;
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
    auto mapped = std::clamp(nn * 400, -2000.f, 2000.f);
    return static_cast<score_t>(mapped);
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
        return contempt;
    } else {
        return -contempt;
    }
}
