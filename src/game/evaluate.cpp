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
    constants();
    // Initialise training parameters list (Maybe we should seperate this out as not normally used)
    for (PieceType p = KNIGHT; p < KING; p++) {
        std::string label = "Material " + Printing::piece_name((PieceType)p);
        // training_parameters.push_back(labled_parameter(&piece_values[p], label));
    }

    for (int p = KNIGHT; p < KING; p++) {
        std::string label = "Mobility " + Printing::piece_name((PieceType)p);
        // training_parameters.push_back(labled_parameter(&mobility[p], label));
    }

    for (PieceType p = PAWN; p < N_PIECE; p++) {
        for (int sq = 0; sq < N_SQUARE; sq++) {
            std::string label = "PSQT[" + Printing::piece_name(p) + "][" + std::to_string(sq) + "]";
            // training_parameters.push_back(labled_parameter(&PSQT[p][sq], label));
        }
    }
    for (int sq = 0; sq < N_SQUARE; sq++) {
        std::string label = "pb_passed[" + std::to_string(sq) + "]";
        // training_parameters.push_back(labled_parameter(&pb_passed[sq], label));
    }
    for (CastlingSide s = KINGSIDE; s < N_SIDE; s++) {
        for (CastlingSide t = KINGSIDE; t < N_SIDE; t++) {
            for (PieceType p = PAWN; p < KING; p++) {
                for (int sq = 0; sq < N_SQUARE; sq++) {
                    std::string label = "side_piece_square_tables[" + Printing::side_name(s) + "][" +
                                        Printing::side_name(t) + "][" + Printing::piece_name(p) + "][" +
                                        std::to_string(sq) + "]";
                    training_parameters.push_back(labled_parameter(&side_piece_square_tables[s][t][p][sq], label));
                }
            }
        }
    }
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
Score piece_value(const PieceType p) { return piece_values[p]; }

Score psqt(const Board &board) {
    // Calculate PSQT values for default PSQT, for both colours.
    Score score = Score(0, 0);

    // Piece Square Tables and Material
    for (PieceType p = PAWN; p < N_PIECE; p++) {
        Bitboard occ = board.pieces(WHITE, p);
        while (occ) {
            Square sq = pop_lsb(&occ);
            score += piece_values[p];
            score += PSQT[p][sq.reverse()];
        }
        occ = board.pieces(BLACK, p);
        while (occ) {
            Square sq = pop_lsb(&occ);
            score -= piece_values[p];
            score -= PSQT[p][sq];
        }
    }
    return score;
}

Score psqt(const Board &board, const psqt_t psqt, const Colour c) {
    Score score = Score(0, 0);
    // Piece Square Tables and Material
    if (c == WHITE) {
        for (PieceType p = PAWN; p < N_PIECE; p++) {
            Bitboard occ = board.pieces(WHITE, p);
            while (occ) {
                Square sq = pop_lsb(&occ);
                score += psqt[p][sq.reverse()];
            }
        }
    } else {
        for (PieceType p = PAWN; p < N_PIECE; p++) {
            Bitboard occ = board.pieces(BLACK, p);
            while (occ) {
                Square sq = pop_lsb(&occ);
                score -= psqt[p][sq];
            }
        }
    }
    return score;
}

