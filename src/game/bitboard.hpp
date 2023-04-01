#pragma once
#include "types.hpp"
#include <assert.h>
#include <bit>
#include <inttypes.h>

typedef uint64_t Bitboard;

inline Bitboard PseudolegalAttacks[N_PIECE][N_SQUARE];
inline Bitboard PawnAttacks[N_COLOUR][N_SQUARE];
inline Bitboard LineBBs[N_SQUARE][N_SQUARE];
inline Bitboard SquareBBs[N_SQUARE];

inline Bitboard sq_to_bb(const int s) {
    // The space-time balance doesn't work out here, cheaper to calculate than look up.
    return Bitboard(1) << s;
}
inline Bitboard sq_to_bb(const Square &s) { return Bitboard(1) << s; }

inline Bitboard operator&(Bitboard b, const Square &s) { return b & sq_to_bb(s); }
inline Bitboard operator|(Bitboard b, const Square &s) { return b | sq_to_bb(s); }
inline Bitboard operator^(Bitboard b, const Square &s) { return b ^ sq_to_bb(s); }
inline Bitboard &operator|=(Bitboard &b, const Square &s) { return b |= sq_to_bb(s); }
inline Bitboard &operator^=(Bitboard &b, const Square &s) { return b ^= sq_to_bb(s); }

// Trying to implement the magic bitboard in https://www.chessprogramming.org/Magic_Bitboards "fancy"
struct Magic {
    // Pointer to attacks bitboard
    Bitboard *ptr;
    Bitboard mask;
    Bitboard magic;
    int shift;
    int index(Bitboard occ) const { return unsigned(((occ & mask) * magic) >> shift); }
};

inline Bitboard BishopTable[0x1480]; // The length of these I grabbed from stockfish but cannot justify.
inline Bitboard RookTable[0x19000];

inline Magic BishopMagics[N_SQUARE];
inline Magic RookMagics[N_SQUARE];

inline Bitboard bishop_attacks(Bitboard occ, const Square &sq) {
    const Magic magic = BishopMagics[sq];
    Bitboard *aptr = magic.ptr;
    return aptr[magic.index(occ)];
}

inline Bitboard rook_attacks(Bitboard occ, const Square &sq) {
    const Magic magic = RookMagics[sq];
    Bitboard *aptr = magic.ptr;
    return aptr[magic.index(occ)];
}

inline Bitboard bishop_xrays(Bitboard occ, const Square &sq) {
    const Bitboard atk = bishop_attacks(occ, sq);
    return bishop_attacks(occ ^ (occ & atk), sq);
}

inline Bitboard rook_xrays(Bitboard occ, const Square &sq) {
    const Bitboard atk = rook_attacks(occ, sq);
    return rook_attacks(occ ^ (occ & atk), sq);
}

