#include "board.hpp"
#include "evaluate.hpp"
#include <gtest/gtest.h>

TEST(Eval, Contempt) {
  // This test just looks at a bunch of cases and makes sure the contempt score
  // alternates with the player.
  std::string fens[] = {
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 ",
      "rnbqk2r/ppp1ppbp/3p1np1/8/2PPP3/2N2N2/PP3PPP/R1BQKB1R b KQkq - 0 5 ",
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 ",
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w K - 0 1 ",
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w Q - 0 1 ",
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w k - 0 1 ",
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b q - 0 1 ",
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b - - 0 1 ",
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b Kk - 0 1 ",
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b Qq - 0 1 ",
      "2rq1rk1/pp1bppbp/2np1np1/8/3NP3/1BN1BP2/PPPQ2PP/2KR3R b - - 8 11 ",
      "2rqr1k1/pp1bppbp/3p1np1/4n3/3NP2P/1BN1BP2/PPPQ2P1/1K1R3R b - - 0 13 ",
      "rnbqkbnr/ppp1pppp/8/8/3pP3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 3 "};

  Board board = Board();
  for (std::string board_fen : fens) {
    board.fen_decode(board_fen);
    board.set_root();
    EXPECT_EQ(Evaluation::drawn_score(board), -Evaluation::contempt);
    board.make_nullmove();
    EXPECT_EQ(Evaluation::drawn_score(board), Evaluation::contempt);
    board.make_nullmove();
    EXPECT_EQ(Evaluation::drawn_score(board), -Evaluation::contempt);
    board.make_nullmove();
    EXPECT_EQ(Evaluation::drawn_score(board), Evaluation::contempt);
    board.make_nullmove();
    EXPECT_EQ(Evaluation::drawn_score(board), -Evaluation::contempt);
    board.unmake_nullmove();
    EXPECT_EQ(Evaluation::drawn_score(board), Evaluation::contempt);
    board.unmake_nullmove();
    EXPECT_EQ(Evaluation::drawn_score(board), -Evaluation::contempt);
    board.unmake_nullmove();
    EXPECT_EQ(Evaluation::drawn_score(board), Evaluation::contempt);
    board.unmake_nullmove();
    EXPECT_EQ(Evaluation::drawn_score(board), -Evaluation::contempt);
  }
}

TEST(Eval, Symmetry) {
  // Make sure the evaluation is symmetric
  std::string fens[] = {
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 ",
      "rnbqk2r/ppp1ppbp/3p1np1/8/2PPP3/2N2N2/PP3PPP/R1BQKB1R b KQkq - 0 5 ",
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 ",
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w K - 0 1 ",
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w Q - 0 1 ",
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w k - 0 1 ",
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b q - 0 1 ",
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b - - 0 1 ",
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b Kk - 0 1 ",
      "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b Qq - 0 1 ",
      "2rq1rk1/pp1bppbp/2np1np1/8/3NP3/1BN1BP2/PPPQ2PP/2KR3R b - - 8 11 ",
      "2rqr1k1/pp1bppbp/3p1np1/4n3/3NP2P/1BN1BP2/PPPQ2P1/1K1R3R b - - 0 13 ",
      "rnbqkbnr/ppp1pppp/8/8/3pP3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 3 ",
      "3R4/1p4pp/4k3/2B2b2/1P6/b7/5PPP/6K1 w - - 2 25",
  };

  Board board = Board();
  for (std::string board_fen : fens) {
    board.fen_decode(board_fen);
    const score_t score1 = Evaluation::evaluate_white(board);
    const score_t rscore1 = Evaluation::eval(board);
    board.flip();
    EXPECT_EQ(score1, -Evaluation::evaluate_white(board));
    EXPECT_EQ(rscore1, Evaluation::eval(board));
  }
}