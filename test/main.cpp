#include "bitboard.hpp"
#include "board.hpp"
#include "evaluate.hpp"
#include "search.hpp"
#include "transposition.hpp"
#include "zobrist.hpp"
#include <gtest/gtest.h>

int main(int argc, char **argv) {
  ::Bitboards::init();
  ::Cache::init();
  ::Evaluation::init();
  ::Search::init();
  ::testing::InitGoogleTest(&argc, argv);
  ::Zobrist::init();

  int ret = RUN_ALL_TESTS();
  return ret;
}