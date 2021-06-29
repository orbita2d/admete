#include "evaluate.hpp"
#include "board.hpp"
#include "piece.hpp"
#include <array>
#include <fstream>
#include <iomanip>
#include <iostream>

// Want some consideration of positional play

enum GamePhase { OPENING, ENDGAME, N_GAMEPHASE };

inline int reverse_rank(int square) {
    // Square from Black's perspective;
    return (56 - (square & 0x38)) | (square & 0x07);
}

position_board reverse_board(position_board in) {
    position_board pb;
    for (int s = 0; s < 64; s++) {
        pb[s] = -in[reverse_rank(s)];
    }
    return pb;
}

// clang-format off

constexpr position_board center_dist = {3, 3, 3, 3, 3, 3, 3, 3,
                                        3, 2, 2, 2, 2, 2, 2, 3,
                                        3, 2, 1, 1, 1, 1, 2, 3,
                                        3, 2, 1, 0, 0, 1, 2, 3, 
                                        3, 2, 1, 0, 0, 1, 2, 3,
                                        3, 2, 1, 1, 1, 1, 2, 3,
                                        3, 2, 2, 2, 2, 2, 2, 3, 
                                        3, 3, 3, 3, 3, 3, 3, 3};

constexpr position_board center_manhattan_dist = {  6, 5, 4, 3, 3, 4, 5, 6,
                                                    5, 4, 3, 2, 2, 3, 4, 5,
                                                    4, 3, 2, 1, 1, 2, 3, 4, 
                                                    3, 2, 1, 0, 0, 1, 2, 3, 
                                                    3, 2, 1, 0, 0, 1, 2, 3,
                                                    4, 3, 2, 1, 1, 2, 3, 4,
                                                    5, 4, 3, 2, 2, 3, 4, 5,
                                                    6, 5, 4, 3, 3, 4, 5, 6};

constexpr position_board lightsquare_corner_distance = {0, 1, 2, 3, 4, 5, 6, 7,
                                                        1, 2, 3, 4, 5, 6, 7, 6,
                                                        2, 3, 4, 5, 6, 7, 6, 5,
                                                        3, 4, 5, 6, 7, 6, 5, 4,
                                                        4, 5, 6, 7, 6, 5, 4, 3,
                                                        5, 6, 7, 6, 5, 4, 3, 2,
                                                        6, 7, 6, 5, 2, 3, 2, 1,
                                                        7, 6, 5, 4, 3, 2, 1, 0};

constexpr position_board darksquare_corner_distance = { 7, 6, 5, 4, 3, 2, 1, 0,
                                                        6, 7, 6, 5, 4, 3, 2, 1,
                                                        5, 6, 7, 6, 5, 4, 3, 2,
                                                        4, 5, 6, 7, 6, 5, 4, 3,
                                                        3, 4, 5, 6, 7, 6, 5, 4,
                                                        2, 3, 4, 5, 6, 7, 6, 5,
                                                        1, 2, 3, 4, 5, 6, 7, 6,
                                                        0, 1, 2, 3, 4, 5, 6, 7};

constexpr position_board pb_knight = {  0, 0,  0,  0,  0,  0,  0,  0,
                                        0, 10, 10, 10, 10, 10, 10, 0,
                                        0, 10, 20, 20, 20, 20, 10, 0,
                                        0, 10, 20, 20, 20, 20, 10, 0,
                                        0, 10, 20, 20, 20, 20, 10, 0,
                                        0, 10, 20, 20, 20, 20, 10, 0,
                                        0, 10, 10, 10, 10, 10, 10, 0,
                                        0, 0,  0,  0,  0,  0,  0,  0};

constexpr position_board pb_bishop = {   20,  0,  0,  0,  0,  0,  0, 20,
                                          0, 20,  0,  0,  0,  0, 20,  0,
                                          0,  0, 20, 10, 10, 20,  0,  0,
                                          0,  0, 10, 20, 20, 10,  0,  0,
                                          0,  0, 10, 20, 20, 10,  0,  0,
                                          0,  0, 20, 10, 10, 20,  0,  0,
                                          0, 20,  0,  0,  0,  0, 20,  0,
                                         20,  0,  0,  0,  0,  0,  0, 20};

