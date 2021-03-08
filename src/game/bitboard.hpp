typedef unsigned long int Bitboard;

inline Bitboard sq_to_bb(const int s) {
    return Bitboard(1) << s;
}