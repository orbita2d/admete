#include "evaluate.hpp"
#include "piece.hpp"
#include "board.hpp"
#include <array>
#include <iostream>
#include <iomanip>

// Want some consideration of positional play
typedef std::array<int, 64> position_board;
typedef std::array<position_board, 6> position_board_set;

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
        pb[s] = -in[reverse_rank(s)];
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
    const int weight = 8;
    const int material = 300;
    position_board pb;
    for( int i = 0; i<64 ; i++) {
        pb[i] = (3-center_dist[i]) * weight + material;
    }
    return pb;
}
position_board pb_knight = fill_knight_positional_scores();


constexpr position_board pb_bishop = {  305, 300, 300, 300, 300, 300, 300, 305,
                                        300, 310, 305, 305, 305, 305, 310, 300,
                                        300, 300, 305, 310, 310, 305, 300, 300,
                                        300, 300, 310, 310, 310, 310, 300, 300,
                                        300, 300, 310, 310, 310, 310, 300, 300,
                                        300, 300, 305, 310, 310, 305, 300, 300,
                                        305, 330, 300, 300, 300, 300, 330, 300, 
                                        300, 300, 290, 300, 300, 290, 300, 310};

constexpr position_board pb_king = {      0,  0,  0,  0,  0,  0,  0,  0,
                                          0,  0,  0,  0,  0,  0,  0,  0,
                                          0,  0,  0,  0,  0,  0,  0,  0,
                                          0,  0,  0,  0,  0,  0,  0,  0,
                                          0,  0,  0,  0,  0,  0,  0,  0,
                                          0,  0,  0,  0,  0,  0,  0,  0,
                                          0,  0,  0,  0,  0,  0,  0,  0, 
                                         10, 30, 20,  4,  4,  6, 30, 10 };


constexpr position_board pb_pawn = {      0,   0,   0,   0,   0,   0,   0,   0,
                                        260, 160, 160, 160, 160, 160, 160, 160,
                                        120, 120, 120, 120, 120, 120, 120, 120,
                                        108, 108, 108, 108, 108, 108, 108, 108,
                                        105, 105, 105, 120, 120, 105, 105, 105,
                                        103, 103, 103, 103, 103, 103, 103, 103,
                                        100, 100, 100, 100, 100, 100, 100, 100, 
                                          0,   0,   0,   0,   0,   0,   0,   0 };


constexpr position_board pb_queen = {   900, 900, 900, 900, 900, 900, 900, 900,
                                        900, 900, 900, 900, 900, 900, 900, 900,
                                        900, 900, 900, 900, 900, 900, 900, 900,
                                        900, 900, 900, 900, 900, 900, 900, 900,
                                        900, 900, 900, 900, 900, 900, 900, 900,
                                        900, 900, 900, 900, 900, 900, 900, 900,
                                        900, 900, 900, 900, 900, 900, 900, 900, 
                                        900, 900, 900, 900, 900, 900, 900, 900 };

constexpr position_board pb_rook = {    500, 500, 500, 500, 500, 500, 500, 500,
                                        500, 500, 500, 500, 500, 500, 500, 500,
                                        500, 500, 500, 500, 500, 500, 500, 500,
                                        500, 500, 500, 500, 500, 500, 500, 500,
                                        500, 500, 500, 500, 500, 500, 500, 500,
                                        500, 500, 500, 500, 500, 500, 500, 500,
                                        500, 500, 500, 500, 500, 500, 500, 500, 
                                        500, 500, 500, 500, 500, 500, 500, 500 };

std::array<position_board_set, 2> fill_positional_scores() {
    std::array<position_board_set, 2> scores;

    scores[WHITE][PAWN] = pb_pawn;
    scores[WHITE][KNIGHT] = fill_knight_positional_scores();
    scores[WHITE][BISHOP] = pb_bishop;
    scores[WHITE][ROOK] = pb_rook;
    scores[WHITE][QUEEN] = pb_queen;
    scores[WHITE][KING] = pb_king;

    scores[BLACK][PAWN] = reverse_board(pb_pawn);
    scores[BLACK][KNIGHT] = reverse_board(fill_knight_positional_scores());
    scores[BLACK][BISHOP] = reverse_board(pb_bishop);
    scores[BLACK][ROOK] = reverse_board(pb_rook);
    scores[BLACK][QUEEN] = reverse_board(pb_queen);
    scores[BLACK][KING] = reverse_board(pb_king);

    return scores;
}
std::array<position_board_set, 2> PositionScores = fill_positional_scores();

void print_table(const position_board table) {
    for (int i = 0; i< 64; i++) {
        std::cout << std::setfill(' ') << std::setw(4) << table[i] << " ";
        if (i % 8 == 7) {
            std::cout << std::endl;
        }
    }
    std::cout << std::endl;
}

void print_tables() {
    std::cout << "PAWN" << std::endl;
    print_table(PositionScores[WHITE][PAWN]);
    print_table(PositionScores[BLACK][PAWN]);
    std::cout << "KNIGHT" << std::endl;
    print_table(PositionScores[WHITE][KNIGHT]);
    print_table(PositionScores[BLACK][KNIGHT]);
    std::cout << "BISHOP" << std::endl;
    print_table(PositionScores[WHITE][BISHOP]);
    print_table(PositionScores[BLACK][BISHOP]);
    std::cout << "ROOK" << std::endl;
    print_table(PositionScores[WHITE][ROOK]);
    print_table(PositionScores[BLACK][ROOK]);
    std::cout << "QUEEN" << std::endl;
    print_table(PositionScores[WHITE][QUEEN]);
    print_table(PositionScores[BLACK][QUEEN]);
    std::cout << "KING" << std::endl;
    print_table(PositionScores[WHITE][KING]);
    print_table(PositionScores[BLACK][KING]);
}

int evaluate(Board &board) {
    std::vector<Move> legal_moves = board.get_moves();
    return evaluate(board, legal_moves);
}

int heuristic(Board &board) {
    int side_multiplier = board.is_white_move() ? 1 : -1;
    int value = 0;
    for (uint i = 0; i < 64; i++) {
        if(board.is_free(i)) {continue;}
        Piece piece = board.pieces(i);
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
    return heuristic(board);
}