constexpr position_board pb_king_opening = { -20, -20, -20, -20, -20, -20, -20, -20,
                                             -20, -20, -20, -20, -20, -20, -20, -20,
                                             -20, -20, -20, -20, -20, -20, -20, -20,
                                             -20, -20, -20, -20, -20, -20, -20, -20,
                                             -20, -20, -20, -20, -20, -20, -20, -20,
                                             -20, -20, -20, -20, -20, -20, -20, -20,
                                             -10, -20, -20, -20, -20, -20, -20, -20,
                                              20,  20,  10,  0,   0,   10,  20,  20};

constexpr position_board pb_king_endgame = { 0, 0,  0,  0,  0,  0,  0,  0,
                                             0, 10, 10, 10, 10, 10, 10, 0,
                                             0, 10, 20, 20, 20, 20, 10, 0,
                                             0, 10, 20, 20, 20, 20, 10, 0,
                                             0, 10, 20, 20, 20, 20, 10, 0,
                                             0, 10, 20, 20, 20, 20, 10, 0,
                                             0, 10, 10, 10, 10, 10, 10, 0,
                                             0, 0,  0,  0,  0,  0,  0,  0};

constexpr position_board pb_pawn_opening = { 0,  0,  0,  0,  0,  0,  0,  0,
                                             0,  0,  0,  0,  0,  0,  0,  0,
                                             5,  5, 10, 10, 10, 10,  5,  5,
                                             0,  5, 10, 15, 15, 10,  5,  0,
                                             0,  0, 15, 30, 30, 15,  0,  0,
                                             0,  0,  0,  0,  0,  0,  0,  0,
                                             0,  0,  0,  0,  0,  0,  0,  0,
                                             0,  0,  0,  0,  0,  0,  0,  0};

constexpr position_board pb_pawn_endgame = { 0,  0,  0,  0,  0,  0,  0,  0,
                                            20, 20, 20, 20, 20, 20, 20, 20,
                                            15, 15, 15, 15, 15, 15, 15, 15,
                                            10, 10, 10, 10, 10, 10, 10, 10,
                                             5, 10, 10, 10, 10, 10, 10, 10,
                                             0,  0,  0,  0,  0,  0,  0,  0,
                                             0,  0,  0,  0,  0,  0,  0,  0,
                                             0,  0,  0,  0,  0,  0,  0,  0};

constexpr position_board pb_queen = {   0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0};

constexpr position_board pb_rook_op = { 0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0};

constexpr position_board pb_rook_eg = { 0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0};


// PSqT for passed pawns.
constexpr position_board pb_p_pawn = {   0,  0,  0,  0,  0,  0,  0,  0,
                                        10, 10, 10, 10, 10, 10, 10, 10,
                                         5,  5,  5,  5,  5,  5,  5,  5,
                                         4,  4,  4,  4,  4,  4,  4,  4,
                                         3,  3,  3,  3,  3,  3,  3,  3,
                                         2,  2,  2,  2,  2,  2,  2,  2,
                                         1,  1,  1,  1,  1,  1,  1,  1,
                                         0,  0,  0,  0,  0,  0,  0,  0 };
// clang-format on

inline Score operator*(const int a, const Score s) { return Score(a * s.opening_score, a * s.endgame_score); }

inline Score operator*(const Score s, const int a) { return a * s; }

constexpr Score weak_pawn = Score(-5, -5);        // Pawn not defended by another pawn.
constexpr Score isolated_pawn = Score(-10, -10);  // Pawn with no supporting pawns on adjacent files
constexpr Score connected_passed = Score(20, 20); // Bonus for connected passed pawns (per pawn)
constexpr Score rook_open_file = Score(10, 5);    // Bonus for rook on open file
constexpr Score rook_hopen_file = Score(5, 0);    // Bonus for rook on half open file

constexpr Score castle_hopen_file = Score(-5, 0); // Penalty for castled king near a half open file.

// Bonus given the pawns on the 2nd and 3rd ranks in front of a castled king. This promotes castling and penalises
// pushing pawns in front of the king.
constexpr Score castle_pawns2 = Score(6, 0);
constexpr Score castle_pawns3 = Score(4, 0);

