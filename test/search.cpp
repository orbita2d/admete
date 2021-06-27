#include <gtest/gtest.h>
#include "search.hpp"
#include "board.hpp"


TEST(ForcedMateTest, MateInTwo) {
    Board board = Board();
    std::vector<Move> line;
    uint depth = 4;
    line.reserve(depth);
    board.fen_decode("r2q1b1r/1pN1n1pp/p1n3k1/4Pb2/2BP4/8/PPP3PP/R1BQ1RK1 w - - 1 0");
    EXPECT_EQ(iterative_deepening(board, depth, line),  MATING_SCORE - 3);
    board.fen_decode("1rb4r/pkPp3p/1b1P3n/1Q6/N3Pp2/8/P1P3PP/7K w - - 1 0");
    line.clear();
    EXPECT_EQ(iterative_deepening(board, depth, line),  MATING_SCORE - 3);
    board.fen_decode("4kb1r/p2n1ppp/4q3/4p1B1/4P3/1Q6/PPP2PPP/2KR4 w k - 1 0");
    line.clear();
    EXPECT_EQ(iterative_deepening(board, depth, line),  MATING_SCORE - 3);
    board.fen_decode("r1b2k1r/ppp1bppp/8/1B1Q4/5q2/2P5/PPP2PPP/R3R1K1 w - - 1 0");
    line.clear();
    EXPECT_EQ(iterative_deepening(board, depth, line),  MATING_SCORE - 3);
    board.fen_decode("5rkr/pp2Rp2/1b1p1Pb1/3P2Q1/2n3P1/2p5/P4P2/4R1K1 w - - 1 0");
    line.clear();
    EXPECT_EQ(iterative_deepening(board, depth, line),  MATING_SCORE - 3);
    board.fen_decode("1r1kr3/Nbppn1pp/1b6/8/6Q1/3B1P2/Pq3P1P/3RR1K1 w - - 1 0");
    line.clear();
    EXPECT_EQ(iterative_deepening(board, depth, line),  MATING_SCORE - 3);
}

TEST(ForcedMateTest, MateInThree) {
    Board board = Board();
    std::vector<Move> line;
    uint depth = 6;
    line.reserve(depth);
    board.fen_decode("r1b1kb1r/pppp1ppp/5q2/4n3/3KP3/2N3PN/PPP4P/R1BQ1B1R b kq - 0 1");
    EXPECT_EQ(iterative_deepening(board, depth, line),  (MATING_SCORE - 5));
}


TEST(ForcedMateTest, MateInFour) {
    Board board = Board();
    std::vector<Move> line;
    uint depth = 8;
    line.reserve(depth);
    board.fen_decode("2r2b2/p2q1P1p/3p2k1/4pNP1/4P1RQ/7K/2pr4/5R2 w - - 1 0");
    EXPECT_EQ(iterative_deepening(board, depth, line),  +(MATING_SCORE - 7));
}
