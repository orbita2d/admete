#include "evaluate.hpp"
#include "piece.hpp"
#include "board.hpp"
#include <array>
#include <iostream>
#include <iomanip>
#include <fstream>

// Want some consideration of positional play

enum GamePhase {
    OPENING,
    ENDGAME
};
constexpr int OPENING_MATERIAL = 7800;
constexpr int ENDGAME_MATERIAL = 0;
std::array<int, 6> material = {
        {100, 300, 300, 500, 900, 0}
};

int piece_value(const PieceType p) {
    return material[p];
}
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
                                                300, 300, 300, 300, 300, 300, 300, 300,
                                                200, 200, 200, 200, 200, 200, 200, 200,
                                                180, 180, 180, 180, 180, 180, 180, 180,
                                                160, 160, 160, 160, 160, 160, 160, 160,
                                                140, 140, 140, 140, 140, 140, 140, 140,
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


static std::array<std::array<position_board_set, 2>, 2> piece_square_tables;
namespace Evaluation {
    void init() {
        piece_square_tables[OPENING][WHITE][PAWN] = pb_pawn_opening;
        piece_square_tables[OPENING][WHITE][KNIGHT] = fill_knight_positional_scores();
        piece_square_tables[OPENING][WHITE][BISHOP] = pb_bishop;
        piece_square_tables[OPENING][WHITE][ROOK] = pb_rook;
        piece_square_tables[OPENING][WHITE][QUEEN] = pb_queen;
        piece_square_tables[OPENING][WHITE][KING] = pb_king_opening;

        piece_square_tables[OPENING][BLACK][PAWN] = reverse_board(pb_pawn_opening);
        piece_square_tables[OPENING][BLACK][KNIGHT] = reverse_board(fill_knight_positional_scores());
        piece_square_tables[OPENING][BLACK][BISHOP] = reverse_board(pb_bishop);
        piece_square_tables[OPENING][BLACK][ROOK] = reverse_board(pb_rook);
        piece_square_tables[OPENING][BLACK][QUEEN] = reverse_board(pb_queen);
        piece_square_tables[OPENING][BLACK][KING] = reverse_board(pb_king_opening);


        piece_square_tables[ENDGAME][WHITE][PAWN] = pb_pawn_endgame;
        piece_square_tables[ENDGAME][WHITE][KNIGHT] = fill_knight_positional_scores();
        piece_square_tables[ENDGAME][WHITE][BISHOP] = pb_bishop;
        piece_square_tables[ENDGAME][WHITE][ROOK] = pb_rook;
        piece_square_tables[ENDGAME][WHITE][QUEEN] = pb_queen;
        piece_square_tables[ENDGAME][WHITE][KING] = pb_king_endgame;

        piece_square_tables[ENDGAME][BLACK][PAWN] = reverse_board(pb_pawn_endgame);
        piece_square_tables[ENDGAME][BLACK][KNIGHT] = reverse_board(fill_knight_positional_scores());
        piece_square_tables[ENDGAME][BLACK][BISHOP] = reverse_board(pb_bishop);
        piece_square_tables[ENDGAME][BLACK][ROOK] = reverse_board(pb_rook);
        piece_square_tables[ENDGAME][BLACK][QUEEN] = reverse_board(pb_queen);
        piece_square_tables[ENDGAME][BLACK][KING] = reverse_board(pb_king_endgame);
    }
}


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
    print_table(piece_square_tables[OPENING][WHITE][PAWN]);
    print_table(piece_square_tables[OPENING][BLACK][PAWN]);
    std::cout << "KNIGHT" << std::endl;
    print_table(piece_square_tables[OPENING][WHITE][KNIGHT]);
    print_table(piece_square_tables[OPENING][BLACK][KNIGHT]);
    std::cout << "BISHOP" << std::endl;
    print_table(piece_square_tables[OPENING][WHITE][BISHOP]);
    print_table(piece_square_tables[OPENING][BLACK][BISHOP]);
    std::cout << "ROOK" << std::endl;
    print_table(piece_square_tables[OPENING][WHITE][ROOK]);
    print_table(piece_square_tables[OPENING][BLACK][ROOK]);
    std::cout << "QUEEN" << std::endl;
    print_table(piece_square_tables[OPENING][WHITE][QUEEN]);
    print_table(piece_square_tables[OPENING][BLACK][QUEEN]);
    std::cout << "KING" << std::endl;
    print_table(piece_square_tables[OPENING][WHITE][KING]);
    print_table(piece_square_tables[OPENING][BLACK][KING]);

    std::cout << "ENDGAME" << std::endl;
    std::cout << "PAWN" << std::endl;
    print_table(piece_square_tables[ENDGAME][WHITE][PAWN]);
    print_table(piece_square_tables[ENDGAME][BLACK][PAWN]);
    std::cout << "KNIGHT" << std::endl;
    print_table(piece_square_tables[ENDGAME][WHITE][KNIGHT]);
    print_table(piece_square_tables[ENDGAME][BLACK][KNIGHT]);
    std::cout << "BISHOP" << std::endl;
    print_table(piece_square_tables[ENDGAME][WHITE][BISHOP]);
    print_table(piece_square_tables[ENDGAME][BLACK][BISHOP]);
    std::cout << "ROOK" << std::endl;
    print_table(piece_square_tables[ENDGAME][WHITE][ROOK]);
    print_table(piece_square_tables[ENDGAME][BLACK][ROOK]);
    std::cout << "QUEEN" << std::endl;
    print_table(piece_square_tables[ENDGAME][WHITE][QUEEN]);
    print_table(piece_square_tables[ENDGAME][BLACK][QUEEN]);
    std::cout << "KING" << std::endl;
    print_table(piece_square_tables[ENDGAME][WHITE][KING]);
    print_table(piece_square_tables[ENDGAME][BLACK][KING]);
}