Score psqt_diff(const Colour moving, const psqt_t psqt, const Move &move) {
    Score score = Score(0, 0);
    assert(move != NULL_MOVE);
    const PieceType p = move.moving_piece;
    assert(p < NO_PIECE);
    // Apply psqt
    if (moving == WHITE) {
        score += psqt[p][move.target.reverse()];
        score -= psqt[p][move.origin.reverse()];
    } else {
        score -= psqt[p][move.target];
        score += psqt[p][move.origin];
    }

    // Apply piece value for captures.
    if (move.is_ep_capture()) {
        const Square captured_square(move.origin.rank(), move.target.file());
        assert(move.captured_piece == PAWN);
        if (moving == WHITE) {
            score += psqt[PAWN][captured_square];
        } else {
            score -= psqt[PAWN][captured_square.reverse()];
        }
    } else if (move.is_capture()) {
        assert(move.captured_piece < NO_PIECE);
        const PieceType cp = move.captured_piece;
        if (moving == WHITE) {
            score += psqt[cp][move.target];
        } else {
            score -= psqt[cp][move.target.reverse()];
        }
    }

    // Promotions
    if (move.is_promotion()) {
        const PieceType promoted = get_promoted(move);
        assert(promoted < NO_PIECE);
        // We've already dealt with moving the pawn to the 8th rank, where the score is zero. Just add the score from
        // the promoted piece.
        if (moving == WHITE) {
            score += psqt[promoted][move.target.reverse()];
        } else {
            score -= psqt[promoted][move.target];
        }
    }

    // Castling needs the rook to be moved.
    if (move.is_castle()) {
        const CastlingSide side = move.get_castleside();
        const Square rook_from = RookSquares[moving][side];
        const Square rook_to = RookCastleSquares[moving][side];
        if (moving == WHITE) {
            score += psqt[ROOK][rook_to.reverse()];
            score -= psqt[ROOK][rook_from.reverse()];
        } else {
            score -= psqt[ROOK][rook_to];
            score += psqt[ROOK][rook_from];
        }
    }
    return score;
}

Score material_diff(const Colour moving, const Move &move) {
    Score score = Score(0, 0);
    assert(move != NULL_MOVE);

    // Apply piece value for captures.
    if (move.is_ep_capture()) {
        const Square captured_square(move.origin.rank(), move.target.file());
        assert(move.captured_piece == PAWN);
        if (moving == WHITE) {
            score += piece_values[PAWN];
        } else {
            score -= piece_values[PAWN];
        }
    } else if (move.is_capture()) {
        assert(move.captured_piece < NO_PIECE);
        if (moving == WHITE) {
            score += piece_values[move.captured_piece];
        } else {
            score -= piece_values[move.captured_piece];
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
        } else {
            score -= piece_values[promoted];
            score += piece_values[PAWN];
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
    // Sided castling sides
    const Bitboard wk = sq_to_bb(board.find_king(WHITE));
    const Bitboard bk = sq_to_bb(board.find_king(BLACK));
    {
        constexpr unsigned cols = 2;
        constexpr unsigned width = 8 / (cols - 1);
        const unsigned s = board.find_king(WHITE).file_index() % width;
        const unsigned t = board.find_king(BLACK).file_index() % width;

        score += (s * t * psqt(board, side_piece_square_tables[0][0], WHITE)) / 64;
        score += ((width - s - 1) * t * psqt(board, side_piece_square_tables[1][0], WHITE)) / 64;
        score += (s * (width - t - 1) * psqt(board, side_piece_square_tables[0][1], WHITE)) / 64;
        score += ((width - s - 1) * (width - t - 1) * psqt(board, side_piece_square_tables[1][1], WHITE)) / 64;

        score += (s * t * psqt(board, side_piece_square_tables[0][0], BLACK)) / 64;
        score += ((width - s - 1) * t * psqt(board, side_piece_square_tables[0][1], BLACK)) / 64;
        score += (s * (width - t - 1) * psqt(board, side_piece_square_tables[1][0], BLACK)) / 64;
        score += ((width - s - 1) * (width - t - 1) * psqt(board, side_piece_square_tables[1][1], BLACK)) / 64;
    }
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

void print_table(const sqt_t table) {
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
        print_table(PSQT[p]);
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
    for (const labled_parameter &S : training_parameters) {
        std::string label;
        file >> S.first->opening_score;
        file >> S.first->endgame_score;
        file >> label;
        file >> std::ws;
        if (S.second != label) {
            std::cerr << label << "=/=" << S.first << std::endl;
        }
    }
    file.close();
}

void Evaluation::save_tables(std::string filename) {
    std::fstream file;
    file.open(filename, std::ios::out);
    for (const labled_parameter &S : training_parameters) {
        file << *S.first << " " << S.second << std::endl;
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