// Penealty for squares where a queen would check the king if only pawns were on the board.
constexpr Score queen_check = Score(-8, 0);

// Bonus for every square accessible (that isn't protected by a pawn) to every piece.
constexpr Score mobility = Score(6, 8);

// Bonus for having the bishop pair.
constexpr Score bishop_pair = Score(15, 15);

// Bonus for a piece on a weak enemy square.
constexpr Score piece_weak_square = Score(0, 0);

// Bonus for a knight on an outpost.
constexpr Score knight_outpost = Score(15, 0);

// Bonus for an outpost (occupied or otherwise).
constexpr Score outpost = Score(5, 0);

// Bonus given to passed pawns, multiplied by PSqT below.
constexpr Score passed_mult = Score(10, 15);

// Bonus for rook behind a passed pawn.
constexpr Score rook_behind_passed = Score(5, 20);

// Multiplier for special psqt to push enemy king into corner same colour as our only bishop.
constexpr Score bishop_corner_multiplier = Score(0, 8);

static std::array<std::array<position_board_set, N_COLOUR>, N_GAMEPHASE> piece_square_tables;
static position_board pb_passed[N_COLOUR];

// Piece values here for evaluation heuristic.
static std::array<Score, 6> piece_values = {
    {Score(100, 100), Score(300, 300), Score(330, 350), Score(500, 500), Score(900, 900), Score(0, 0)}};

// Material here is for determining the game phase.
static std::array<score_t, 6> material = {{100, 300, 350, 500, 900, 0}};

namespace Evaluation {
void init() {
    piece_square_tables[OPENING][WHITE][PAWN] = pb_pawn_opening;
    piece_square_tables[OPENING][WHITE][KNIGHT] = pb_knight;
    piece_square_tables[OPENING][WHITE][BISHOP] = pb_bishop;
    piece_square_tables[OPENING][WHITE][ROOK] = pb_rook_op;
    piece_square_tables[OPENING][WHITE][QUEEN] = pb_queen;
    piece_square_tables[OPENING][WHITE][KING] = pb_king_opening;

    piece_square_tables[ENDGAME][WHITE][PAWN] = pb_pawn_endgame;
    piece_square_tables[ENDGAME][WHITE][KNIGHT] = pb_knight;
    piece_square_tables[ENDGAME][WHITE][BISHOP] = pb_bishop;
    piece_square_tables[ENDGAME][WHITE][ROOK] = pb_rook_eg;
    piece_square_tables[ENDGAME][WHITE][QUEEN] = pb_queen;
    piece_square_tables[ENDGAME][WHITE][KING] = pb_king_endgame;

    for (int p = 0; p < N_PIECE; p++) {
        piece_square_tables[OPENING][BLACK][p] = reverse_board(piece_square_tables[OPENING][WHITE][p]);
        piece_square_tables[ENDGAME][BLACK][p] = reverse_board(piece_square_tables[ENDGAME][WHITE][p]);
    }

    pb_passed[WHITE] = pb_p_pawn;
    pb_passed[BLACK] = reverse_board(pb_passed[WHITE]);
}

score_t piece_material(const PieceType p) { return material[p]; }
Score piece_value(const PieceType p) { return piece_values[p]; }

} // namespace Evaluation