namespace Bitboards {
void pretty(Bitboard);
void init();
constexpr Bitboard a_file = 0x0101010101010101;
constexpr Bitboard b_file = 0x0202020202020202;
constexpr Bitboard c_file = 0x0404040404040404;
constexpr Bitboard d_file = 0x0808080808080808;
constexpr Bitboard e_file = 0x1010101010101010;
constexpr Bitboard f_file = 0x2020202020202020;
constexpr Bitboard g_file = 0x4040404040404040;
constexpr Bitboard h_file = 0x8080808080808080;
constexpr Bitboard rank_8 = 0xff00000000000000;
constexpr Bitboard rank_7 = 0x00ff000000000000;
constexpr Bitboard rank_6 = 0x0000ff0000000000;
constexpr Bitboard rank_5 = 0x000000ff00000000;
constexpr Bitboard rank_4 = 0x00000000ff000000;
constexpr Bitboard rank_3 = 0x0000000000ff0000;
constexpr Bitboard rank_2 = 0x000000000000ff00;
constexpr Bitboard rank_1 = 0x00000000000000ff;
constexpr Bitboard rank_bb[8] = {rank_1, rank_2, rank_3, rank_4, rank_5, rank_6, rank_7, rank_8};
constexpr Bitboard file_bb[8] = {a_file, b_file, c_file, d_file, e_file, f_file, g_file, h_file};
constexpr Bitboard middle_ranks = rank_3 | rank_4 | rank_5 | rank_6;
constexpr Bitboard null = 0x0000000000000000;
constexpr Bitboard omega = 0xffffffffffffffff;
// Bitboard for squares where king is castled
constexpr Bitboard castle_king[N_COLOUR][N_SIDE] = {{0xe0e0e0, 0x070707}, {0xe0e0e00000000000, 0x707070000000000}};
// Bitboard for king safetly pawns on 2nd rank
constexpr Bitboard castle_pawn2[N_COLOUR][N_SIDE] = {{0xe000, 0x700}, {0x00e0000000000000, 0x007000000000000}};
// Bitboard for king safetly pawns on 3rd rank
constexpr Bitboard castle_pawn3[N_COLOUR][N_SIDE] = {{0xe00000, 0x70000}, {0x0000e00000000000, 0x000070000000000}};

// Bitboards for squares that cannot be occupied for caslting to be legal.
constexpr Bitboard castle_blocks_bb[N_COLOUR][N_SIDE] = {{0x60, 0xe}, {0x6000000000000000, 0xe00000000000000}};
// Bitboards for squares that cannot be attacked for caslting to be legal.
constexpr Bitboard castle_checks_bb[N_COLOUR][N_SIDE] = {{0x70, 0xc}, {0x7000000000000000, 0xc00000000000000}};

constexpr Bitboard dark_squares = 0xAA55AA55AA55AA55;
constexpr Bitboard light_squares = 0x55AA55AA55AA55AA;
constexpr Bitboard bishop_squares[N_BISHOPTYPES] = {light_squares, dark_squares};

constexpr Bitboard rook_from_to[N_COLOUR][N_SIDE] = {{0xa0, 0x09}, {0xa000000000000000, 0x900000000000000}};

template <Direction dir> constexpr Bitboard shift(const Bitboard bb) {
    // Bitboards are stored big-endian but who can remember that, so this hides the implementation a little
    if constexpr (dir == Direction::N) {
        return (bb << 8);
    } else if constexpr (dir == Direction::S) {
        return (bb >> 8);
    } else if constexpr (dir == Direction::E) {
        return (bb << 1) & ~Bitboards::a_file;
    } else if constexpr (dir == Direction::W) {
        return (bb >> 1) & ~Bitboards::h_file;
    } else if constexpr (dir == Direction::NW) {
        return (bb << 7) & ~Bitboards::h_file;
    } else if constexpr (dir == Direction::NE) {
        return (bb << 9) & ~Bitboards::a_file;
    } else if constexpr (dir == Direction::SW) {
        return (bb >> 9) & ~Bitboards::h_file;
    } else if constexpr (dir == Direction::SE) {
        return (bb >> 7) & ~Bitboards::a_file;
    } else if constexpr (dir == Direction::SS) {
        return (bb >> 16);
    } else if constexpr (dir == Direction::NN) {
        return (bb << 16);
    } else if constexpr (dir == Direction::EE) {
        return (bb << 2) & ~(Bitboards::a_file | Bitboards::b_file);
    } else if constexpr (dir == Direction::WW) {
        return (bb >> 2) & ~(Bitboards::h_file | Bitboards::g_file);
    }
}

inline Bitboard pseudo_attacks(const PieceType p, const Square &s) {
    // Get the value from the pseudolegal attacks table
    return PseudolegalAttacks[p][s];
}

inline Bitboard attacks(const PieceType p, const Bitboard occ, const Square &sq) {
    if (p == ROOK) {
        return rook_attacks(occ, sq);
    } else if (p == BISHOP) {
        return bishop_attacks(occ, sq);
    } else if (p == QUEEN) {
        return bishop_attacks(occ, sq) | rook_attacks(occ, sq);
    } else {
        return PseudolegalAttacks[p][sq];
    }
}

template <PieceType p> inline Bitboard attacks(const Bitboard occ, const Square &sq) {
    if constexpr (p == ROOK) {
        return rook_attacks(occ, sq);
    } else if constexpr (p == BISHOP) {
        return bishop_attacks(occ, sq);
    } else if constexpr (p == QUEEN) {
        return bishop_attacks(occ, sq) | rook_attacks(occ, sq);
    } else {
        return PseudolegalAttacks[p][sq];
    }
}

constexpr Bitboard rank(const Rank r) { return rank_bb[r]; }
constexpr Bitboard file(const File f) { return file_bb[f]; }

inline Bitboard rank(const Square &s) { return rank(s.rank()); }
inline Bitboard file(const Square &s) { return file(s.file()); }

inline Bitboard line(const Square &s1, const Square &s2) { return LineBBs[s1][s2]; }
inline Bitboard between(const Square &s1, const Square &s2) {
    Bitboard bb = line(s1, s2);
    if (bb) {
        bb &= ((omega << s1) ^ (omega << s2));
        return bb & (bb - 1);
    } else {
        return 0;
    }
}

constexpr Bitboard castle_blocks(const Colour c, const CastlingSide cs) { return castle_blocks_bb[c][cs]; }
constexpr Bitboard castle_checks(const Colour c, const CastlingSide cs) { return castle_checks_bb[c][cs]; }

inline Bitboard north_fill(Bitboard g) {
    g |= g << 0x08;
    g |= g << 0x10;
    g |= g << 0x20;
    return g;
}

inline Bitboard south_fill(Bitboard g) {
    g |= g >> 0x08;
    g |= g >> 0x10;
    g |= g >> 0x20;
    return g;
}

inline Bitboard vertical_fill(Bitboard g) {
    // Fill the whole file for every bit in g
    return north_fill(south_fill(g));
}

// Squares that a pawn could be to not be passed.
inline Bitboard north_block_span(Bitboard g) {
    g = north_fill(g);
    g = shift<Direction::N>(g);
    g |= shift<Direction::W>(g);
    g |= shift<Direction::E>(g);
    return g;
}

inline Bitboard south_block_span(Bitboard g) {
    g = south_fill(g);
    g = shift<Direction::S>(g);
    g |= shift<Direction::W>(g);
    g |= shift<Direction::E>(g);
    return g;
}

template <Colour c> inline Bitboard forward_span(const Bitboard g) {
    assert(c == WHITE || c == BLACK);
    if (c == WHITE) {
        return shift<Direction::N>(north_fill(g));
    } else {
        return shift<Direction::S>(south_fill(g));
    }
}
template <Colour c> inline Bitboard rear_span(const Bitboard g) {
    assert(c == WHITE || c == BLACK);
    if (c == WHITE) {
        return shift<Direction::S>(south_fill(g));
    } else {
        return shift<Direction::N>(north_fill(g));
    }
}

template <Colour c> inline Bitboard reverse_pawn_push(Bitboard g) {
    assert(c == WHITE || c == BLACK);
    if (c == WHITE) {
        g = shift<Direction::S>(g);
        return g;
    } else {
        g = shift<Direction::N>(g);
        return g;
    }
}

template <Colour c> inline Bitboard reverse_pawn_double_push(Bitboard g) {
    assert(c == WHITE || c == BLACK);
    if (c == WHITE) {
        g = shift<Direction::S>(g);
        return shift<Direction::S>(g);
    } else {
        g = shift<Direction::N>(g);
        return shift<Direction::N>(g);
    }
}

template <Colour c> inline Bitboard forward_block_span(Bitboard g) {
    assert(c == WHITE || c == BLACK);
    if (c == WHITE) {
        return north_block_span(g);
    } else {
        return south_block_span(g);
    }
}

template <Colour c> inline Bitboard rear_block_span(Bitboard g) {
    assert(c == WHITE || c == BLACK);
    if (c == WHITE) {
        return south_block_span(g);
    } else {
        return north_block_span(g);
    }
}

template <Colour c> inline Bitboard forward_atk_span(Bitboard g) {
    assert(c == WHITE || c == BLACK);
    g = forward_span<c>(g);
    return shift<Direction::W>(g) | shift<Direction::E>(g);
}

template <Colour c> inline Bitboard rear_atk_span(Bitboard g) {
    assert(c == WHITE || c == BLACK);
    g = rear_span<c>(g);
    return shift<Direction::W>(g) | shift<Direction::E>(g);
}

inline Bitboard full_atk_span(const Bitboard g) {
    Bitboard f = vertical_fill(g);
    return shift<Direction::W>(f) | shift<Direction::E>(f);
}

template <Colour c> inline Bitboard forward_fill(const Bitboard g) {
    if (c == WHITE) {
        return north_fill(g);
    } else {
        return south_fill(g);
    }
}

template <Colour c> inline Bitboard rear_fill(const Bitboard g) {
    if (c == WHITE) {
        return south_fill(g);
    } else {
        return north_fill(g);
    }
}

inline Bitboard pawn_attacks(const Colour c, const Square &s) { return PawnAttacks[c][s]; }

template <Colour c> inline Bitboard pawn_attacks(const Bitboard g) {
    if (c == WHITE) {
        return shift<Direction::NW>(g) | shift<Direction::NE>(g);
    } else {
        return shift<Direction::SW>(g) | shift<Direction::SE>(g);
    }
}

inline Bitboard flip_vertical(const Bitboard g) {
    // clang-format off
    return   (g << 56)           |
            ((g << 40) & rank_7) | 
            ((g << 24) & rank_6) | 
            ((g <<  8) & rank_5) | 
            ((g >>  8) & rank_4) |
            ((g >> 24) & rank_3) | 
            ((g >> 40) & rank_2) | 
             (g >> 56);
    // clang-format on
}
} // namespace Bitboards

/// lsb() and msb() return the least/most significant bit in a non-zero bitboard

inline Square lsb(const Bitboard b) { return Square(std::countr_zero(b)); }

inline Square msb(const Bitboard b) { return Square(std::countl_zero(b)); }

inline int count_bits(const Bitboard b) { return std::popcount(b); }

/// pop_lsb() finds and clears the least significant bit in a non-zero bitboard

inline Square pop_lsb(Bitboard *b) {
    const Square s = lsb(*b);
    *b &= *b - 1;
    return s;
}

/// frontmost_sq() returns the most advanced square for the given color,
/// requires a non-zero bitboard.
inline Square frontmost_sq(const Colour c, const Bitboard b) { return c == WHITE ? msb(b) : lsb(b); }
