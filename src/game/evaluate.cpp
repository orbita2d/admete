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
    ENDGAME,
    N_GAMEPHASE
};

// We consider 3 game phases. Opening, where material > OPENING_MATERIAL. Endgame, where material < ENDGAME_MATERIAL. And a midgame in between.
// Evalutation is linearly interpolated in the midgame:
/*
op  |mid| eg
----+---+----
----
    \
     \
      \
        -----
*/
constexpr int OPENING_MATERIAL = 6000;
constexpr int ENDGAME_MATERIAL = 2000;

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
    // Square from Black's perspective;
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
                                                20,  20,  10,   0,   0,  10,  20,  20 };


constexpr position_board pb_king_endgame = {    0,  0,  0,  0,  0,  0,  0,  0,
                                                0, 10, 10, 10, 10, 10, 10,  0,
                                                0, 10, 20, 20, 20, 20, 10,  0,
                                                0, 10, 20, 20, 20, 20, 10,  0,
                                                0, 10, 20, 20, 20, 20, 10,  0,
                                                0, 10, 20, 20, 20, 20, 10,  0,
                                                0, 10, 10, 10, 10, 10, 10,  0, 
                                                0,  0,  0,  0,  0,  0,  0,  0 };


constexpr position_board pb_pawn_opening = {      0,   0,   0,   0,   0,   0,   0,   0,
                                                100, 100, 100, 100, 100, 100, 100, 100,
                                                105, 105, 110, 110, 110, 110, 105, 105,
                                                100, 105, 110, 115, 115, 110, 105, 100,
                                                100, 100, 115, 130, 130, 115, 100, 100,
                                                100, 100, 100, 100, 100, 100, 100, 100,
                                                100, 100, 100, 100, 100, 100, 100, 100, 
                                                  0,   0,   0,   0,   0,   0,   0,   0 };

