#include "evaluate.hpp"
#include <array>

// Want some consideration of positional play
typedef std::array<int, 64> positon_board;

positon_board fill_blank_positional_scores() {
    // want centralised bishops.
    positon_board pb;
    pb.fill(0);
    return pb;
}

constexpr positon_board center_dist = {
            3, 3, 3, 3, 3, 3, 3, 3,
            3, 2, 2, 2, 2, 2, 2, 3,
            3, 2, 1, 1, 1, 1, 2, 3,
            3, 2, 1, 0, 0, 1, 2, 3,
            3, 2, 1, 0, 0, 1, 2, 3,
            3, 2, 1, 1, 1, 1, 2, 3,
            3, 2, 2, 2, 2, 2, 2, 3,
            3, 3, 3, 3, 3, 3, 3, 3
};

positon_board fill_knight_positional_scores() {
    // want centralised knights.
    const float weight = 5;
    positon_board pb;
    for( int i = 0; i<64 ; i++) {
        pb[i] = (3-center_dist[i]) * weight;
    }
    return pb;
}

positon_board fill_bishop_positional_scores() {
    // want centralised bishops.
    const float weight = 1;
    positon_board pb = {  5,  0,  0, 0,  0,   0,  0,  5,
                          0, 10,  5, 5,  5,   5,  10,  0,
                          0,  0, 20, 10, 10, 20,  0,  0,
                          0,  0, 10, 10, 10, 10,  0,  0,
                          0,  0, 10, 10, 10, 10,  0,  0,
                          0,  0, 20, 10, 10, 20,  0,  0,
                          5, 30,  0,  0,  0,  0, 30,  0, 
                         10,  0,-10,  0,  0,-10,  0, 10};
    return pb;
}

positon_board fill_king_positional_scores() {
    // want castled kings.
    const float weight = 1;
    positon_board pb = {  0,  0,  0,  0,  0,  0,  0,  0,
                          0,  0,  0,  0,  0,  0,  0,  0,
                          0,  0,  0,  0,  0,  0,  0,  0,
                          0,  0,  0,  0,  0,  0,  0,  0,
                          0,  0,  0,  0,  0,  0,  0,  0,
                          0,  0,  0,  0,  0,  0,  0,  0,
                          0,  0,  0,  0,  0,  0,  0,  0, 
                          10, 30, 20, 4,  4,  6, 30, 10 };
    return pb;
}

positon_board fill_pawn_positional_scores() {
    positon_board pb = {    0,    0,    0,    0,    0,    0,    0,     0,
                          160,  160,  160,  160,  160,  160,  160,   160,
                           20,   20,   20,   20,   20,   20,   20,    20,
                           8,   8,   8,   8,   8,   8,   8,    8,
                           5,   5,   5,   10,   10,   5,   5,    5,
                           3,   3,   3,   3,   3,   3,   3,    3,
                            0,    0,    0,    0,    0,    0,    0,     0, 
                            0,    0,    0,    0,    0,    0,    0,     0 };
    return pb;
}

std::array<positon_board, 6> fill_positional_scores() {
    std::array<positon_board, 6> scores;
    scores.fill(fill_blank_positional_scores());
    scores[Pieces::Knight - 1] = fill_knight_positional_scores();
    scores[Pieces::Bishop - 1] = fill_bishop_positional_scores();
    scores[Pieces::King - 1] = fill_king_positional_scores();
    scores[Pieces::Pawn - 1] = fill_pawn_positional_scores();
    return scores;
}
std::array<positon_board, 6> PositionScores = fill_positional_scores();

int evaluate(Board &board) {
    std::vector<Move> legal_moves = board.get_moves();
    return evaluate(board, legal_moves);
}

inline int reverse_rank(int square) {
    // Square from our perspective;
    return (56 - (square & 0x38)) | (square & 0x07);
}

int evaluate(Board &board, std::vector<Move> &legal_moves) {
    // evaluate the position relative to the current player.
    int side_multiplier = board.is_white_move() ? 1 : -1;
    int value = 0;
    // First check if we have been mated.
    if (legal_moves.size() == 0) {
        if (board.aux_info.is_check) {
            // This is checkmate
            return -mating_score;
        } else {
            // This is stalemate.
            return -10;
        }
    }
    for (uint i = 0; i < 64; i++) {
        if(board.is_free(i)) {continue;}
        Piece piece = board.pieces[i];
        value += material(piece);
        if (piece.is_white()) {
            value += PositionScores[piece.get_piece() - 1][i];
        } else {
            value -= PositionScores[piece.get_piece() - 1][reverse_rank(i)];

        }
    }
    if (board.can_castle(WHITE)) {
        value+=10;
    } 
    if (board.can_castle(BLACK)) {
        value-=10;
    } 
    return value * side_multiplier;
}
