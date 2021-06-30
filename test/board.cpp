#include "board.hpp"
#include <gtest/gtest.h>
#include <tuple>

TEST(Board, NullMoves) {
  std::string fens[] = {
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
      "rnbqk2r/ppp1ppbp/3p1np1/8/2PPP3/2N2N2/PP3PPP/R1BQKB1R b KQkq - 0 5",
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w K - 0 1",
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w Q - 0 1",
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w k - 0 1",
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b q - 0 1",
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b - - 0 1",
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b Kk - 0 1",
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b Qq - 0 1",
      "2rq1rk1/pp1bppbp/2np1np1/8/3NP3/1BN1BP2/PPPQ2PP/2KR3R b - - 8 11",
      "2rqr1k1/pp1bppbp/3p1np1/4n3/3NP2P/1BN1BP2/PPPQ2P1/1K1R3R b - - 0 13",
      "rnbqkbnr/ppp1pppp/8/8/3pP3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 3"};

  Board board = Board();
  for (std::string board_fen : fens) {
    board.fen_decode(board_fen);
    board.make_nullmove();
    board.unmake_nullmove();
    EXPECT_EQ(board.fen_encode(), board_fen);
  }
}

TEST(Board, InsufficientMaterial) {
  Board board = Board();
  std::pair<std::string, bool> testcases[] = {
      {"8/2k5/8/8/8/4K3/8/8 w - - 0 1", true},
      {"8/5k2/8/2K5/8/8/8/8 b - - 0 1", true},
      {"7K/8/8/8/8/8/8/k7 w - - 0 1", true},
      {"7K/8/8/8/8/8/2n5/k7 w - - 0 1", true},
      {"7K/8/8/8/8/8/2n5/k7 b - - 0 1", true},
      {"7K/8/8/8/8/8/2N5/k7 w - - 0 1", true},
      {"7K/8/8/8/8/8/2N5/k7 b - - 0 1", true},
      {"7K/8/2B5/8/8/8/8/k7 w - - 0 1", true},
      {"7K/8/2B5/8/8/8/8/k7 b - - 0 1", true},
      {"7K/8/2B5/8/5b2/8/8/k7 w - - 0 1", true},
      {"7K/8/2B5/8/5b2/8/8/k7 b - - 0 1", true},
      {"7K/8/2BB4/8/8/8/8/k7 w - - 0 1", false},
      {"7K/8/2BB4/8/8/8/8/k7 b - - 0 1", false},
      {"7K/8/8/8/2B5/p7/8/k7 w - - 0 1", false},
      {"7K/8/8/8/2B5/p7/8/k7 b - - 0 1", false},
  };
  for (const auto &[fen, is_draw] : testcases) {
    board.fen_decode(fen);
    board.set_root();
    EXPECT_EQ(board.is_draw(), is_draw);
  }
}

TEST(Board, Flip) {
  Board board = Board();
  std::pair<std::string, std::string> testcases[] = {
      {
          "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
          "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1",
      },
      {
          "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1",
          "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
      },
      {
          "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQ - 0 1",
          "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b kq - 0 1",
      },
      {
          "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w Qk - 0 1",
          "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b Kq - 0 1",
      },
      {"8/5K1p/3B4/8/2n5/8/Pk6/8 w - - 0 1",
       "8/pK6/8/2N5/8/3b4/5k1P/8 b - - 0 1"},
  };
  for (const auto &[in, out] : testcases) {
    board.fen_decode(in);
    board.flip();
    EXPECT_EQ(board.fen_encode(), out);
    board.flip();
    EXPECT_EQ(board.fen_encode(), in);
  }
}

TEST(Board, LastMove) {
  Board board = Board();
  std::tuple<std::string, std::string, std::string> testcases[] = {
      {"r1bqkbnr/pp1p1pp1/7p/n3p3/2B1P3/1QN2N2/PP3PPP/R1B1K2R w KQkq - 2 8",
       "c4f7", "e8e7"},
      {"rn1qkbnr/pp3ppp/2p1p3/3pPb2/3P4/5N2/PPP2PPP/RNBQKB1R w KQkq - 0 5",
       "f1e2", "g8e7"},
  };
  for (const auto &[fen, movestring1, movestring2] : testcases) {
    board.fen_decode(fen);
    EXPECT_EQ(board.last_move(), NULL_MOVE);
    Move move1 = board.fetch_move(movestring1);
    board.make_move(move1);
    EXPECT_EQ(board.last_move(), move1);
    Move move2 = board.fetch_move(movestring2);
    board.make_move(move2);
    EXPECT_EQ(board.last_move(), move2);
    board.unmake_move(move2);
    EXPECT_EQ(board.last_move(), move1);
    board.unmake_move(move1);
    EXPECT_EQ(board.last_move(), NULL_MOVE);
  }
}