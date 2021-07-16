#include "search.hpp"
#include "board.hpp"
#include "evaluate.hpp"
#include <gtest/gtest.h>

TEST(Search, MateInTwo) {
  std::pair<std::string, score_t> testcases[] = {
      {"r2q1b1r/1pN1n1pp/p1n3k1/4Pb2/2BP4/8/PPP3PP/R1BQ1RK1 w - - 1 0",
       MATING_SCORE - 3},
      {"4kb1r/p2n1ppp/4q3/4p1B1/4P3/1Q6/PPP2PPP/2KR4 w k - 1 0",
       MATING_SCORE - 3},
      {"r1b2k1r/ppp1bppp/8/1B1Q4/5q2/2P5/PPP2PPP/R3R1K1 w - - 1 0",
       MATING_SCORE - 3},
      {"5rkr/pp2Rp2/1b1p1Pb1/3P2Q1/2n3P1/2p5/P4P2/4R1K1 w - - 1 0",
       MATING_SCORE - 3},
      {"1r1kr3/Nbppn1pp/1b6/8/6Q1/3B1P2/Pq3P1P/3RR1K1 w - - 1 0",
       MATING_SCORE - 3},
      {"1rb4r/pkPp3p/1b1P3n/1Q6/N3Pp2/8/P1P3PP/7K w - - 1 0", MATING_SCORE - 3},
  };
  Board board = Board();
  constexpr depth_t depth = 8;
  PrincipleLine line;
  line.reserve(depth);

  for (const auto &[fen, score] : testcases) {
    board.fen_decode(fen);
    EXPECT_EQ(Search::search(board, depth, line), score);
  }
}

TEST(Search, MateInThree) {
  std::pair<std::string, score_t> testcases[] = {
      {"r1b1kb1r/pppp1ppp/5q2/4n3/3KP3/2N3PN/PPP4P/R1BQ1B1R b kq - 0 1",
       MATING_SCORE - 5},
      {"4N1nk/p5R1/4b2p/3pPp1Q/2pB1P1K/2P3PP/7r/2q5 w - - 1 0",
       MATING_SCORE - 5},
      {"r6r/pp1Q2pp/2p4k/4R3/5P2/2q5/P1P3PP/R5K1 w - - 1 0", MATING_SCORE - 5},
      {"4r1k1/5bpp/2p5/3pr3/8/1B3pPq/PPR2P2/2R2QK1 b - - 0 1",
       MATING_SCORE - 5},
      {"r3kr2/6Qp/1Pb2p2/pB3R2/3pq2B/4n3/1P4PP/4R1K1 w - - 1 0",
       MATING_SCORE - 5},
      {"r5rk/ppq2p2/2pb1P1B/3n4/3P4/2PB3P/PP1QNP2/1K6 w - - 1 0",
       MATING_SCORE - 5},
  };
  Board board = Board();
  constexpr depth_t depth = 10;
  PrincipleLine line;
  line.reserve(depth);

  for (const auto &[fen, score] : testcases) {
    board.fen_decode(fen);
    EXPECT_EQ(Search::search(board, depth, line), score);
  }
}

TEST(Search, MateInFour) {
  std::pair<std::string, score_t> testcases[] = {
      {"2r2b2/p2q1P1p/3p2k1/4pNP1/4P1RQ/7K/2pr4/5R2 w - - 1 0",
       MATING_SCORE - 7},
      {"4r3/p4pkp/q7/3Bbb2/P2P1ppP/2N3n1/1PP2KPR/R1BQ4 b - - 0 1",
       MATING_SCORE - 7},
  };
  Board board = Board();
  constexpr depth_t depth = 10;
  PrincipleLine line;
  line.reserve(depth);

  for (const auto &[fen, score] : testcases) {
    board.fen_decode(fen);
    EXPECT_EQ(Search::search(board, depth, line), score);
  }
}

