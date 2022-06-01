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

// clang-format off

constexpr per_square<score_t> center_dist = {
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 2, 2, 2, 2, 2, 2, 3,
    3, 2, 1, 1, 1, 1, 2, 3,
    3, 2, 1, 0, 0, 1, 2, 3, 
    3, 2, 1, 0, 0, 1, 2, 3,
    3, 2, 1, 1, 1, 1, 2, 3,
    3, 2, 2, 2, 2, 2, 2, 3, 
    3, 3, 3, 3, 3, 3, 3, 3
    };

constexpr per_square<score_t> center_manhattan_dist = { 
    6, 5, 4, 3, 3, 4, 5, 6,
    5, 4, 3, 2, 2, 3, 4, 5,
    4, 3, 2, 1, 1, 2, 3, 4, 
    3, 2, 1, 0, 0, 1, 2, 3, 
    3, 2, 1, 0, 0, 1, 2, 3,
    4, 3, 2, 1, 1, 2, 3, 4,
    5, 4, 3, 2, 2, 3, 4, 5,
    6, 5, 4, 3, 3, 4, 5, 6
    };

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

void init() {
    piece_square_tables[PAWN] = {
        Score(0, 0),     Score(0, 0),    Score(0, 0),     Score(0, 0),      Score(0, 0),     Score(0, 0),
        Score(0, 0),     Score(0, 0),    Score(-2, 160),  Score(-22, 148),  Score(-18, 150), Score(6, 128),
        Score(-26, 100), Score(2, 128),  Score(-42, 124), Score(-150, 138), Score(-17, 63),  Score(-1, 73),
        Score(34, 65),   Score(58, 37),  Score(74, 55),   Score(86, 39),    Score(63, 63),   Score(3, 61),
        Score(-28, 46),  Score(-7, 44),  Score(-12, 32),  Score(9, 24),     Score(29, 24),   Score(16, 30),
        Score(3, 34),    Score(-20, 34), Score(-36, 30),  Score(-26, 38),   Score(-17, 20),  Score(18, 22),
        Score(12, 28),   Score(-7, 26),  Score(-16, 30),  Score(-38, 20),   Score(-40, 30),  Score(-30, 32),
        Score(-24, 24),  Score(-8, 28),  Score(0, 36),    Score(-20, 32),   Score(-6, 26),   Score(-26, 18),
        Score(-30, 32),  Score(-18, 36), Score(-18, 28),  Score(-16, 18),   Score(-10, 58),  Score(-4, 42),
        Score(6, 32),    Score(-26, 22), Score(0, 0),     Score(0, 0),      Score(0, 0),     Score(0, 0),
        Score(0, 0),     Score(0, 0),    Score(0, 0),     Score(0, 0),
    };
    piece_square_tables[KNIGHT] = {
        Score(-114, -30), Score(-34, 16),  Score(-4, 62),  Score(-10, 24), Score(42, 56),  Score(-118, 64),
        Score(14, -2),    Score(-80, -30), Score(48, 10),  Score(22, 38),  Score(158, 16), Score(96, 46),
        Score(144, 52),   Score(164, 28),  Score(68, 24),  Score(82, 18),  Score(40, 36),  Score(58, 40),
        Score(66, 62),    Score(98, 64),   Score(130, 48), Score(184, 46), Score(102, 36), Score(86, 32),
        Score(42, 50),    Score(20, 66),   Score(48, 70),  Score(94, 82),  Score(48, 80),  Score(88, 70),
        Score(24, 74),    Score(68, 48),   Score(2, 50),   Score(6, 52),   Score(34, 72),  Score(24, 82),
        Score(48, 76),    Score(44, 68),   Score(40, 60),  Score(10, 46),  Score(-30, 38), Score(8, 40),
        Score(24, 44),    Score(28, 56),   Score(46, 54),  Score(30, 44),  Score(28, 38),  Score(-22, 58),
        Score(-36, 36),   Score(-24, 48),  Score(6, 22),   Score(30, 40),  Score(26, 40),  Score(28, 34),
        Score(2, 46),     Score(6, 42),    Score(-86, 20), Score(0, 20),   Score(-20, 40), Score(0, 52),
        Score(0, 30),     Score(6, 20),    Score(-6, 32),  Score(-68, -4),
    };
    piece_square_tables[BISHOP] = {
        Score(22, 20), Score(-26, 20), Score(-48, -8), Score(-116, 24), Score(-36, 6),  Score(-90, 12), Score(-34, 4),
        Score(20, 12), Score(60, 6),   Score(66, -8),  Score(36, 16),   Score(-26, 14), Score(30, -2),  Score(52, -4),
        Score(78, 18), Score(54, -14), Score(40, 8),   Score(68, 6),    Score(76, -10), Score(72, 4),   Score(74, 0),
        Score(96, 12), Score(70, -6),  Score(50, 34),  Score(20, 18),   Score(28, 0),   Score(52, 18),  Score(72, 10),
        Score(70, 6),  Score(62, -2),  Score(26, 26),  Score(22, -4),   Score(12, 2),   Score(24, 4),   Score(24, 18),
        Score(70, 26), Score(56, 2),   Score(24, 26),  Score(30, 4),    Score(32, -16), Score(16, 16),  Score(40, 6),
        Score(36, 34), Score(36, 20),  Score(40, 42),  Score(36, 6),    Score(40, 14),  Score(32, 0),   Score(36, -8),
        Score(44, 12), Score(48, -2),  Score(26, 14),  Score(32, 4),    Score(52, 6),   Score(58, -6),  Score(40, 6),
        Score(6, 20),  Score(40, 0),   Score(24, 12),  Score(8, -8),    Score(10, -8),  Score(12, 12),  Score(2, -10),
        Score(4, 10),
    };
    piece_square_tables[ROOK] = {
        Score(52, 68),  Score(50, 82),  Score(44, 84),  Score(56, 94),  Score(58, 88),  Score(76, 94),  Score(54, 100),
        Score(84, 90),  Score(26, 92),  Score(20, 106), Score(40, 110), Score(58, 108), Score(58, 96),  Score(94, 108),
        Score(78, 114), Score(104, 92), Score(14, 92),  Score(34, 102), Score(38, 100), Score(58, 94),  Score(54, 98),
        Score(82, 94),  Score(126, 96), Score(78, 80),  Score(-14, 98), Score(-10, 94), Score(8, 104),  Score(14, 96),
        Score(12, 90),  Score(10, 96),  Score(46, 82),  Score(52, 74),  Score(-28, 90), Score(-32, 80), Score(-32, 90),
        Score(-16, 82), Score(-22, 84), Score(-32, 92), Score(-6, 90),  Score(-12, 78), Score(-48, 84), Score(-30, 76),
        Score(-26, 78), Score(-32, 78), Score(-20, 70), Score(-22, 72), Score(16, 64),  Score(-14, 72), Score(-40, 58),
        Score(-38, 68), Score(-18, 78), Score(-18, 76), Score(-6, 74),  Score(4, 70),   Score(10, 50),  Score(-24, 58),
        Score(-22, 78), Score(-12, 74), Score(2, 72),   Score(10, 74),  Score(10, 72),  Score(2, 80),   Score(10, 68),
        Score(-10, 74),
    };
    piece_square_tables[QUEEN] = {
        Score(24, 184),  Score(46, 162),  Score(74, 198),  Score(-34, 230), Score(74, 210),  Score(148, 180),
        Score(134, 172), Score(114, 172), Score(34, 176),  Score(-10, 190), Score(2, 238),   Score(-4, 262),
        Score(0, 250),   Score(100, 224), Score(46, 236),  Score(122, 188), Score(16, 152),  Score(2, 194),
        Score(46, 164),  Score(32, 190),  Score(72, 218),  Score(98, 216),  Score(110, 230), Score(74, 238),
        Score(4, 156),   Score(-12, 192), Score(8, 186),   Score(4, 214),   Score(16, 226),  Score(26, 240),
        Score(12, 272),  Score(32, 258),  Score(-4, 166),  Score(-12, 176), Score(-14, 208), Score(-4, 194),
        Score(-2, 210),  Score(-8, 220),  Score(18, 196),  Score(10, 206),  Score(-14, 162), Score(2, 172),
        Score(-6, 190),  Score(-4, 182),  Score(-4, 180),  Score(2, 206),   Score(14, 160),  Score(12, 172),
        Score(-16, 140), Score(0, 132),   Score(14, 168),  Score(22, 150),  Score(16, 168),  Score(26, 138),
        Score(24, 124),  Score(34, 106),  Score(-8, 150),  Score(-6, 134),  Score(6, 126),   Score(30, 156),
        Score(18, 142),  Score(-4, 106),  Score(-10, 132), Score(10, 148),
    };
    piece_square_tables[KING] = {
        Score(-2, -60),  Score(-24, -4),  Score(-18, -6),   Score(-20, -2),  Score(-26, 12),   Score(-10, 4),
        Score(-14, 4),   Score(-16, -68), Score(-12, -10),  Score(-36, 70),  Score(-32, 44),   Score(-12, 50),
        Score(-26, 58),  Score(-24, 70),  Score(-16, 84),   Score(-12, 12),  Score(-16, 0),    Score(-30, 54),
        Score(-52, 52),  Score(-30, 56),  Score(-28, 52),   Score(-22, 56),  Score(-20, 46),   Score(-26, 10),
        Score(-156, 6),  Score(-118, 28), Score(-152, 42),  Score(-214, 50), Score(-194, 50),  Score(-128, 48),
        Score(-86, 30),  Score(-98, -10), Score(-150, -28), Score(-154, 16), Score(-202, 24),  Score(-196, 46),
        Score(-200, 38), Score(-190, 26), Score(-150, 10),  Score(-206, -6), Score(-106, -12), Score(-82, 0),
        Score(-110, 14), Score(-134, 16), Score(-140, 22),  Score(-120, 16), Score(-86, 2),    Score(-96, -10),
        Score(-40, -16), Score(-30, 6),   Score(-26, -4),   Score(-62, 4),   Score(-60, 4),    Score(-36, -2),
        Score(-10, -4),  Score(-26, -22), Score(-34, -22),  Score(38, -28),  Score(36, -10),   Score(-34, -32),
        Score(30, -44),  Score(-36, -26), Score(32, -30),   Score(12, -50),
    };

    pb_passed = {
        Score(0, 0),    Score(0, 0),     Score(0, 0),     Score(0, 0),     Score(0, 0),     Score(0, 0),
        Score(0, 0),    Score(0, 0),     Score(124, 154), Score(120, 164), Score(101, 152), Score(104, 156),
        Score(54, 169), Score(107, 158), Score(97, 165),  Score(108, 162), Score(93, 132),  Score(109, 123),
        Score(54, 100), Score(34, 93),   Score(21, 77),   Score(32, 117),  Score(2, 110),   Score(0, 118),
        Score(64, 100), Score(40, 82),   Score(45, 80),   Score(33, 64),   Score(9, 58),    Score(12, 67),
        Score(-15, 91), Score(25, 92),   Score(27, 67),   Score(-1, 46),   Score(-16, 56),  Score(-13, 34),
        Score(-25, 31), Score(-39, 42),  Score(-22, 62),  Score(24, 66),   Score(14, 8),    Score(-39, 7),
        Score(-25, 9),  Score(-34, 6),   Score(-33, 9),   Score(-58, 10),  Score(-78, 18),  Score(-8, 25),
        Score(-14, 9),  Score(-48, -13), Score(-35, 7),   Score(-50, -8),  Score(-18, -2),  Score(-37, 0),
        Score(-56, -3), Score(-8, 11),   Score(0, 0),     Score(0, 0),     Score(0, 0),     Score(0, 0),
        Score(0, 0),    Score(0, 0),     Score(0, 0),     Score(0, 0),
    };

    // Initialise training parameters list (Maybe we should seperate this out as not normally used)

    for (PieceType p = KNIGHT; p < KING; p++) {
        std::string label = "Material " + Printing::piece_name((PieceType)p);
        training_parameters.push_back(labled_parameter(&piece_values[p], label));
    }

    for (int p = KNIGHT; p < KING; p++) {
        std::string label = "Mobility " + Printing::piece_name((PieceType)p);
        training_parameters.push_back(labled_parameter(&mobility[p], label));
    }

    for (int sq = 8; sq < 56; sq++) {
        std::string label = "PSQT PAWN " + Square(sq ^ 56).pretty();
        training_parameters.push_back(labled_parameter(&piece_square_tables[PAWN][sq], label));
    }
    for (PieceType p = KNIGHT; p < N_PIECE; p++) {
        for (int sq = 0; sq < N_SQUARE; sq++) {
            std::string label = "PSQT " + Printing::piece_name((PieceType)p) + " " + Square(sq ^ 56).pretty();
            training_parameters.push_back(labled_parameter(&piece_square_tables[p][sq], label));
        }
    }
    for (int sq = 0; sq < N_SQUARE; sq++) {
        std::string label = "PASSED " + Square(sq ^ 56).pretty();
        training_parameters.push_back(labled_parameter(&pb_passed[sq], label));
    }
}