void Evaluation::load_tables(std::string filename) {

	std::fstream file;
	file.open(filename, std::ios::in);
	if (!file) {
		std::cout << "No such file: " << filename << std::endl;
		exit(EXIT_FAILURE);
	}
    // Start by reading opening tables.
    for (int i = 8; i < 48; i++) {
        // Pawn Table
        int value;
        file >> value;
        piece_square_tables[OPENING][WHITE][PAWN].at(i) = value;
        piece_square_tables[OPENING][BLACK][PAWN] = reverse_board(piece_square_tables[OPENING][WHITE][PAWN]);
    }
    file >> std::ws;
    for (int p = KNIGHT; p < N_PIECE; p++) {
        for (int sq = 0; sq < 64; sq++) {
            // Other Tables
            int value;
            file >> value;
            piece_square_tables[OPENING][WHITE][p].at(sq) = value;
            piece_square_tables[OPENING][BLACK][p] = reverse_board(piece_square_tables[OPENING][WHITE][p]);
        }
        file >> std::ws;
    }
    // Endgame tables.
    for (int i = 8; i < 48; i++) {
        // Pawn Table
        int value;
        file >> value;
        piece_square_tables[ENDGAME][WHITE][PAWN].at(i) = value;
        piece_square_tables[ENDGAME][BLACK][PAWN] = reverse_board(piece_square_tables[ENDGAME][WHITE][PAWN]);
    }
    file >> std::ws;
    for (int p = KNIGHT; p < N_PIECE; p++) {
        for (int sq = 0; sq < 64; sq++) {
            // Other Tables
            int value;
            file >> value;
            piece_square_tables[ENDGAME][WHITE][p].at(sq) = value;
            piece_square_tables[ENDGAME][BLACK][p] = reverse_board(piece_square_tables[ENDGAME][WHITE][p]);
        }
        file >> std::ws;
    }
	file.close();
}

int evaluate(Board &board) {
    std::vector<Move> legal_moves = board.get_moves();
    return evaluate(board, legal_moves);
}

int heuristic_diff(Colour us, Move &move, int material_value) {
    int opening_value = 0;
    int endgame_value = 0;
    Colour them = ~us;
    PieceType p = move.moving_piece;
    // Remove the piece from the origin
    opening_value -= piece_square_tables[OPENING][us][p][move.origin];
    endgame_value -= piece_square_tables[ENDGAME][us][p][move.origin];
    // Add the piece from the origin
    opening_value += piece_square_tables[OPENING][us][p][move.target];
    endgame_value += piece_square_tables[ENDGAME][us][p][move.target];

    if (move.is_promotion()) {
        PieceType promoted = get_promoted(move);
        opening_value += piece_square_tables[OPENING][us][promoted][move.target];
        endgame_value += piece_square_tables[ENDGAME][us][promoted][move.target];
    } 
    if (move.is_king_castle()) {
        opening_value -= piece_square_tables[OPENING][us][ROOK][RookSquare[us][KINGSIDE]];
        endgame_value -= piece_square_tables[ENDGAME][us][ROOK][RookSquare[us][KINGSIDE]];
        opening_value += piece_square_tables[OPENING][us][ROOK][move.origin + Direction::E];
        endgame_value += piece_square_tables[ENDGAME][us][ROOK][move.origin + Direction::E];
    } else if (move.is_queen_castle()) {
        opening_value -= piece_square_tables[OPENING][us][ROOK][RookSquare[us][QUEENSIDE]];
        endgame_value -= piece_square_tables[ENDGAME][us][ROOK][RookSquare[us][QUEENSIDE]];
        opening_value += piece_square_tables[OPENING][us][ROOK][move.origin + Direction::W];
        endgame_value += piece_square_tables[ENDGAME][us][ROOK][move.origin + Direction::W];
    } else if (move.is_ep_capture()) {
        const Square captured_square = move.origin.rank()|move.target.file();
        opening_value -= piece_square_tables[OPENING][them][PAWN][captured_square];
        endgame_value -= piece_square_tables[ENDGAME][them][PAWN][captured_square];
    } else if (move.is_capture()) {
        const PieceType cp = to_enum_piece(move.captured_piece);
        opening_value -= piece_square_tables[OPENING][them][cp][move.target];
        endgame_value -= piece_square_tables[ENDGAME][them][cp][move.target];
    }
    
    int value = opening_value + (material_value - OPENING_MATERIAL) * (endgame_value - opening_value) / (ENDGAME_MATERIAL - OPENING_MATERIAL);
    return value;
}

int count_material(Board &board) {
    int material_value = 0;
    for (int p = 0; p < N_PIECE; p++) {
        Bitboard occ = board.pieces((PieceType)p);
        material_value += material[p] * count_bits(occ);
    }
    return material_value;
}

int heuristic(Board &board) {
    int material_value = 0;
    int opening_value = 0;
    int endgame_value = 0;
    for (int p = 0; p < N_PIECE; p++) {
        Bitboard occ = board.pieces(WHITE, (PieceType)p);
        while (occ) {
            material_value += material[p];
            Square sq = pop_lsb(&occ);
            opening_value += piece_square_tables[OPENING][WHITE][p][sq];
            endgame_value += piece_square_tables[ENDGAME][WHITE][p][sq];
        }
        occ = board.pieces(BLACK, (PieceType)p);
        while (occ) {
            material_value += material[p];
            Square sq = pop_lsb(&occ);
            opening_value += piece_square_tables[OPENING][BLACK][p][sq];
            endgame_value += piece_square_tables[ENDGAME][BLACK][p][sq];
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