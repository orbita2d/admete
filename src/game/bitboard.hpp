#pragma once
#include "types.hpp"
#include <inttypes.h>

typedef uint64_t Bitboard;

inline Bitboard PseudolegalAttacks[N_PIECE][N_SQUARE];
inline Bitboard PawnAttacks[N_COLOUR][N_SQUARE];
inline Bitboard PawnSpans[N_COLOUR][N_SQUARE];
inline Bitboard LineBBs[N_SQUARE][N_SQUARE];
constexpr Bitboard CastleCheckBBs[N_COLOUR][N_CASTLE] = {{0x60, 0xe}, {0x6000000000000000, 0xe00000000000000}};
inline Bitboard SquareBBs[N_SQUARE];

inline Bitboard sq_to_bb(const int s) {
    // The space-time balance doesn't work out here, cheaper to calculate than look up.
    return Bitboard(1) << s;
}
inline Bitboard sq_to_bb(const Square s) { return Bitboard(1) << s; }

inline Bitboard operator&(Bitboard b, Square s) { return b & sq_to_bb(s); }
inline Bitboard operator|(Bitboard b, Square s) { return b | sq_to_bb(s); }
inline Bitboard operator^(Bitboard b, Square s) { return b ^ sq_to_bb(s); }
inline Bitboard &operator|=(Bitboard &b, Square s) { return b |= sq_to_bb(s); }
inline Bitboard &operator^=(Bitboard &b, Square s) { return b ^= sq_to_bb(s); }

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

inline Bitboard bishop_attacks(Bitboard occ, const Square sq) {
    const Magic magic = BishopMagics[sq];
    Bitboard *aptr = magic.ptr;
    return aptr[magic.index(occ)];
}

inline Bitboard rook_attacks(Bitboard occ, const Square sq) {
    const Magic magic = RookMagics[sq];
    Bitboard *aptr = magic.ptr;
    return aptr[magic.index(occ)];
}

inline Bitboard bishop_xrays(Bitboard occ, const Square sq) {
    const Bitboard atk = bishop_attacks(occ, sq);
    return bishop_attacks(occ ^ (occ & atk), sq);
}

