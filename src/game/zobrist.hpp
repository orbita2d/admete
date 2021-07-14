#include "board.hpp"
// zorbist.cpp
namespace Zobrist {
void init();
// Compute the entire zobrist hash for the position.
zobrist_t hash(const Board &board);
// Compute the pawn part of the zobrist hash for the position, just for caching pawn structure evals.
zobrist_t pawns(const Board &board);
// Compute a material key for the position.
zobrist_t material(const Board &board);
// Compute the change in zobrist hash (bitwise) caused by a move.
zobrist_t diff(const Move move, const Colour us, const File last_ep_file, const unsigned castling_rights_change);
// Compute the change in zobrist hash (bitwise) caused by a null move.
zobrist_t nulldiff(const Colour us, const int last_ep_file);

inline zobrist_t zobrist_table[N_COLOUR][N_PIECE][N_SQUARE];
inline zobrist_t zobrist_table_cr[N_COLOUR][2];
inline zobrist_t zobrist_table_move[N_COLOUR];
inline zobrist_t zobrist_table_ep[8];

} // namespace Zobrist