#include "ordering.hpp"
#include "board.hpp"
#include "evaluate.hpp"
#include <gtest/gtest.h>
#include <tuple>

TEST(Ordering, SmallestAttacker) {
  std::tuple<std::string, Square, Colour, PieceType> testcases[] = {
      {"1k1r4/1pp4p/p7/4p3/8/P5P1/1PP4P/2K1R3 w - - ",
       Squares::FileE | Squares::Rank5, WHITE, ROOK},
      {"1k1r4/1pp4p/p7/4p3/8/P5P1/1PP4P/2K1R3 w - - ",
       Squares::FileE | Squares::Rank5, BLACK, NO_PIECE},
      {"1k1r4/1pp4p/p7/4p3/8/P5P1/1PP4P/2K1R3 b - - ",
       Squares::FileE | Squares::Rank5, WHITE, ROOK},
      {"1k1r4/1pp4p/p7/4p3/8/P5P1/1PP4P/2K1R3 b - - ",
       Squares::FileE | Squares::Rank5, BLACK, NO_PIECE},
      {"1k1r3q/1ppn3p/p4b2/4p3/8/P2N2P1/1PP1R1BP/2K1Q3 w - - ",
       Squares::FileE | Squares::Rank5, WHITE, KNIGHT},
      {"1k1r3q/1ppn3p/p4b2/4p3/8/P2N2P1/1PP1R1BP/2K1Q3 w - - ",
       Squares::FileE | Squares::Rank5, BLACK, KNIGHT},
      {"1k1r3q/1ppn3p/p4b2/4p3/8/P2N2P1/1PP1R1BP/2K1Q3 b - - ",
       Squares::FileE | Squares::Rank5, WHITE, KNIGHT},
      {"1k1r3q/1ppn3p/p4b2/4p3/8/P2N2P1/1PP1R1BP/2K1Q3 b - - ",
       Squares::FileE | Squares::Rank5, BLACK, KNIGHT},
  };
  Board board = Board();

  for (const auto &[fen, sq, side, piece] : testcases) {
    board.fen_decode(fen);
    const Bitboard atk =
        Ordering::get_smallest_attacker(board, sq, Bitboards::omega, side);
    const Square atksq = lsb(atk);
    EXPECT_LE(count_bits(atk), 1);
    EXPECT_EQ(board.piece_type(atksq), piece);
    EXPECT_EQ(Ordering::get_smallest_attacker(board, sq, Bitboards::null, side),
              Bitboards::null);
  }
}

TEST(Ordering, SEE) {
  std::tuple<std::string, Square, Colour, score_t> testcases[] = {
      {"1k1r4/1pp4p/p7/4p3/8/P5P1/1PP4P/2K1R3 w - - ",
       Squares::FileE | Squares::Rank5, WHITE,
       Evaluation::piece_material(PAWN)},
      {"1k1r3q/1ppn3p/p4b2/4p3/8/P2N2P1/1PP1R1BP/2K1Q3 w - -",
       Squares::FileE | Squares::Rank5, WHITE, 0},
  };
  Board board = Board();

  for (const auto &[fen, sq, side, score] : testcases) {
    board.fen_decode(fen);
    PieceType pt = board.piece_type(sq);
    EXPECT_EQ(Ordering::see(board, sq, side, pt, Bitboards::omega), score);
  }
}

TEST(Ordering, SeeCapture) {
  std::tuple<std::string, std::string, score_t> testcases[] = {
      {"1k1r4/1pp4p/p7/4p3/8/P5P1/1PP4P/2K1R3 w - - ", "e1e5",
       Evaluation::piece_material(PAWN)},
      {"1k1r3q/1ppn3p/p4b2/4p3/8/P2N2P1/1PP1R1BP/2K1Q3 w - -", "d3e5",
       Evaluation::piece_material(PAWN) - Evaluation::piece_material(KNIGHT)},
      {"rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2", "e4d5",
       0},
      {"rnbqkbnr/ppp1pppp/8/3p4/8/2N5/PPPPPPPP/R1BQKBNR w KQkq - 0 2", "c3d5",
       Evaluation::piece_material(PAWN) - Evaluation::piece_material(KNIGHT)},
      {"2q1k3/8/8/2P5/1P6/8/8/4K3 b - - 0 1", "c8c5",
       Evaluation::piece_material(PAWN) - Evaluation::piece_material(QUEEN)},
      {"2q1k3/2r5/8/2N5/1B6/8/8/4K3 b - - 0 1", "c7c5",
       Evaluation::piece_material(KNIGHT) + Evaluation::piece_material(BISHOP) -
           Evaluation::piece_material(ROOK)},
      {"2q1k3/8/3p4/2N5/1B6/8/8/4K3 b - - 0 1", "d6c5",
       Evaluation::piece_material(KNIGHT)},
      {"4k3/8/8/8/4Pp2/8/8/4K3 b - e3 0 1", "f4e3",
       Evaluation::piece_material(PAWN)},
  };
  Board board = Board();

  for (const auto &[fen, movestring, score] : testcases) {
    board.fen_decode(fen);
    Move move = board.fetch_move(movestring);
    EXPECT_EQ(Ordering::see_capture(board, move), score);
  }
}

TEST(Ordering, SeeQuiet) {
  std::tuple<std::string, std::string, score_t> testcases[] = {
      {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "e2e4", 0},
      {"4k3/8/8/8/3Q4/8/8/4K3 w - - 0 1", "d4d7",
       -Evaluation::piece_material(QUEEN)},
      {"4k3/8/8/2q5/8/2PP4/8/4K3 w - - 0 1", "d3d4", 0},
      {"4k3/8/4n3/2q5/8/2QP4/8/4K3 w - - 0 1", "d3d4",
       -Evaluation::piece_material(PAWN)},
      {"4k3/8/4n3/2b5/8/2QP4/8/4K3 w - - 0 1", "d3d4",
       -Evaluation::piece_material(PAWN)},
  };
  Board board = Board();

  for (const auto &[fen, movestring, score] : testcases) {
    board.fen_decode(fen);
    Move move = board.fetch_move(movestring);
    EXPECT_EQ(Ordering::see_quiet(board, move), score);
  }
}