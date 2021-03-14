#pragma once
#include "types.hpp"
#include "piece.hpp"

typedef unsigned long int Bitboard;


inline Bitboard PseudolegalAttacks[N_PIECE][N_SQUARE];

inline Bitboard sq_to_bb(const int s) {
    return Bitboard(1) << s;
}
inline Bitboard sq_to_bb(const Square s) {
    return Bitboard(1) << s.get_value();
}

inline Bitboard  operator&( Bitboard  b, Square s) { return b &  sq_to_bb(s); }
inline Bitboard  operator|( Bitboard  b, Square s) { return b |  sq_to_bb(s); }
inline Bitboard  operator^( Bitboard  b, Square s) { return b ^  sq_to_bb(s); }
inline Bitboard& operator|=(Bitboard& b, Square s) { return b |= sq_to_bb(s); }
inline Bitboard& operator^=(Bitboard& b, Square s) { return b ^= sq_to_bb(s); }

inline Bitboard attacks(const PieceEnum p, const Square s) {
    // Get the value from the pseudolegal attacks table
    return PseudolegalAttacks[p][s];
}

namespace Bitboards
{
    void pretty(Bitboard);
    void init();
} // namespace 


// Following bit magic shamelessly stolen from Stockfish

/// lsb() and msb() return the least/most significant bit in a non-zero bitboard

#if defined(__GNUC__)  // GCC, Clang, ICC

inline Square lsb(Bitboard b) {
  return Square(__builtin_ctzll(b));
}

inline Square msb(Bitboard b) {
  return Square(63 ^ __builtin_clzll(b));
}

#else  // Compiler is neither GCC nor MSVC compatible

#error "Compiler not supported."

#endif


/// pop_lsb() finds and clears the least significant bit in a non-zero bitboard

inline Square pop_lsb(Bitboard* b) {
  const Square s = lsb(*b);
  *b &= *b - 1;
  return s;
}


/// frontmost_sq() returns the most advanced square for the given color,
/// requires a non-zero bitboard.
inline Square frontmost_sq(Colour c, Bitboard b) {
  return c == WHITE ? msb(b) : lsb(b);
}