constexpr position_board pb_pawn_endgame = {      0,   0,   0,   0,   0,   0,   0,   0,
                                                120, 120, 120, 120, 120, 120, 120, 120,
                                                115, 115, 115, 115, 115, 115, 115, 115,
                                                110, 110, 110, 110, 110, 110, 110, 110,
                                                105, 110, 110, 110, 110, 110, 110, 110,
                                                100, 100, 100, 100, 100, 100, 100, 100,
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

constexpr position_board pb_rook_op = { 500, 500, 500, 500, 500, 500, 500, 500,
                                        520, 520, 520, 520, 520, 520, 520, 520,
                                        500, 500, 500, 500, 500, 500, 500, 500,
                                        500, 500, 500, 500, 500, 500, 500, 500,
                                        500, 500, 500, 500, 500, 500, 500, 500,
                                        500, 500, 500, 500, 500, 500, 500, 500,
                                        500, 500, 500, 500, 500, 500, 500, 500, 
                                        500, 500, 500, 500, 500, 500, 500, 500 };

constexpr position_board pb_rook_eg = { 500, 500, 500, 500, 500, 500, 500, 500,
                                        500, 500, 500, 500, 500, 500, 500, 500,
                                        500, 500, 500, 500, 500, 500, 500, 500,
                                        500, 500, 500, 500, 500, 500, 500, 500,
                                        500, 500, 500, 500, 500, 500, 500, 500,
                                        500, 500, 500, 500, 500, 500, 500, 500,
                                        500, 500, 500, 500, 500, 500, 500, 500, 
                                        500, 500, 500, 500, 500, 500, 500, 500 };

constexpr int weak_pawn = -5; // Pawn not defended by another pawn.
constexpr int isolated_pawn = -10; // Pawn with no supporting pawns on adjacent files
constexpr int connected_passed = 20; // Bonus for connected passed pawns (per pawn)
constexpr int rook_open_file = 10; // Bonus for rook on open file
constexpr int rook_hopen_file = 5; // Bonus for rook on half open file

constexpr int castle_hopen_file = -5; // Penalty for castled king near a half open file

// Bonus given the pawns on the 2nd and 3rd ranks in front of a castled king. This promotes castling and penalises pushing pawns in front of the king.
constexpr int castle_pawns2 = 5;
constexpr int castle_pawns3 = 5;

// Penealty for squares where a queen would check the king, if only pawns were on the board.
constexpr int queen_check = -8;

// Bonus given to passed pawns, multiplied by PSqT below
constexpr int passed_mult_op = 10;
constexpr int passed_mult_eg = 15;
// PSqT for passed pawns.
constexpr position_board pb_p_pawn = {   0,  0,  0,  0,  0,  0,  0,  0,
                                        10, 10, 10, 10, 10, 10, 10, 10,
                                         5,  5,  5,  5,  5,  5,  5,  5,
                                         4,  4,  4,  4,  4,  4,  4,  4,
                                         3,  3,  3,  3,  3,  3,  3,  3,
                                         2,  2,  2,  2,  2,  2,  2,  2,
                                         1,  1,  1,  1,  1,  1,  1,  1, 
                                         0,  0,  0,  0,  0,  0,  0,  0 };

static std::array<std::array<position_board_set, N_COLOUR>, N_GAMEPHASE> piece_square_tables;
static position_board pb_passed[N_COLOUR];

namespace Evaluation {
    void init() {
        piece_square_tables[OPENING][WHITE][PAWN] = pb_pawn_opening;
        piece_square_tables[OPENING][WHITE][KNIGHT] = fill_knight_positional_scores();
        piece_square_tables[OPENING][WHITE][BISHOP] = pb_bishop;
        piece_square_tables[OPENING][WHITE][ROOK] = pb_rook_op;
        piece_square_tables[OPENING][WHITE][QUEEN] = pb_queen;
        piece_square_tables[OPENING][WHITE][KING] = pb_king_opening;

        piece_square_tables[OPENING][BLACK][PAWN] = reverse_board(pb_pawn_opening);
        piece_square_tables[OPENING][BLACK][KNIGHT] = reverse_board(fill_knight_positional_scores());
        piece_square_tables[OPENING][BLACK][BISHOP] = reverse_board(pb_bishop);
        piece_square_tables[OPENING][BLACK][ROOK] = reverse_board(pb_rook_op);
        piece_square_tables[OPENING][BLACK][QUEEN] = reverse_board(pb_queen);
        piece_square_tables[OPENING][BLACK][KING] = reverse_board(pb_king_opening);


        piece_square_tables[ENDGAME][WHITE][PAWN] = pb_pawn_endgame;
        piece_square_tables[ENDGAME][WHITE][KNIGHT] = fill_knight_positional_scores();
        piece_square_tables[ENDGAME][WHITE][BISHOP] = pb_bishop;
        piece_square_tables[ENDGAME][WHITE][ROOK] = pb_rook_eg;
        piece_square_tables[ENDGAME][WHITE][QUEEN] = pb_queen;
        piece_square_tables[ENDGAME][WHITE][KING] = pb_king_endgame;

        piece_square_tables[ENDGAME][BLACK][PAWN] = reverse_board(pb_pawn_endgame);
        piece_square_tables[ENDGAME][BLACK][KNIGHT] = reverse_board(fill_knight_positional_scores());
        piece_square_tables[ENDGAME][BLACK][BISHOP] = reverse_board(pb_bishop);
        piece_square_tables[ENDGAME][BLACK][ROOK] = reverse_board(pb_rook_eg);
        piece_square_tables[ENDGAME][BLACK][QUEEN] = reverse_board(pb_queen);
        piece_square_tables[ENDGAME][BLACK][KING] = reverse_board(pb_king_endgame);

        pb_passed[WHITE] = pb_p_pawn;
        pb_passed[BLACK] = reverse_board(pb_p_pawn);
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
    std::cout << "KNIGHT" << std::endl;
    print_table(piece_square_tables[OPENING][WHITE][KNIGHT]);
    std::cout << "BISHOP" << std::endl;
    print_table(piece_square_tables[OPENING][WHITE][BISHOP]);
    std::cout << "ROOK" << std::endl;
    print_table(piece_square_tables[OPENING][WHITE][ROOK]);
    std::cout << "QUEEN" << std::endl;
    print_table(piece_square_tables[OPENING][WHITE][QUEEN]);
    std::cout << "KING" << std::endl;
    print_table(piece_square_tables[OPENING][WHITE][KING]);

    std::cout << "ENDGAME" << std::endl;
    std::cout << "PAWN" << std::endl;
    print_table(piece_square_tables[ENDGAME][WHITE][PAWN]);
    std::cout << "KNIGHT" << std::endl;
    print_table(piece_square_tables[ENDGAME][WHITE][KNIGHT]);
    std::cout << "BISHOP" << std::endl;
    print_table(piece_square_tables[ENDGAME][WHITE][BISHOP]);
    std::cout << "ROOK" << std::endl;
    print_table(piece_square_tables[ENDGAME][WHITE][ROOK]);
    std::cout << "QUEEN" << std::endl;
    print_table(piece_square_tables[ENDGAME][WHITE][QUEEN]);
    std::cout << "KING" << std::endl;
    print_table(piece_square_tables[ENDGAME][WHITE][KING]);
    
    std::cout << "PASSED PAWN" << std::endl;
    print_table(pb_passed[WHITE]);
    print_table(pb_passed[BLACK]);
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

    // Pawn structure bonuses:

    // Add bonus for passed pawns for each side.
    Bitboard occ = board.passed_pawns(WHITE);
    while (occ) {
        Square sq = pop_lsb(&occ);
        opening_value += passed_mult_op * pb_passed[WHITE][sq];
        endgame_value += passed_mult_eg * pb_passed[WHITE][sq];
    }

    occ = board.passed_pawns(BLACK);
    while (occ) {
        Square sq = pop_lsb(&occ);
        opening_value += passed_mult_op * pb_passed[BLACK][sq];
        endgame_value += passed_mult_eg * pb_passed[BLACK][sq];
    }

    // Bonus for connected passers.
    occ = board.connected_passed_pawns(WHITE);
    opening_value += connected_passed * count_bits(occ);
    endgame_value += connected_passed * count_bits(occ);

    occ = board.connected_passed_pawns(BLACK);
    opening_value -= connected_passed * count_bits(occ);
    endgame_value -= connected_passed * count_bits(occ);
    
    // Penalty for weak pawns.
    occ = board.weak_pawns(WHITE);
    opening_value += weak_pawn * count_bits(occ);
    endgame_value += weak_pawn * count_bits(occ);

    occ = board.weak_pawns(BLACK);
    opening_value -= weak_pawn * count_bits(occ);
    endgame_value -= weak_pawn * count_bits(occ);

    // Penalty for isolated pawns.
    occ = board.isolated_pawns(WHITE);
    opening_value += isolated_pawn * count_bits(occ);
    endgame_value += isolated_pawn * count_bits(occ);

    occ = board.isolated_pawns(BLACK);
    opening_value -= isolated_pawn * count_bits(occ);
    endgame_value -= isolated_pawn * count_bits(occ);

    // Rooks in open and half-open files;
    const Bitboard open_files = board.open_files();
    const Bitboard w_hopen_files = board.half_open_files(WHITE);
    const Bitboard b_hopen_files = board.half_open_files(BLACK);

    occ = board.pieces(WHITE, ROOK) & open_files;
    opening_value += rook_open_file * count_bits(occ);

    occ = board.pieces(BLACK, ROOK) & open_files;
    opening_value -= rook_open_file * count_bits(occ);

    occ = board.pieces(WHITE, ROOK) & w_hopen_files;
    opening_value += rook_hopen_file * count_bits(occ);

    occ = board.pieces(BLACK, ROOK) & b_hopen_files;
    opening_value -= rook_hopen_file * count_bits(occ);

    // King safety

    const Bitboard wk = board.find_king(WHITE);
    const Bitboard bk = board.find_king(BLACK);
    
    if (wk & Bitboards::castle_king[WHITE][KINGSIDE]) {
        opening_value += castle_pawns2 * count_bits(Bitboards::castle_pawn2[WHITE][KINGSIDE] & board.pieces(WHITE, PAWN));
        opening_value += castle_pawns3 * count_bits(Bitboards::castle_pawn3[WHITE][KINGSIDE] & board.pieces(WHITE, PAWN));
        opening_value += castle_hopen_file * count_bits(Bitboards::castle_king[WHITE][KINGSIDE] & w_hopen_files);
    } else if (wk & Bitboards::castle_king[WHITE][QUEENSIDE]) {
        opening_value += castle_pawns2 * count_bits(Bitboards::castle_pawn2[WHITE][QUEENSIDE] & board.pieces(WHITE, PAWN));
        opening_value += castle_pawns3 * count_bits(Bitboards::castle_pawn3[WHITE][QUEENSIDE] & board.pieces(WHITE, PAWN));
        opening_value += castle_hopen_file * count_bits(Bitboards::castle_king[WHITE][QUEENSIDE] & w_hopen_files);
    }

    if (bk & Bitboards::castle_king[BLACK][KINGSIDE]) {
        opening_value -= castle_pawns2 * count_bits(Bitboards::castle_pawn2[BLACK][KINGSIDE] & board.pieces(BLACK, PAWN));
        opening_value -= castle_pawns3 * count_bits(Bitboards::castle_pawn3[BLACK][KINGSIDE] & board.pieces(BLACK, PAWN));
        opening_value -= castle_hopen_file * count_bits(Bitboards::castle_king[BLACK][KINGSIDE] & b_hopen_files);
    } else if (bk & Bitboards::castle_king[BLACK][QUEENSIDE]) {
        opening_value -= castle_pawns2 * count_bits(Bitboards::castle_pawn2[BLACK][QUEENSIDE] & board.pieces(BLACK, PAWN));
        opening_value -= castle_pawns3 * count_bits(Bitboards::castle_pawn3[BLACK][QUEENSIDE] & board.pieces(BLACK, PAWN));
        opening_value -= castle_hopen_file * count_bits(Bitboards::castle_king[BLACK][QUEENSIDE] & b_hopen_files);
    }

    // Punish squares where a queen could check the king, if there were only pawns on the board, without being captured by a pawn, loose heuristic for king safety.

    occ = Bitboards::attacks<QUEEN>(board.pieces(PAWN), board.find_king(WHITE));
    occ &= ~board.pieces(PAWN);
    occ &= ~Bitboards::pawn_attacks(WHITE, board.pieces(WHITE, PAWN));
    opening_value += queen_check * count_bits(occ);

    occ = Bitboards::attacks<QUEEN>(board.pieces(PAWN), board.find_king(BLACK));
    occ &= ~board.pieces(PAWN);
    occ &= ~Bitboards::pawn_attacks(BLACK, board.pieces(BLACK, PAWN));
    opening_value -= queen_check * count_bits(occ);
    
    int value;
    if (material_value > OPENING_MATERIAL) {
        // Just use opening tables
        value = opening_value;
    } else if (material_value > ENDGAME_MATERIAL) {
        // Interpolate linearly between the game phases.
        value = opening_value + (material_value - OPENING_MATERIAL) * (endgame_value - opening_value) / (ENDGAME_MATERIAL - OPENING_MATERIAL);
    } else {
        // Just use endgame tables
        value = endgame_value;
    }
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