psqt_t reverse_board(psqt_t in) {
    psqt_t pb;
    for (int s = 0; s < 64; s++) {
        pb[s] = in[56 ^ s];
    }
    return pb;
}

score_t piece_phase_material(const PieceType p) {
    assert(p != NO_PIECE);
    return phase_material[p];
}
Score piece_value(const PieceType p) { return piece_values[p]; }

Score psqt(const Board &board) {
    Score score = Score(0, 0);

    // Piece Square Tables and Material
    for (PieceType p = PAWN; p < N_PIECE; p++) {
        Bitboard occ = board.pieces(WHITE, p);
        while (occ) {
            Square sq = pop_lsb(&occ);
            score += piece_values[p];
            score += piece_square_tables[p][sq.reverse()];
        }
        occ = board.pieces(BLACK, p);
        while (occ) {
            Square sq = pop_lsb(&occ);
            score -= piece_values[p];
            score -= piece_square_tables[p][sq];
        }
    }
    return score;
}

Score psqt_diff(const Colour moving, const Move &move) {
    Score score = Score(0, 0);
    assert(move != NULL_MOVE);
    const PieceType p = move.moving_piece;
    assert(p < NO_PIECE);
    // Apply psqt
    if (moving == WHITE) {
        score += piece_square_tables[p][move.target.reverse()];
        score -= piece_square_tables[p][move.origin.reverse()];
    } else {
        score -= piece_square_tables[p][move.target];
        score += piece_square_tables[p][move.origin];
    }

    // Apply piece value for captures.
    if (move.is_ep_capture()) {
        const Square captured_square(move.origin.rank(), move.target.file());
        assert(move.captured_piece == PAWN);
        if (moving == WHITE) {
            score += piece_values[PAWN];
            score += piece_square_tables[PAWN][captured_square];
        } else {
            score -= piece_values[PAWN];
            score -= piece_square_tables[PAWN][captured_square.reverse()];
        }
    } else if (move.is_capture()) {
        assert(move.captured_piece < NO_PIECE);
        const PieceType cp = move.captured_piece;
        if (moving == WHITE) {
            score += piece_values[move.captured_piece];
            score += piece_square_tables[cp][move.target];
        } else {
            score -= piece_values[move.captured_piece];
            score -= piece_square_tables[cp][move.target.reverse()];
        }
    }

    // Promotions
    if (move.is_promotion()) {
        const PieceType promoted = get_promoted(move);
        assert(promoted < NO_PIECE);
        // We've already dealt with moving the pawn to the 8th rank, where the score is zero. Just add the score from
        // the promoted piece.
        if (moving == WHITE) {
            score += piece_values[promoted];
            score -= piece_values[PAWN];
            score += piece_square_tables[promoted][move.target.reverse()];
        } else {
            score -= piece_values[promoted];
            score += piece_values[PAWN];
            score -= piece_square_tables[promoted][move.target];
        }
    }

    // Castling needs the rook to be moved.
    if (move.is_castle()) {
        const CastlingSide side = move.get_castleside();
        const Square rook_from = RookSquares[moving][side];
        const Square rook_to = RookCastleSquares[moving][side];
        if (moving == WHITE) {
            score += piece_square_tables[ROOK][rook_to.reverse()];
            score -= piece_square_tables[ROOK][rook_from.reverse()];
        } else {
            score -= piece_square_tables[ROOK][rook_to];
            score += piece_square_tables[ROOK][rook_from];
        }
    }
    return score;
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

score_t evaluate_white(const Board &board) {
    Score score = board.get_psqt();
    // Mobility
    // A bonus is given to every square accessible to every piece, which isn't blocked by one of our pieces.
    for (PieceType p = KNIGHT; p < KING; p++) {
        Bitboard occ = board.pieces(WHITE, p);
        while (occ) {
            Square sq = pop_lsb(&occ);
            Bitboard mob = Bitboards::attacks(p, board.pieces(), sq);
            mob &= ~board.pawn_controlled(BLACK);
            mob &= ~board.pieces(WHITE);
            score += mobility[p] * count_bits(mob);
        }
        occ = board.pieces(BLACK, p);
        while (occ) {
            Square sq = pop_lsb(&occ);
            Bitboard mob = Bitboards::attacks(p, board.pieces(), sq);
            mob &= ~board.pawn_controlled(WHITE);
            mob &= ~board.pieces(BLACK);
            score -= mobility[p] * count_bits(mob);
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

    // Give a penalty for having a minor piece on a square that cannot be defended by a pawn.
    occ = (board.pieces(WHITE, KNIGHT) | board.pieces(WHITE, BISHOP)) & board.weak_squares(BLACK);
    score += piece_weak_square * count_bits(occ);
    occ = (board.pieces(BLACK, KNIGHT) | board.pieces(BLACK, BISHOP)) & board.weak_squares(WHITE);
    score -= piece_weak_square * count_bits(occ);

    // Pawn structure bonuses:
    score += eval_pawns(board);

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
             count_bits(board.pieces(WHITE, ROOK) & Bitboards::rear_span<WHITE>(board.passed_pawns(WHITE)));
    score -= rook_behind_passed *
             count_bits(board.pieces(BLACK, ROOK) & Bitboards::rear_span<BLACK>(board.passed_pawns(BLACK)));

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

    // We consider 3 game phases. Opening, where phase_material > OPENING_MATERIAL. Endgame, where phase_material <
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
    score_t value = score.interpolate(board.phase_material());
    return value;
}

} // namespace Evaluation

void print_table(const psqt_t table) {
    for (int i = 0; i < 64; i++) {
        std::cout << std::setfill(' ') << std::setw(4) << table[i].opening_score << " ";
        if (i % 8 == 7) {
            std::cout << std::endl;
        }
    }
    std::cout << std::endl;

    for (int i = 0; i < 64; i++) {
        std::cout << std::setfill(' ') << std::setw(4) << table[i].endgame_score << " ";
        if (i % 8 == 7) {
            std::cout << std::endl;
        }
    }
    std::cout << std::endl;
}

void Evaluation::print_tables() {
    std::cout << "MATERIAL" << std::endl;
    for (int i = 0; i < 6; i++) {
        std::cout << std::setfill(' ') << std::setw(4) << phase_material[i] << " ";
    }
    std::cout << std::endl;
    std::cout << "VALUES" << std::endl;
    for (int i = 0; i < 6; i++) {
        std::cout << std::setfill(' ') << std::setw(4) << phase_material[i] << " ";
    }
    std::cout << std::endl;
    std::cout << "OPENING" << std::endl;
    for (PieceType p = PAWN; p < N_PIECE; p++) {
        std::cout << Printing::piece_name(p) << std::endl;
        print_table(piece_square_tables[p]);
    }

    std::cout << "PASSED PAWN" << std::endl;
    print_table(pb_passed);
}

void Evaluation::load_tables(std::string filename) {

    std::fstream file;
    file.open(filename, std::ios::in);
    if (!file) {
        std::cout << "No such file: " << filename << std::endl;
        return;
    }

    for (labled_parameter &S : training_parameters) {
        file >> S.first->opening_score;
        file >> S.first->endgame_score;
        file >> std::ws;
    }
    file.close();
}

void Evaluation::save_tables(std::string filename) {

    std::fstream file;
    file.open(filename, std::ios::out);
    for (const labled_parameter &S : training_parameters) {
        file << *S.first << std::endl;
    }
    file.close();
}

score_t Evaluation::count_material(const Board &board) {
    score_t material_value = 0;
    for (PieceType p = PAWN; p < N_PIECE; p++) {
        material_value += phase_material[p] * (board.count_pieces(WHITE, p) + board.count_pieces(BLACK, p));
    }
    return material_value;
}

score_t Evaluation::eval(const Board &board) {
    // Return the eval from the point of view of the current player.
    if (board.is_white_move()) {
        return evaluate_white(board);
    } else {
        return -evaluate_white(board);
    }
}

score_t Evaluation::eval_psqt(const Board &board) {
    // Return the eval from the point of view of the current player.
    const score_t material_value = board.phase_material();
    const Score score = board.get_psqt();
    score_t value = score.interpolate(material_value);

    if (board.is_white_move()) {
        return value;
    } else {
        return -value;
    }
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
