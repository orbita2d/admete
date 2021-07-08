#include "board.hpp"
#include "types.hpp"
#include <random>

void Zobrist::init() {
    std::mt19937_64 generator(0x3243f6a8885a308d);
    std::uniform_int_distribution<zobrist_t> distribution;
    // Fill table with random bitstrings
    for (int c = 0; c < N_COLOUR; c++) {
        for (int p = 0; p < N_PIECE; p++) {
            for (int sq = 0; sq < N_SQUARE; sq++) {
                zobrist_table[c][p][sq] = distribution(generator);
            }
        }
    }
    // En-passent, one key per file
    for (int i = 0; i < 8; i++) {
        zobrist_table_ep[i] = distribution(generator);
    }
    // Who to play
    zobrist_table_move[WHITE] = distribution(generator);
    zobrist_table_move[BLACK] = distribution(generator);
    // Castling rights
    zobrist_table_cr[WHITE][KINGSIDE] = distribution(generator);
    zobrist_table_cr[WHITE][QUEENSIDE] = distribution(generator);
    zobrist_table_cr[BLACK][KINGSIDE] = distribution(generator);
    zobrist_table_cr[BLACK][QUEENSIDE] = distribution(generator);
}

zobrist_t Zobrist::hash(const Board &board) {
    zobrist_t hash = 0;

    for (int p = 0; p < N_PIECE; p++) {
        Bitboard occ = board.pieces(WHITE, (PieceType)p);
        while (occ) {
            Square sq = pop_lsb(&occ);
            hash ^= zobrist_table[WHITE][p][sq];
        }
        occ = board.pieces(BLACK, (PieceType)p);
        while (occ) {
            Square sq = pop_lsb(&occ);
            hash ^= zobrist_table[BLACK][p][sq];
        }
    }

    hash ^= zobrist_table_move[board.who_to_play()];
    // Castling rights
    if (board.can_castle(WHITE, KINGSIDE)) {
        hash ^= zobrist_table_cr[WHITE][KINGSIDE];
    }
    if (board.can_castle(WHITE, QUEENSIDE)) {
        hash ^= zobrist_table_cr[WHITE][QUEENSIDE];
    }
    if (board.can_castle(BLACK, KINGSIDE)) {
        hash ^= zobrist_table_cr[BLACK][KINGSIDE];
    }
    if (board.can_castle(BLACK, QUEENSIDE)) {
        hash ^= zobrist_table_cr[BLACK][QUEENSIDE];
    }
    // en-passent

    if (board.en_passent()) {
        hash ^= zobrist_table_ep[board.en_passent().file_index()];
    }

    return hash;
}

// Material Key. Unique key for N1 pawns, N2 knights, ... etc
zobrist_t Zobrist::material(const Board &board) {
    zobrist_t hash = 0;

    for (PieceType p = PAWN; p < N_PIECE; p++) {
        Bitboard occ = board.pieces(WHITE, p);
        for (int i = 0; i < count_bits(occ); i++) {
            hash ^= zobrist_table[WHITE][p][i];
        }
        occ = board.pieces(BLACK, p);
        for (int i = 0; i < count_bits(occ); i++) {
            hash ^= zobrist_table[BLACK][p][i];
        }
    }
    return hash;
}

zobrist_t Zobrist::diff(const Move move, const Colour us, const int last_ep_file,
                        const std::array<std::array<bool, N_COLOUR>, N_CASTLE> castling_rights_change) {
    zobrist_t hash = 0;
    Colour them = ~us;
    hash ^= zobrist_table[us][move.moving_piece][move.origin];
    hash ^= zobrist_table[us][move.moving_piece][move.target];

    if (last_ep_file >= 0) {
        hash ^= zobrist_table_ep[last_ep_file];
    }
    if (move.is_double_push()) {
        hash ^= zobrist_table_ep[move.origin.file_index()];
    }

    for (int i = 0; i < N_COLOUR; i++) {
        for (int j = 0; j < N_CASTLE; j++) {
            if (castling_rights_change[i][j]) {
                hash ^= zobrist_table_cr[i][j];
            }
        }
    }
    if (move.is_ep_capture()) {
        // En-passent is weird.
        const Square captured_square = move.origin.rank() | move.target.file();
        // Remove their pawn from the ep-square
        hash ^= zobrist_table[them][PAWN][captured_square];
    } else if (move.is_capture()) {
        hash ^= zobrist_table[them][move.captured_piece][move.target];
    } else if (move.is_king_castle()) {
        hash ^= zobrist_table[us][ROOK][RookSquares[us][KINGSIDE]];
        hash ^= zobrist_table[us][ROOK][move.origin + Direction::E];
    } else if (move.is_queen_castle()) {
        hash ^= zobrist_table[us][ROOK][RookSquares[us][QUEENSIDE]];
        hash ^= zobrist_table[us][ROOK][move.origin + Direction::W];
    }
    if (move.is_promotion()) {
        hash ^= zobrist_table[us][PAWN][move.target];
        hash ^= zobrist_table[us][get_promoted(move)][move.target];
    }
    hash ^= zobrist_table_move[us];
    hash ^= zobrist_table_move[them];

    return hash;
}

zobrist_t Zobrist::nulldiff(const Colour us, const int last_ep_file) {
    zobrist_t hash = 0;
    Colour them = ~us;

    if (last_ep_file >= 0) {
        hash ^= zobrist_table_ep[last_ep_file];
    }

    hash ^= zobrist_table_move[us];
    hash ^= zobrist_table_move[them];

    return hash;
}
