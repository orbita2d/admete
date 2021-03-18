#include "evaluate.hpp"
#include "piece.hpp"
#include "board.hpp"
#include <array>
#include <iostream>
#include <iomanip>

// Want some consideration of positional play
typedef std::array<int, 64> position_board;
typedef std::array<position_board, 6> position_board_set;

enum GamePhase {
    OPENING,
    ENDGAME
};
constexpr int OPENING_MATERIAL = 7800;
constexpr int ENDGAME_MATERIAL = 0;
std::array<int, 6> material = {
        {100, 300, 300, 500, 900, 0}
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

constexpr position_board pb_king_opening = {   -20, -20, -20, -20, -20, -20, -20, -20,
                                               -20, -20, -20, -20, -20, -20, -20, -20,
                                               -20, -20, -20, -20, -20, -20, -20, -20,
                                               -20, -20, -20, -20, -20, -20, -20, -20,
                                               -20, -20, -20, -20, -20, -20, -20, -20,
                                               -20, -20, -20, -20, -20, -20, -20, -20,
                                               -10, -20, -20, -20, -20, -20, -20, -20, 
                                                10,  50,  30,   0,   0,   6,  50,  10 };


constexpr position_board pb_king_endgame = {    0,  0,  0,  0,  0,  0,  0,  0,
                                                0, 10, 10, 10, 10, 10, 10,  0,
                                                0, 10, 20, 20, 20, 20, 10,  0,
                                                0, 10, 20, 20, 20, 20, 10,  0,
                                                0, 10, 20, 20, 20, 20, 10,  0,
                                                0, 10, 20, 20, 20, 20, 10,  0,
                                                0, 10, 10, 10, 10, 10, 10,  0, 
                                                0,  0,  0,  0,  0,  0,  0,  0 };


constexpr position_board pb_pawn_opening = {      0,   0,   0,   0,   0,   0,   0,   0,
                                                260, 260, 260, 260, 260, 260, 260, 260,
                                                120, 120, 120, 120, 120, 120, 120, 120,
                                                108, 108, 108, 108, 108, 108, 108, 108,
                                                100, 105, 105, 130, 130, 105, 100, 100,
                                                100, 100, 100, 100, 100, 100, 100, 100,
                                                100, 100, 100, 100, 100, 100, 100, 100, 
                                                  0,   0,   0,   0,   0,   0,   0,   0 };

constexpr position_board pb_pawn_endgame = {      0,   0,   0,   0,   0,   0,   0,   0,
                                                400, 400, 400, 400, 400, 400, 400, 400,
                                                300, 300, 300, 300, 300, 300, 300, 300,
                                                280, 280, 280, 280, 280, 280, 280, 280,
                                                260, 260, 260, 260, 260, 260, 260, 260,
                                                240, 240, 240, 240, 240, 240, 240, 240,
                                                200, 200, 200, 200, 200, 200, 200, 200, 
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

std::array<std::array<position_board_set, 2>, 2> fill_positional_scores() {
    std::array<std::array<position_board_set, 2>, 2> scores;

    scores[OPENING][WHITE][PAWN] = pb_pawn_opening;
    scores[OPENING][WHITE][KNIGHT] = fill_knight_positional_scores();
    scores[OPENING][WHITE][BISHOP] = pb_bishop;
    scores[OPENING][WHITE][ROOK] = pb_rook;
    scores[OPENING][WHITE][QUEEN] = pb_queen;
    scores[OPENING][WHITE][KING] = pb_king_opening;

    scores[OPENING][BLACK][PAWN] = reverse_board(pb_pawn_opening);
    scores[OPENING][BLACK][KNIGHT] = reverse_board(fill_knight_positional_scores());
    scores[OPENING][BLACK][BISHOP] = reverse_board(pb_bishop);
    scores[OPENING][BLACK][ROOK] = reverse_board(pb_rook);
    scores[OPENING][BLACK][QUEEN] = reverse_board(pb_queen);
    scores[OPENING][BLACK][KING] = reverse_board(pb_king_opening);


    scores[ENDGAME][WHITE][PAWN] = pb_pawn_endgame;
    scores[ENDGAME][WHITE][KNIGHT] = fill_knight_positional_scores();
    scores[ENDGAME][WHITE][BISHOP] = pb_bishop;
    scores[ENDGAME][WHITE][ROOK] = pb_rook;
    scores[ENDGAME][WHITE][QUEEN] = pb_queen;
    scores[ENDGAME][WHITE][KING] = pb_king_endgame;

    scores[ENDGAME][BLACK][PAWN] = reverse_board(pb_pawn_endgame);
    scores[ENDGAME][BLACK][KNIGHT] = reverse_board(fill_knight_positional_scores());
    scores[ENDGAME][BLACK][BISHOP] = reverse_board(pb_bishop);
    scores[ENDGAME][BLACK][ROOK] = reverse_board(pb_rook);
    scores[ENDGAME][BLACK][QUEEN] = reverse_board(pb_queen);
    scores[ENDGAME][BLACK][KING] = reverse_board(pb_king_endgame);

    return scores;
}

std::array<std::array<position_board_set, 2>, 2> PositionScores = fill_positional_scores();

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
    std::cout << "MATERIAL" << std::endl;
    for (int i = 0; i<6; i++) {
        std::cout << std::setfill(' ') << std::setw(4) << material[i] << " ";
    }
    std::cout << std::endl;
    std::cout << "OPENING" << std::endl;
    std::cout << "PAWN" << std::endl;
    print_table(PositionScores[OPENING][WHITE][PAWN]);
    print_table(PositionScores[OPENING][BLACK][PAWN]);
    std::cout << "KNIGHT" << std::endl;
    print_table(PositionScores[OPENING][WHITE][KNIGHT]);
    print_table(PositionScores[OPENING][BLACK][KNIGHT]);
    std::cout << "BISHOP" << std::endl;
    print_table(PositionScores[OPENING][WHITE][BISHOP]);
    print_table(PositionScores[OPENING][BLACK][BISHOP]);
    std::cout << "ROOK" << std::endl;
    print_table(PositionScores[OPENING][WHITE][ROOK]);
    print_table(PositionScores[OPENING][BLACK][ROOK]);
    std::cout << "QUEEN" << std::endl;
    print_table(PositionScores[OPENING][WHITE][QUEEN]);
    print_table(PositionScores[OPENING][BLACK][QUEEN]);
    std::cout << "KING" << std::endl;
    print_table(PositionScores[OPENING][WHITE][KING]);
    print_table(PositionScores[OPENING][BLACK][KING]);

    std::cout << "ENDGAME" << std::endl;
    std::cout << "PAWN" << std::endl;
    print_table(PositionScores[ENDGAME][WHITE][PAWN]);
    print_table(PositionScores[ENDGAME][BLACK][PAWN]);
    std::cout << "KNIGHT" << std::endl;
    print_table(PositionScores[ENDGAME][WHITE][KNIGHT]);
    print_table(PositionScores[ENDGAME][BLACK][KNIGHT]);
    std::cout << "BISHOP" << std::endl;
    print_table(PositionScores[ENDGAME][WHITE][BISHOP]);
    print_table(PositionScores[ENDGAME][BLACK][BISHOP]);
    std::cout << "ROOK" << std::endl;
    print_table(PositionScores[ENDGAME][WHITE][ROOK]);
    print_table(PositionScores[ENDGAME][BLACK][ROOK]);
    std::cout << "QUEEN" << std::endl;
    print_table(PositionScores[ENDGAME][WHITE][QUEEN]);
    print_table(PositionScores[ENDGAME][BLACK][QUEEN]);
    std::cout << "KING" << std::endl;
    print_table(PositionScores[ENDGAME][WHITE][KING]);
    print_table(PositionScores[ENDGAME][BLACK][KING]);
}

int evaluate(Board &board) {
    std::vector<Move> legal_moves = board.get_moves();
    return evaluate(board, legal_moves);
}

int heuristic_diff(Colour us, Move &move, int material_value) {
    int opening_value = 0;
    int endgame_value = 0;
    Colour them = ~us;
    PieceEnum p = move.moving_peice;
    // Remove the piece from the origin
    opening_value -= PositionScores[OPENING][us][p][move.origin];
    endgame_value -= PositionScores[ENDGAME][us][p][move.origin];
    // Add the piece from the origin
    opening_value += PositionScores[OPENING][us][p][move.target];
    endgame_value += PositionScores[ENDGAME][us][p][move.target];

    if (move.is_promotion()) {
        PieceEnum promoted = get_promoted(move);
        opening_value += PositionScores[OPENING][us][promoted][move.target];
        endgame_value += PositionScores[ENDGAME][us][promoted][move.target];
    } 
    if (move.is_king_castle()) {
        opening_value -= PositionScores[OPENING][us][ROOK][RookSquare[us][KINGSIDE]];
        endgame_value -= PositionScores[ENDGAME][us][ROOK][RookSquare[us][KINGSIDE]];
        opening_value += PositionScores[OPENING][us][ROOK][move.origin + Direction::E];
        endgame_value += PositionScores[ENDGAME][us][ROOK][move.origin + Direction::E];
    } else if (move.is_queen_castle()) {
        opening_value -= PositionScores[OPENING][us][ROOK][RookSquare[us][QUEENSIDE]];
        endgame_value -= PositionScores[ENDGAME][us][ROOK][RookSquare[us][QUEENSIDE]];
        opening_value += PositionScores[OPENING][us][ROOK][move.origin + Direction::W];
        endgame_value += PositionScores[ENDGAME][us][ROOK][move.origin + Direction::W];
    } else if (move.is_ep_capture()) {
        const Square captured_square = move.origin.rank()|move.target.file();
        opening_value -= PositionScores[OPENING][them][PAWN][captured_square];
        endgame_value -= PositionScores[ENDGAME][them][PAWN][captured_square];
    } else if (move.is_capture()) {
        const PieceEnum cp = to_enum_piece(move.captured_peice);
        opening_value -= PositionScores[OPENING][them][cp][move.target];
        endgame_value -= PositionScores[ENDGAME][them][cp][move.target];
    }
    
    int value = opening_value + (material_value - OPENING_MATERIAL) * (endgame_value - opening_value) / (ENDGAME_MATERIAL - OPENING_MATERIAL);
    return value;
}

int count_material(Board &board) {
    int material_value = 0;
    for (int p = 0; p < N_PIECE; p++) {
        Bitboard occ = board.pieces((PieceEnum)p);
        material_value += material[p] * count_bits(occ);
    }
    return material_value;
}

int heuristic(Board &board) {
    int material_value = 0;
    int opening_value = 0;
    int endgame_value = 0;
    for (int p = 0; p < N_PIECE; p++) {
        Bitboard occ = board.pieces(WHITE, (PieceEnum)p);
        while (occ) {
            material_value += material[p];
            Square sq = pop_lsb(&occ);
            opening_value += PositionScores[OPENING][WHITE][p][sq];
            endgame_value += PositionScores[ENDGAME][WHITE][p][sq];
        }
        occ = board.pieces(BLACK, (PieceEnum)p);
        while (occ) {
            material_value += material[p];
            Square sq = pop_lsb(&occ);
            opening_value += PositionScores[OPENING][BLACK][p][sq];
            endgame_value += PositionScores[ENDGAME][BLACK][p][sq];
        }
    }
    // Interpolate linearly between the game phases.
    int value = opening_value + (material_value - OPENING_MATERIAL) * (endgame_value - opening_value) / (ENDGAME_MATERIAL - OPENING_MATERIAL);
    return value;
}

int negamax_heuristic(Board &board) {
    int side_multiplier = board.is_white_move() ? 1 : -1;
    return heuristic(board) * side_multiplier;
}

int evaluate(Board &board, std::vector<Move> &legal_moves) {
    // evaluate the position relative to the current player.
    // First check if we have been mated.
    if (legal_moves.size() == 0) {
        if (board.is_check()) {
            // This is checkmate
            return -mating_score;
        } else {
            // This is stalemate.
            return -10;
        }
    }
    return negamax_heuristic(board);
}