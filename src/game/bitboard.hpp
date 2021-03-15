#pragma once
#include "types.hpp"
#include "piece.hpp"

typedef unsigned long int Bitboard;

inline Bitboard PseudolegalAttacks[N_PIECE][N_SQUARE];
// Pawn attacks vary by colour
inline Bitboard PawnAttacks[N_COLOUR][N_SQUARE];

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


namespace Bitboards
{
    void pretty(Bitboard);
    void init();
    constexpr Bitboard h_file_bb = 0x0101010101010101;
    constexpr Bitboard a_file_bb = 0x8080808080808080;

  template<Direction dir>
  constexpr Bitboard shift(const Bitboard bb) {
    // Bitboards are stored big-endian but who can remember that, so this hides the implementation a little
    if      (dir == Direction::N ) { return (bb >> 8);}
    else if (dir == Direction::S ) { return (bb << 8);}
    else if (dir == Direction::E ) { return (bb << 1) & ~Bitboards::h_file_bb;}
    else if (dir == Direction::W ) { return (bb >> 1) & ~Bitboards::a_file_bb;}
    else if (dir == Direction::NW) { return (bb >> 9) & ~Bitboards::a_file_bb;}
    else if (dir == Direction::NE) { return (bb >> 7) & ~Bitboards::h_file_bb;}
    else if (dir == Direction::SW) { return (bb << 7) & ~Bitboards::a_file_bb;}
    else if (dir == Direction::SE) { return (bb << 9) & ~Bitboards::h_file_bb;}
  }

  inline Bitboard attacks(const PieceEnum p, const Square s) {
      // Get the value from the pseudolegal attacks table
      return PseudolegalAttacks[p][s];
  }

  inline Bitboard pawn_attacks(const Colour c, const Square s) {
      return PawnAttacks[c][s];
  }
}


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
