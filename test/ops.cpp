#include "board.hpp"
#include <gtest/gtest.h>

TEST(Operations, PassedPawns) {
  Board board = Board();
  board.fen_decode("8/2p5/Pp2k3/8/8/8/1K1P2P1/8 w - - 0 1");
  EXPECT_EQ(board.passed_pawns(WHITE), 0x40000000010000);
  EXPECT_EQ(board.passed_pawns(BLACK), 0x20000);
}

TEST(Operations, InsufficientMaterial) {
  Board board = Board();
  board.fen_decode("8/2k5/8/8/8/4K3/8/8 w - - 0 1");
  EXPECT_TRUE(board.is_draw());
  board.fen_decode("8/5k2/8/2K5/8/8/8/8 b - - 0 1");
  EXPECT_TRUE(board.is_draw());
  board.fen_decode("7K/8/8/8/8/8/8/k7 w - - 0 1");
  EXPECT_TRUE(board.is_draw());
  board.fen_decode("7K/8/8/8/8/8/2n5/k7 w - - 0 1");
  EXPECT_TRUE(board.is_draw());
  board.fen_decode("7K/8/8/8/8/8/2n5/k7 b - - 0 1");
  EXPECT_TRUE(board.is_draw());
  board.fen_decode("7K/8/8/8/8/8/2N5/k7 w - - 0 1");
  EXPECT_TRUE(board.is_draw());
  board.fen_decode("7K/8/8/8/8/8/2N5/k7 b - - 0 1");
  EXPECT_TRUE(board.is_draw());
  board.fen_decode("7K/8/2B5/8/8/8/8/k7 w - - 0 1");
  EXPECT_TRUE(board.is_draw());
  board.fen_decode("7K/8/2B5/8/8/8/8/k7 b - - 0 1");
  EXPECT_TRUE(board.is_draw());
  board.fen_decode("7K/8/2B5/8/5b2/8/8/k7 w - - 0 1");
  EXPECT_TRUE(board.is_draw());
  board.fen_decode("7K/8/2B5/8/5b2/8/8/k7 b - - 0 1");
  EXPECT_TRUE(board.is_draw());
  board.fen_decode("7K/8/2BB4/8/8/8/8/k7 w - - 0 1");
  EXPECT_FALSE(board.is_draw());
  board.fen_decode("7K/8/2BB4/8/8/8/8/k7 b - - 0 1");
  EXPECT_FALSE(board.is_draw());
  board.fen_decode("7K/8/8/8/2B5/p7/8/k7 w - - 0 1");
  EXPECT_FALSE(board.is_draw());
  board.fen_decode("7K/8/8/8/2B5/p7/8/k7 b - - 0 1");
  EXPECT_FALSE(board.is_draw());
}

TEST(Operations, RelativeFills) {
  Bitboard testcases[] = {
      0xFFFFFFFFFFFFFFFF, 0x0000000000000000, 0x0101010101010101,
      0xff00000000000000, 0x00000000000000ff, 0x0100000000000000,
      0x0000000000000001, 0x0000000001000000, 0x0000000000020400};

  for (const auto &test : testcases) {
    EXPECT_EQ(Bitboards::north_fill(test),
              Bitboards::forward_fill(WHITE, test));
    EXPECT_EQ(Bitboards::south_fill(test),
              Bitboards::forward_fill(BLACK, test));
    EXPECT_EQ(Bitboards::south_fill(test), Bitboards::rear_fill(WHITE, test));
    EXPECT_EQ(Bitboards::north_fill(test), Bitboards::rear_fill(BLACK, test));
  }
}

TEST(Operations, NorthFill) {
  std::pair<Bitboard, Bitboard> testcases[] = {
      {0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF},
      {0x0000000000000000, 0x0000000000000000},
      {0x0101010101010101, 0x0101010101010101},
      {0xff00000000000000, 0xFFFFFFFFFFFFFFFF},
      {0x00000000000000ff, 0x00000000000000ff},
      {0x0100000000000000, 0x0101010101010101},
      {0x0000000000000001, 0x0000000000000001},
      {0x0000000001000000, 0x0000000001010101},
      {0x0000000000020400, 0x0000000000020606},

  };

  for (const auto &[in, out] : testcases) {
    EXPECT_EQ(Bitboards::north_fill(in), out);
  }
}

TEST(Operations, SouthFill) {
  std::pair<Bitboard, Bitboard> testcases[] = {
      {0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF},
      {0x0000000000000000, 0x0000000000000000},
      {0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF},
      {0x0000000000000000, 0x0000000000000000},
      {0x0101010101010101, 0x0101010101010101},
      {0xff00000000000000, 0xff00000000000000},
      {0x00000000000000ff, 0xFFFFFFFFFFFFFFFF},
      {0x0100000000000000, 0x0100000000000000},
      {0x0000000000000001, 0x0101010101010101},
      {0x0000000001000000, 0x0101010101000000},
      {0x0000000000020400, 0x606060606060400},
  };
  for (const auto &[in, out] : testcases) {
    EXPECT_EQ(Bitboards::south_fill(in), out);
  }
}