inline Bitboard rook_xrays(Bitboard occ, const Square sq) {
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
constexpr Bitboard castle_king[N_COLOUR][N_CASTLE] = {{0xe0, 0x7}, {0xe000000000000000, 0x700000000000000}};
// Bitboard for king safetly pawns on 2nd rank
constexpr Bitboard castle_pawn2[N_COLOUR][N_CASTLE] = {{0xe000, 0x700}, {0x00e0000000000000, 0x007000000000000}};
// Bitboard for king safetly pawns on 3rd rank
constexpr Bitboard castle_pawn3[N_COLOUR][N_CASTLE] = {{0xe00000, 0x70000}, {0x0000e00000000000, 0x000070000000000}};

constexpr Bitboard dark_squares = 0xAA55AA55AA55AA55;
constexpr Bitboard light_squares = 0x55AA55AA55AA55AA;
constexpr Bitboard bishop_squares[N_BISHOPTYPES] = {light_squares, dark_squares};

template <Direction dir> constexpr Bitboard shift(const Bitboard bb) {
    // Bitboards are stored big-endian but who can remember that, so this hides the implementation a little
    if (dir == Direction::N) {
        return (bb << 8);
    } else if (dir == Direction::S) {
        return (bb >> 8);
    } else if (dir == Direction::E) {
        return (bb << 1) & ~Bitboards::a_file;
    } else if (dir == Direction::W) {
        return (bb >> 1) & ~Bitboards::h_file;
    } else if (dir == Direction::NW) {
        return (bb << 7) & ~Bitboards::h_file;
    } else if (dir == Direction::NE) {
        return (bb << 9) & ~Bitboards::a_file;
    } else if (dir == Direction::SW) {
        return (bb >> 9) & ~Bitboards::h_file;
    } else if (dir == Direction::SE) {
        return (bb >> 7) & ~Bitboards::a_file;
    }
}

inline Bitboard pseudo_attacks(const PieceType p, const Square s) {
    // Get the value from the pseudolegal attacks table
    return PseudolegalAttacks[p][s];
}

inline Bitboard attacks(const PieceType p, const Bitboard occ, const Square sq) {
    // Get the value from the pseudolegal attacks table
    switch (p) {
    case ROOK:
        return rook_attacks(occ, sq);
    case BISHOP:
        return bishop_attacks(occ, sq);
    case QUEEN:
        return bishop_attacks(occ, sq) | rook_attacks(occ, sq);
    default:
        return PseudolegalAttacks[p][sq];
    }
}

template <PieceType p> inline Bitboard attacks(const Bitboard occ, const Square sq) {
    // Get the value from the pseudolegal attacks table
    switch (p) {
    case ROOK:
        return rook_attacks(occ, sq);
    case BISHOP:
        return bishop_attacks(occ, sq);
    case QUEEN:
        return bishop_attacks(occ, sq) | rook_attacks(occ, sq);
    default:
        return PseudolegalAttacks[p][sq];
    }
}

inline Bitboard pawn_attacks(const Colour c, const Square s) { return PawnAttacks[c][s]; }

inline Bitboard rank(const Square s) { return rank_bb[s.rank_index()]; }

inline Bitboard file(const Square s) { return file_bb[s.file_index()]; }
inline Bitboard line(const Square s1, const Square s2) { return LineBBs[s1][s2]; }
inline Bitboard between(const Square s1, const Square s2) {
    Bitboard bb = line(s1, s2);
    if (bb) {
        bb &= ((omega << s1) ^ (omega << s2));
        return bb & (bb - 1);
    } else {
        return 0;
    }
}

constexpr Bitboard castle(const Colour c, const CastlingSide cs) { return CastleCheckBBs[c][cs]; }

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

inline Bitboard north_block_span(Bitboard g) {
    // Squares that a pawn could be to not be passed.
    g = north_fill(g);
    g = shift<Direction::N>(g);
    g |= shift<Direction::W>(g);
    g |= shift<Direction::E>(g);
    return g;
}

inline Bitboard south_block_span(Bitboard g) {
    // Squares that a pawn could be to not be passed.
    g = south_fill(g);
    g = shift<Direction::S>(g);
    g |= shift<Direction::W>(g);
    g |= shift<Direction::E>(g);
    return g;
}

inline Bitboard forward_span(const Colour c, Bitboard g) {
    if (c == WHITE) {
        g = north_fill(g);
        return shift<Direction::N>(g);
    } else {
        g = south_fill(g);
        return shift<Direction::S>(g);
    }
}

inline Bitboard rear_span(const Colour c, Bitboard g) {
    if (c == WHITE) {
        g = south_fill(g);
        return shift<Direction::S>(g);
    } else {
        g = north_fill(g);
        return shift<Direction::N>(g);
    }
}

inline Bitboard pawn_push(const Colour c, Bitboard g) {
    if (c == WHITE) {
        g = shift<Direction::N>(g);
        g |= shift<Direction::N>(g & rank_3);
        return g;
    } else {
        g = shift<Direction::S>(g);
        g |= shift<Direction::S>(g & rank_6);
        return g;
    }
}

inline Bitboard reverse_pawn_push(const Colour c, Bitboard g) {
    if (c == WHITE) {
        g = shift<Direction::S>(g);
        g |= shift<Direction::S>(g & rank_3);
        return g;
    } else {
        g = shift<Direction::N>(g);
        g |= shift<Direction::N>(g & rank_6);
        return g;
    }
}

inline Bitboard forward_block_span(const Colour c, Bitboard g) {
    if (c == WHITE) {
        return north_block_span(g);
    } else {
        return south_block_span(g);
    }
}

inline Bitboard rear_block_span(const Colour c, Bitboard g) {
    if (c == WHITE) {
        return south_block_span(g);
    } else {
        return north_block_span(g);
    }
}

inline Bitboard forward_atk_span(const Colour c, Bitboard g) {
    g = forward_span(c, g);
    return shift<Direction::W>(g) | shift<Direction::E>(g);
}

inline Bitboard rear_atk_span(const Colour c, Bitboard g) {
    g = rear_span(c, g);
    return shift<Direction::W>(g) | shift<Direction::E>(g);
}

inline Bitboard full_atk_span(Bitboard g) {
    Bitboard f = vertical_fill(g);
    return shift<Direction::W>(f) | shift<Direction::E>(f);
}

inline Bitboard forward_fill(const Colour c, Bitboard g) {
    if (c == WHITE) {
        return north_fill(g);
    } else {
        return south_fill(g);
    }
}

inline Bitboard rear_fill(const Colour c, Bitboard g) {
    if (c == WHITE) {
        return south_fill(g);
    } else {
        return north_fill(g);
    }
}

inline Bitboard pawn_attacks(const Colour c, Bitboard g) {
    if (c == WHITE) {
        Bitboard atk = shift<Direction::NW>(g) | shift<Direction::NE>(g);
        return atk;
    } else {
        Bitboard atk = shift<Direction::SW>(g) | shift<Direction::SE>(g);
        return atk;
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

#if defined(__GNUC__) // GCC, Clang, ICC

inline Square lsb(Bitboard b) { return Square(__builtin_ctzll(b)); }

inline Square msb(Bitboard b) { return Square(63 ^ __builtin_clzll(b)); }

inline int count_bits(Bitboard bb) { return __builtin_popcountl(bb); }

#elif defined(_MSC_VER) // MSVC

#ifdef _WIN64 // MSVC, WIN64

inline Square lsb(Bitboard b) {
    assert(b);
    unsigned long idx;
    _BitScanForward64(&idx, b);
    return (Square)idx;
}

inline Square msb(Bitboard b) {
    assert(b);
    unsigned long idx;
    _BitScanReverse64(&idx, b);
    return (Square)idx;
}

inline int count_bits(Bitboard bb) { return __popcount64(bb); }

#else // MSVC, WIN32

#error "Will only compile in 64-bit mode"

#endif

#else // Compiler is neither GCC nor MSVC compatible

#error "Compiler not supported."

#endif

/// pop_lsb() finds and clears the least significant bit in a non-zero bitboard

inline Square pop_lsb(Bitboard *b) {
    const Square s = lsb(*b);
    *b &= *b - 1;
    return s;
}

/// frontmost_sq() returns the most advanced square for the given color,
/// requires a non-zero bitboard.
inline Square frontmost_sq(Colour c, Bitboard b) { return c == WHITE ? msb(b) : lsb(b); }
