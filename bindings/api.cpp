#include "api.h"
#include "board.hpp"
#include "search.hpp"
#include <string>
#include <cstddef>
#include <array>
#include <zobrist.hpp>
#include <transposition.hpp>

extern "C" {
int init() {
  Bitboards::init();
  Cache::init();
  Search::init();
  Zobrist::init();

  return 0; // Success
}

int encode_features(char *fen, char *buffer, unsigned int buffer_size, char* white_to_play, int quiece) {
  if (buffer_size < N_SQUARE) {
    return 1; // Error: buffer too small
  }
  if (fen == nullptr || buffer == nullptr) {
    return 2; // Error: null pointer
  }
  const std::string fen_string(fen);
  auto board = Board();
  board.fen_decode(fen_string);
  if (quiece) {
      DenseBoard pos = Search::board_quiesce(board);
      board.unpack(pos);
  }
  // Because these things are colourblind, we need to know who's turn it is *after* the quiescence search.
  // For training, the the eval used as a label should be from the same perspective as the features.
  *white_to_play = board.who_to_play() == WHITE ? 1 : 0;
 auto dense_board = board.byte_encoded();
  for (size_t i = 0; i < N_SQUARE; ++i) {
    buffer[i] = dense_board[i];
  }
  return 0; // Success
}

} // extern "C"