TEST(Search, 50MoveRule) {
  Board board = Board();
  std::pair<std::string, score_t> testcases[] = {
      {"7k/8/R7/1R6/7K/8/7P/8 w - - 99 1", MATING_SCORE - 5},
      {"8/7p/8/7k/1r6/r7/8/7K b - - 99 1", MATING_SCORE - 5},
      {"8/8/8/P7/8/6n1/3R4/R3K2k w Q - 99 1", MATING_SCORE - 7},
      {"r3k2K/3r4/6N1/8/p7/8/8/8 b q - 99 1", MATING_SCORE - 7},
  };

  constexpr depth_t depth = 10;
  PrincipleLine line;
  line.reserve(depth);

  for (const auto &[fen, score] : testcases) {
    board.fen_decode(fen);
    EXPECT_EQ(Search::search(board, depth, line), score);
  }
}

TEST(Search, InsufficientMaterial) {
  Board board = Board();
  std::string fens[] = {
      "8/2k5/8/8/8/4K3/8/8 w - - 0 1",   "8/5k2/8/2K5/8/8/8/8 b - - 0 1",
      "7K/8/8/8/8/8/8/k7 w - - 0 1",     "7K/8/8/8/8/8/2n5/k7 w - - 0 1",
      "7K/8/8/8/8/8/2n5/k7 b - - 0 1",   "7K/8/8/8/8/8/2N5/k7 b - - 0 1",
      "7K/8/2B5/8/8/8/8/k7 w - - 0 1",   "7K/8/2B5/8/8/8/8/k7 b - - 0 1",
      "7K/8/2B5/8/5b2/8/8/k7 w - - 0 1", "7K/8/2B5/8/5b2/8/8/k7 b - - 0 1",
  };

  constexpr depth_t depth = 10;
  PrincipleLine line;
  line.reserve(depth);

  for (const std::string fen : fens) {
    board.fen_decode(fen);
    EXPECT_EQ(Search::search(board, depth, line),
              Evaluation::drawn_score(board));
  }
}

TEST(Search, Stalemate) {
  Board board = Board();
  board.set_root();
  const score_t draw_score = Evaluation::drawn_score(board);
  std::pair<std::string, score_t> testcases[] = {
      {"kb4r1/p7/8/8/8/6q1/8/R6K w - - 0 1", draw_score},
      {"r6k/8/6Q1/8/8/8/P7/KB4R1 b - - 0 1", draw_score},
      {"8/8/8/8/8/8/p7/k1K5 w - - 0 1", draw_score},
      {"K1k5/P7/8/8/8/8/8/8 b - - 0 1", draw_score},
      {"K1k5/P1q5/8/B7/8/8/8/8 w - - 0 1", draw_score},
      {"8/8/8/8/b7/8/p1Q5/k1K5 b - - 0 1", draw_score},
  };

  constexpr depth_t depth = 10;
  PrincipleLine line;
  line.reserve(depth);

  for (const auto &[fen, score] : testcases) {
    board.fen_decode(fen);
    EXPECT_EQ(Search::search(board, depth, line), score);
  }
}

TEST(Search, Underpromotion) {
  Board board = Board();
  board.set_root();
  const score_t draw_score = Evaluation::drawn_score(board);
  std::pair<std::string, score_t> testcases[] = {
      {"6n1/5P1k/5Q2/8/8/8/8/7K w - - 0 1", MATING_SCORE - 1},
      {"7k/8/8/8/8/5q2/5p1K/6N1 b - - 0 1", MATING_SCORE - 1},
  };

  constexpr depth_t depth = 10;
  PrincipleLine line;
  line.reserve(depth);

  for (const auto &[fen, score] : testcases) {
    board.fen_decode(fen);
    EXPECT_EQ(Search::search(board, depth, line), score);
  }
}

TEST(Search, Rule50CheckmatePriority) {
  Board board = Board();
  board.set_root();
  const score_t draw_score = Evaluation::drawn_score(board);
  std::pair<std::string, score_t> testcases[] = {
      {"7k/1R6/R7/8/8/8/8/4K3 w - - 99 1", MATING_SCORE - 1},
      {"4k3/8/8/8/8/r7/1r6/7K b - - 99 1", MATING_SCORE - 1},
  };

  constexpr depth_t depth = 10;
  PrincipleLine line;
  line.reserve(depth);

  for (const auto &[fen, score] : testcases) {
    board.fen_decode(fen);
    EXPECT_EQ(Search::search(board, depth, line), score);
  }
}