#include "bitboard.hpp"
#include <iostream>
#include <random>

void Bitboards::pretty(const Bitboard bb) {
    for (uint rank = 8; rank > 0; rank--) {
        for (uint file = 0; file < 8; file++) {
            uint idx = 8 * (rank - 1) + file;
            std::cout << (((bb >> idx) & 1) ? "X" : ".") << ' ';
        }
        std::cout << std::endl;
    }
    std::cout << "0x" << std::hex << bb << std::endl;
    std::cout << std::endl;
}

class PRNG {
  public:
    PRNG(uint64_t s) {
        std::mt19937_64 generator(s);
        _gen = generator;
    }
    uint64_t rand() {
        std::uniform_int_distribution<uint64_t> distribution;
        return distribution(_gen);
    }
    uint64_t sparse() {
        // Average of 64/8 = 8 bits set
        return rand() & rand() & rand();
    }

  private:
    std::mt19937_64 _gen;
};

Bitboard direct_sliding_move(const Bitboard occ, const Square origin, const Direction dir, const uint to_edge) {
    Square target = origin;
    Bitboard bb = 0;
    for (uint i = 0; i < to_edge; i++) {
        target += dir;
        bb |= sq_to_bb(target);
        if (occ & target) {
            break;
        }
    }
    return bb;
}

Bitboard direct_bishop_attack(const Bitboard occ, Square origin) {
    // Calculate bishop attacks for the magic bitboard precalculation.
    Bitboard bb = 0;
    bb |= direct_sliding_move(occ, origin, Direction::NW, origin.to_northwest());
    bb |= direct_sliding_move(occ, origin, Direction::NE, origin.to_northeast());
    bb |= direct_sliding_move(occ, origin, Direction::SW, origin.to_southwest());
    bb |= direct_sliding_move(occ, origin, Direction::SE, origin.to_southeast());
    return bb;
}

Bitboard direct_rook_attack(const Bitboard occ, Square origin) {
    // Calculate bishop attacks for the magic bitboard precalculation.
    Bitboard bb = 0;
    bb |= direct_sliding_move(occ, origin, Direction::N, origin.to_north());
    bb |= direct_sliding_move(occ, origin, Direction::W, origin.to_west());
    bb |= direct_sliding_move(occ, origin, Direction::E, origin.to_east());
    bb |= direct_sliding_move(occ, origin, Direction::S, origin.to_south());
    return bb;
}

void search_magics(PieceType p, Bitboard table[], Magic magics[]) {
    PRNG prng(0x3243f6a8885a308d);
    // This is the most possible occupation combinations for a bishop or rook on
    // any square (2^12), 6 bits on each ray at corner
    Bitboard occupancy[4096], reference[4096];
    int last_size = 0;
    for (int sq = 0; sq < 64; sq++) {
        Square origin(sq);
        Bitboard mask;
        mask = (~(Bitboards::a_file | Bitboards::h_file) | Bitboards::file(sq));
        mask &= ~(Bitboards::rank_1 | Bitboards::rank_8) | Bitboards::rank(sq);
        if (p == BISHOP) {
            mask &= direct_bishop_attack(0, sq);
        } else if (p == ROOK) {
            mask &= direct_rook_attack(0, sq);
        }
        Magic &magic = magics[sq];
        magic.mask = mask;
        magic.shift = 64 - count_bits(mask);
        if (sq == 0) {
            // table decomposed to a pointer to the first element
            magic.ptr = table;
        } else {
            // point to the first unset element of the table.
            magic.ptr = magics[sq - 1].ptr + last_size;
        }
        // Maths, Carry-Ripler trick.
        Bitboard b = 0;
        last_size = 0;
        do {
            occupancy[last_size] = b;
            if (p == BISHOP) {
                reference[last_size] = direct_bishop_attack(b, sq);
            } else if (p == ROOK) {
                reference[last_size] = direct_rook_attack(b, sq);
            }
            last_size++;
            b = (b - mask) & mask;
        } while (b);
        Bitboard guess;
        while (true) {
            bool verify[4096] = {false};
            bool fail_flag = false;
            guess = prng.sparse();
            magic.magic = guess;
            // For this square, go through each occ combination (last_size many)
            for (int i = 0; i < last_size; i++) {
                // Map the occupancy onto a int 0 <= index < 4096
                int index = magic.index(occupancy[i]);
                if (verify[index] == false) {
                    // This index hasn't been seen before.
                    verify[index] = true;
                    magic.ptr[index] = reference[i];
                } else {
                    // This index has been seen before, check if the hashing is
                    // constructive (two different occupations with the same attack map,
                    // mapping to the same index)

                    if (reference[i] == magic.ptr[index]) {
                        // We can continue, this is constructive
                        continue;
                    } else {
                        // Failed the verification test.
                        fail_flag = true;
                        break;
                    }
                }
            }
            // If we failed, the loop has to go around again.
            if (!fail_flag) {
                break;
            }
        }
    }
}

