#include "evaluate.hpp"
#include "../game/piece.hpp"
#include "../game/board.hpp"
#include <array>

// Want some consideration of positional play
typedef std::array<int, 64> position_board;
typedef std::array<position_board, 2> position_board_pair;

std::array<std::array<int, 6>, 2> material = {
    {
        {100, 300, 300, 500, 900, 0},
        {-100, -300, -300, -500, -900, -0}
    }
    };

position_board fill_blank_positional_scores() {
    // want centralised bishops.
    position_board pb;
    pb.fill(0);
    return pb;
}

inline int reverse_rank(int square) {
    // Square from our perspective;
    return (56 - (square & 0x38)) | (square & 0x07);
}

position_board reverse_board(position_board in) {
    position_board pb;
    for (int s = 0; s<64; s++) {
        pb[s] = in[reverse_rank(s)];
    }
    return pb;
}

constexpr position_board center_dist = {
            3, 3, 3, 3, 3, 3, 3, 3,
            3, 2, 2, 2, 2, 2, 2, 3,
            3, 2, 1, 1, 1, 1, 2, 3,
            3, 2, 1, 0, 0, 1, 2, 3,
            3, 2, 1, 0, 0, 1, 2, 3,
            3, 2, 1, 1, 1, 1, 2, 3,
            3, 2, 2, 2, 2, 2, 2, 3,
            3, 3, 3, 3, 3, 3, 3, 3
};

position_board fill_knight_positional_scores() {
    // want centralised knights.
    const int weight = 5;
    position_board pb;
    for( int i = 0; i<64 ; i++) {
        pb[i] = (3-center_dist[i]) * weight;
    }
    return pb;
}
position_board pb_knight = fill_knight_positional_scores();


constexpr position_board pb_bishop = {    5,   0,   0,   0,   0,   0,   0,   5,
                                          0,  10,   5,   5,   5,   5,  10,   0,
                                          0,   0,   5,  10,  10,   5,   0,   0,
                                          0,   0,  10,  10,  10,  10,   0,   0,
                                          0,   0,  10,  10,  10,  10,   0,   0,
                                          0,   0,   5,  10,  10,   5,   0,   0,
                                          5,  30,   0,   0,   0,   0, 330,   0, 
                                          0,   0, -10,   0,   0, -10,   0,  10};

constexpr position_board pb_king = {      0,  0,  0,  0,  0,  0,  0,  0,
                                          0,  0,  0,  0,  0,  0,  0,  0,
                                          0,  0,  0,  0,  0,  0,  0,  0,
                                          0,  0,  0,  0,  0,  0,  0,  0,
                                          0,  0,  0,  0,  0,  0,  0,  0,
                                          0,  0,  0,  0,  0,  0,  0,  0,
                                          0,  0,  0,  0,  0,  0,  0,  0, 
                                          10, 30, 20,  4,  4,  6, 30, 10 };


constexpr position_board pb_pawn = {      0,   0,   0,   0,   0,   0,   0,   0,
                                        160, 160, 160, 160, 160, 160, 160, 160,
                                         20,  20,  20,  20,  20,  20,  20,  20,
                                          8,   8,   8,   8,   8,   8,   8,   8,
                                          5,   5,   5,  20,  20,   5,   5,   5,
                                          3,   3,   3,   3,   3,   3,   3,   3,
                                          0,   0,   0,   0,   0,   0,   0,   0, 
                                          0,   0,   0,   0,   0,   0,   0,   0 };


constexpr position_board pb_queen = {   0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0, 
                                        0, 0, 0, 0, 0, 0, 0, 0 };

constexpr position_board pb_rook = {   0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0, 
                                        0, 0, 0, 0, 0, 0, 0, 0 };

std::array<position_board_pair, 6> fill_positional_scores() {
    std::array<position_board_pair, 6> scores;

    scores[Pieces::Pawn - 1][WHITE] = pb_pawn;
    scores[Pieces::Knight - 1][WHITE] = fill_knight_positional_scores();
    scores[Pieces::Bishop - 1][WHITE] = pb_bishop;
    scores[Pieces::Rook - 1][WHITE] = pb_pawn;
    scores[Pieces::Queen - 1][WHITE] = pb_pawn;
    scores[Pieces::King - 1][WHITE] = pb_king;

    scores[Pieces::Pawn - 1][BLACK] = reverse_board(pb_pawn);
    scores[Pieces::Knight - 1][BLACK] = reverse_board(fill_knight_positional_scores());
    scores[Pieces::Bishop - 1][BLACK] = reverse_board(pb_bishop);
    scores[Pieces::Rook - 1][BLACK] = reverse_board(pb_pawn);
    scores[Pieces::Queen - 1][BLACK] = reverse_board(pb_pawn);
    scores[Pieces::King - 1][BLACK] = reverse_board(pb_king);

    return scores;
}
std::array<position_board_pair, 6> PositionScores = fill_positional_scores();

int evaluate(Board &board) {
    std::vector<Move> legal_moves = board.get_moves();
    return evaluate(board, legal_moves);
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
        Piece piece = board.pieces(i);
        value += material[to_enum_colour(piece)][piece.get_piece() - 1];
        value += PositionScores[to_enum_colour(piece)][piece.get_piece() - 1][i];
    }
    if (board.can_castle(WHITE)) {
        value+=10;
    } 
    if (board.can_castle(BLACK)) {
        value-=10;
    } 
    return value * side_multiplier;
}

int evaluation_diff(Board &board, Move move) {
    int diff = 0;
    if (move.is_king_castle() | move.is_queen_castle()) {

    } else {
        Piece piece = board.pieces(move.origin);
    }
    diff -= 0;
}