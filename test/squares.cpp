#include "types.hpp"
#include <gtest/gtest.h>

TEST(Squares, Endineness) {
  std::pair<Square, int> testcases[] = {
      {Square(RANK1, FILEA), 0},  {Square(RANK1, FILEB), 1},
      {Square(RANK1, FILEH), 7},  {Square(RANK2, FILEA), 8},
      {Square(RANK2, FILEB), 9},  {Square(RANK2, FILEH), 15},
      {Square(RANK8, FILEA), 56}, {Square(RANK8, FILEB), 57},
      {Square(RANK8, FILEH), 63},
  };

  for (const auto &[sq, v] : testcases) {
    EXPECT_EQ(sq.get_value(), v);
  }
}

TEST(Squares, Relative) {
  std::pair<Rank, Rank> testcases[] = {
      {RANK1, RANK8}, {RANK2, RANK7}, {RANK3, RANK6}, {RANK4, RANK5},
      {RANK5, RANK4}, {RANK6, RANK3}, {RANK7, RANK2}, {RANK8, RANK1},
  };
  for (const auto &[rw, rb] : testcases) {
    EXPECT_EQ(relative_rank(WHITE, rw), rw);
    EXPECT_EQ(relative_rank(BLACK, rw), rb);
  }
  EXPECT_EQ(back_rank(WHITE), RANK1);
  EXPECT_EQ(back_rank(BLACK), RANK8);
}