score_t Evaluation::heuristic(Board &board) {
    int material_value = 0;
    Score score = Score(0, 0);
    for (int p = 0; p < N_PIECE; p++) {
        Bitboard occ = board.pieces(WHITE, (PieceType)p);
        while (occ) {
            material_value += material[p];
            Square sq = pop_lsb(&occ);
            score += piece_values[p];
            score += Score(piece_square_tables[OPENING][WHITE][p][sq], piece_square_tables[ENDGAME][WHITE][p][sq]);
        }
        occ = board.pieces(BLACK, (PieceType)p);
        while (occ) {
            material_value += material[p];
            Square sq = pop_lsb(&occ);
            score -= piece_values[p];
            score += Score(piece_square_tables[OPENING][BLACK][p][sq], piece_square_tables[ENDGAME][BLACK][p][sq]);
        }
    }

    // Mobility
    // A bonus is given to every square accessible to every piece, which isn't blocked by one of our pieces.
    for (int p = (int)KNIGHT; p < KING; p++) {
        Bitboard occ = board.pieces(WHITE, (PieceType)p);
        while (occ) {
            Square sq = pop_lsb(&occ);
            Bitboard mob = Bitboards::attacks((PieceType)p, board.pieces(), sq);
            mob &= ~board.pawn_controlled(BLACK);
            mob &= ~board.pieces(WHITE);
            score += mobility * count_bits(mob);
        }
        occ = board.pieces(BLACK, (PieceType)p);
        while (occ) {
            Square sq = pop_lsb(&occ);
            Bitboard mob = Bitboards::attacks((PieceType)p, board.pieces(), sq);
            mob &= ~board.pawn_controlled(WHITE);
            mob &= ~board.pieces(BLACK);
            score -= mobility * count_bits(mob);
        }
    }

    // Bishops

    // Work out which sides have which bishops.
    bool bishop_types[N_COLOUR][N_BISHOPTYPES] = {{false, false}, {false, false}};

    Bitboard occ = board.pieces(WHITE, BISHOP);
    if (occ & Bitboards::light_squares) {
        bishop_types[WHITE][LIGHTSQUARE] = true;
    }
    if (occ & Bitboards::dark_squares) {
        bishop_types[WHITE][DARKSQUARE] = true;
    }

    occ = board.pieces(BLACK, BISHOP);
    if (occ & Bitboards::light_squares) {
        bishop_types[BLACK][LIGHTSQUARE] = true;
    }
    if (occ & Bitboards::dark_squares) {
        bishop_types[BLACK][DARKSQUARE] = true;
    }

    // Give bonus for having the bishop pair
    if (bishop_types[WHITE][LIGHTSQUARE] && bishop_types[WHITE][DARKSQUARE]) {
        // White has the bishop pair
        score += bishop_pair;
    }
    if (bishop_types[BLACK][LIGHTSQUARE] && bishop_types[BLACK][DARKSQUARE]) {
        // Black has the bishop pair
        score -= bishop_pair;
    }

    // Look for one-bishop situations and give relevant bonuses
    if (bishop_types[WHITE][LIGHTSQUARE] && !bishop_types[WHITE][DARKSQUARE]) {
        // White only has one, lightsquare bishop.
        // Push the enemy king into that corner
        score -= bishop_corner_multiplier * (lightsquare_corner_distance[board.find_king(BLACK)] - 4);
    } else if (!bishop_types[WHITE][LIGHTSQUARE] && bishop_types[WHITE][DARKSQUARE]) {
        // White only has one, darksquare bishop.
        score -= bishop_corner_multiplier * (darksquare_corner_distance[board.find_king(BLACK)] - 4);
    }

    if (bishop_types[BLACK][LIGHTSQUARE] && !bishop_types[BLACK][DARKSQUARE]) {
        // Black only has one, lightsquare bishop.
        score += bishop_corner_multiplier * (lightsquare_corner_distance[board.find_king(WHITE)] - 4);
    } else if (!bishop_types[BLACK][LIGHTSQUARE] && bishop_types[BLACK][DARKSQUARE]) {
        // Black only has one, darksquare bishop.
        score += bishop_corner_multiplier * (darksquare_corner_distance[board.find_king(WHITE)] - 4);
    }

    // Knights
    // Give bonus for every outpost on the board.
    // An outpost is a square, defended by pawns, that can never be attacked by enemy pawns.
    score += outpost * count_bits(board.outposts(WHITE));
    score -= outpost * count_bits(board.outposts(BLACK));

    // Give additional bonus for having a knight in an outpost.
    occ = board.pieces(WHITE, KNIGHT) & board.outposts(WHITE);
    score += knight_outpost * count_bits(occ);

    occ = board.pieces(BLACK, KNIGHT) & board.outposts(BLACK);
    score -= knight_outpost * count_bits(occ);

    // Give a penalty for having a piece on a square that cannot be defended by a pawn.
    occ = (board.pieces(WHITE, KNIGHT) | board.pieces(WHITE, BISHOP)) & board.weak_squares(BLACK);
    score += piece_weak_square * count_bits(occ);
    occ = (board.pieces(BLACK, KNIGHT) | board.pieces(BLACK, BISHOP)) & board.weak_squares(WHITE);
    score -= piece_weak_square * count_bits(occ);

    // Pawn structure bonuses:

    // Add bonus for passed pawns for each side.
    occ = board.passed_pawns(WHITE);
    while (occ) {
        Square sq = pop_lsb(&occ);
        score += passed_mult * pb_passed[WHITE][sq];
    }

    occ = board.passed_pawns(BLACK);
    while (occ) {
        Square sq = pop_lsb(&occ);
        score += passed_mult * pb_passed[BLACK][sq];
    }

    // Bonus for connected passers.
    occ = board.connected_passed_pawns(WHITE);
    score += connected_passed * count_bits(occ);

    occ = board.connected_passed_pawns(BLACK);
    score -= connected_passed * count_bits(occ);

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

    // Rooks
    const Bitboard open_files = board.open_files();
    const Bitboard w_hopen_files = board.half_open_files(WHITE);
    const Bitboard b_hopen_files = board.half_open_files(BLACK);

    // Bonus for a rook on an open file.
    occ = board.pieces(WHITE, ROOK) & open_files;
    score += rook_open_file * count_bits(occ);

    occ = board.pieces(BLACK, ROOK) & open_files;
    score -= rook_open_file * count_bits(occ);

    // Bonus for a rook on a half-open file.
    occ = board.pieces(WHITE, ROOK) & w_hopen_files;
    score += rook_hopen_file * count_bits(occ);

    occ = board.pieces(BLACK, ROOK) & b_hopen_files;
    score -= rook_hopen_file * count_bits(occ);

    // Bonus for a rook behind a passed pawn.
    score += rook_behind_passed *
             count_bits(board.pieces(WHITE, ROOK) & Bitboards::rear_span(WHITE, board.passed_pawns(WHITE)));
    score -= rook_behind_passed *
             count_bits(board.pieces(BLACK, ROOK) & Bitboards::rear_span(BLACK, board.passed_pawns(BLACK)));

    // King safety
    const Bitboard wk = sq_to_bb(board.find_king(WHITE));
    const Bitboard bk = sq_to_bb(board.find_king(BLACK));

    if (wk & Bitboards::castle_king[WHITE][KINGSIDE]) {
        score += castle_pawns2 * count_bits(Bitboards::castle_pawn2[WHITE][KINGSIDE] & board.pieces(WHITE, PAWN));
        score += castle_pawns3 * count_bits(Bitboards::castle_pawn3[WHITE][KINGSIDE] & board.pieces(WHITE, PAWN));
        score += castle_hopen_file * count_bits(Bitboards::castle_king[WHITE][KINGSIDE] & w_hopen_files);
    } else if (wk & Bitboards::castle_king[WHITE][QUEENSIDE]) {
        score += castle_pawns2 * count_bits(Bitboards::castle_pawn2[WHITE][QUEENSIDE] & board.pieces(WHITE, PAWN));
        score += castle_pawns3 * count_bits(Bitboards::castle_pawn3[WHITE][QUEENSIDE] & board.pieces(WHITE, PAWN));
        score += castle_hopen_file * count_bits(Bitboards::castle_king[WHITE][QUEENSIDE] & w_hopen_files);
    }

    if (bk & Bitboards::castle_king[BLACK][KINGSIDE]) {
        score -= castle_pawns2 * count_bits(Bitboards::castle_pawn2[BLACK][KINGSIDE] & board.pieces(BLACK, PAWN));
        score -= castle_pawns3 * count_bits(Bitboards::castle_pawn3[BLACK][KINGSIDE] & board.pieces(BLACK, PAWN));
        score -= castle_hopen_file * count_bits(Bitboards::castle_king[BLACK][KINGSIDE] & b_hopen_files);
    } else if (bk & Bitboards::castle_king[BLACK][QUEENSIDE]) {
        score -= castle_pawns2 * count_bits(Bitboards::castle_pawn2[BLACK][QUEENSIDE] & board.pieces(BLACK, PAWN));
        score -= castle_pawns3 * count_bits(Bitboards::castle_pawn3[BLACK][QUEENSIDE] & board.pieces(BLACK, PAWN));
        score -= castle_hopen_file * count_bits(Bitboards::castle_king[BLACK][QUEENSIDE] & b_hopen_files);
    }

    // Punish squares where a queen could check the king, if there were only pawns on the board, without being captured
    // by a pawn, loose heuristic for king safety.

    occ = Bitboards::attacks<QUEEN>(board.pieces(PAWN), board.find_king(WHITE));
    occ &= ~board.pieces(PAWN);
    occ &= ~board.pawn_controlled(WHITE);
    score += queen_check * count_bits(occ);

    occ = Bitboards::attacks<QUEEN>(board.pieces(PAWN), board.find_king(BLACK));
    occ &= ~board.pieces(PAWN);
    occ &= ~board.pawn_controlled(BLACK);
    score -= queen_check * count_bits(occ);

    // We consider 3 game phases. Opening, where material > OPENING_MATERIAL. Endgame, where material <
    // ENDGAME_MATERIAL. And a midgame in between. Evalutation is linearly interpolated in the midgame:
    /*
    op  |mid| eg
    ----+---+----
    ----
        \
         \
          \
            -----
    */
    score_t value;
    if (material_value > OPENING_MATERIAL) {
        // Just use opening tables
        value = score.opening_score;
    } else if (material_value > ENDGAME_MATERIAL) {
        // Interpolate linearly between the game phases.
        value = score.opening_score + (material_value - OPENING_MATERIAL) *
                                          (score.endgame_score - score.opening_score) /
                                          (ENDGAME_MATERIAL - OPENING_MATERIAL);
    } else {
        // Just use endgame tables
        value = score.endgame_score;
    }
    return value;
}