void Bitboards::init() {
    // Zero-initialise

    for (int sq = 0; sq < N_SQUARE; sq++) {
        PawnAttacks[WHITE][sq] = 0;
        PawnAttacks[BLACK][sq] = 0;
        for (int s2 = 0; s2 < N_SQUARE; s2++) {
            LineBBs[sq][s2] = 0;
        }
    }
    for (int p = 0; p < N_PIECE; p++) {
        for (int sq = 0; sq < N_SQUARE; sq++) {
            PseudolegalAttacks[p][sq] = 0;
        }
    }
    for (int sq = 0; sq < N_SQUARE; sq++) {
        SquareBBs[sq] = Bitboard(1) << sq;
    }

    for (int i = 0; i < N_SQUARE; i++) {
        Square origin = Square(i);
        Bitboard origin_bb = sq_to_bb(origin);
        // Knight attacks
        if (origin.to_north() >= 2) {
            if (origin.to_west() >= 1) {
                PseudolegalAttacks[KNIGHT][origin] |= Square(origin + Direction::NNW);
            }
            if (origin.to_east() >= 1) {
                PseudolegalAttacks[KNIGHT][origin] |= Square(origin + Direction::NNE);
            }
        }
        if (origin.to_east() >= 2) {
            if (origin.to_north() >= 1) {
                PseudolegalAttacks[KNIGHT][origin] |= Square(origin + Direction::ENE);
            }
            if (origin.to_south() >= 1) {
                PseudolegalAttacks[KNIGHT][origin] |= Square(origin + Direction::ESE);
            }
        }
        if (origin.to_south() >= 2) {
            if (origin.to_east() >= 1) {
                PseudolegalAttacks[KNIGHT][origin] |= Square(origin + Direction::SSE);
            }
            if (origin.to_west() >= 1) {
                PseudolegalAttacks[KNIGHT][origin] |= Square(origin + Direction::SSW);
            }
        }
        if (origin.to_west() >= 2) {
            if (origin.to_south() >= 1) {
                PseudolegalAttacks[KNIGHT][origin] |= Square(origin + Direction::WSW);
            }
            if (origin.to_north() >= 1) {
                PseudolegalAttacks[KNIGHT][origin] |= Square(origin + Direction::WNW);
            }
        }
        // King attacks
        if (origin.to_north() >= 1) {
            PseudolegalAttacks[KING][origin] |= Square(origin + Direction::N);
            if (origin.to_west() >= 1) {
                PseudolegalAttacks[KING][origin] |= Square(origin + Direction::NW);
            }
            if (origin.to_east() >= 1) {
                PseudolegalAttacks[KING][origin] |= Square(origin + Direction::NE);
            }
        }
        if (origin.to_west() >= 1) {
            PseudolegalAttacks[KING][origin] |= Square(origin + Direction::W);
        }
        if (origin.to_east() >= 1) {
            PseudolegalAttacks[KING][origin] |= Square(origin + Direction::E);
        }
        if (origin.to_south() >= 1) {
            PseudolegalAttacks[KING][origin] |= Square(origin + Direction::S);
            if (origin.to_east() >= 1) {
                PseudolegalAttacks[KING][origin] |= Square(origin + Direction::SE);
            }
            if (origin.to_west() >= 1) {
                PseudolegalAttacks[KING][origin] |= Square(origin + Direction::SW);
            }
        }
        PawnAttacks[WHITE][origin] = shift<Direction::NW>(origin_bb) | shift<Direction::NE>(origin_bb);
        PawnAttacks[BLACK][origin] = shift<Direction::SW>(origin_bb) | shift<Direction::SE>(origin_bb);
        PawnSpans[WHITE][origin] = forward_block_span(WHITE, origin_bb);
        PawnSpans[BLACK][origin] = forward_block_span(BLACK, origin_bb);
    }
    // Initialise magic bitboard tables.
    search_magics(BISHOP, BishopTable, BishopMagics);
    search_magics(ROOK, RookTable, RookMagics);
    for (int i = 0; i < N_SQUARE; i++) {
        Square s1(i);
        PseudolegalAttacks[BISHOP][s1] = bishop_attacks(0, s1);
        PseudolegalAttacks[ROOK][s1] = rook_attacks(0, s1);
        PseudolegalAttacks[QUEEN][s1] = PseudolegalAttacks[BISHOP][s1] | PseudolegalAttacks[ROOK][s1];
    }
    for (int i = 0; i < N_SQUARE; i++) {
        Square s1(i);
        for (int j = 0; j < N_SQUARE; j++) {
            Square s2(j);
            if (PseudolegalAttacks[BISHOP][s1] & s2) { // do squares share diagonal?
                LineBBs[s1][s2] =
                    (PseudolegalAttacks[BISHOP][s1] & PseudolegalAttacks[BISHOP][s2]) | (sq_to_bb(s1) | sq_to_bb(s2));
            }
            if (PseudolegalAttacks[ROOK][s1] & s2) { // do squares share rank or file?
                LineBBs[s1][s2] =
                    (PseudolegalAttacks[ROOK][s1] & PseudolegalAttacks[ROOK][s2]) | (sq_to_bb(s1) | sq_to_bb(s2));
            }
        }
    }
}