void print_table(const position_board table) {
    for (int i = 0; i < 64; i++) {
        std::cout << std::setfill(' ') << std::setw(4) << table[i] << " ";
        if (i % 8 == 7) {
            std::cout << std::endl;
        }
    }
    std::cout << std::endl;
}

void print_tables() {
    std::cout << "MATERIAL" << std::endl;
    for (int i = 0; i < 6; i++) {
        std::cout << std::setfill(' ') << std::setw(4) << material[i] << " ";
    }
    std::cout << "VALUES" << std::endl;
    for (int i = 0; i < 6; i++) {
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
    }
    piece_square_tables[OPENING][BLACK][PAWN] = reverse_board(piece_square_tables[OPENING][WHITE][PAWN]);
    file >> std::ws;
    for (int p = KNIGHT; p < N_PIECE; p++) {
        for (int sq = 0; sq < 64; sq++) {
            // Other Tables
            int value;
            file >> value;
            piece_square_tables[OPENING][WHITE][p].at(sq) = value;
        }
        piece_square_tables[OPENING][BLACK][p] = reverse_board(piece_square_tables[OPENING][WHITE][p]);
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

score_t Evaluation::count_material(const Board &board) {
    score_t material_value = 0;
    for (score_t p = 0; p < N_PIECE; p++) {
        material_value +=
            material[p] * (board.count_pieces(WHITE, (PieceType)p) + board.count_pieces(BLACK, (PieceType)p));
    }
    return material_value;
}

score_t Evaluation::negamax_heuristic(Board &board) {
    int side_multiplier = board.is_white_move() ? 1 : -1;
    return heuristic(board) * side_multiplier;
}

score_t Evaluation::evaluate(Board &board, std::vector<Move> &legal_moves) {
    // evaluate the position relative to the current player.
    // First check if we have been mated.
    if (legal_moves.size() == 0) {
        if (board.is_check()) {
            // This is checkmate
            return -MATING_SCORE + board.ply();
        } else {
            // This is stalemate.
            return drawn_score(board);
        }
    }
    return negamax_heuristic(board);
}

score_t Evaluation::evaluate(Board &board) {
    std::vector<Move> legal_moves = board.get_moves();
    return evaluate(board, legal_moves);
}

score_t Evaluation::drawn_score(const Board &board) {
    // Returns a relative score for what we should consider a draw.
    // This implements contempt, such that the engine player should try avoid draws.
    // If we are an even number of nodes from root, the root player is the current player.
    const bool root_player = (board.height() % 2) == 0;
    if (root_player) {
        return contempt;
    } else {
        return -contempt;
    }
}