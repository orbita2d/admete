#include <gtest/gtest.h>
#include "search.hpp"
#include "board.hpp"


TEST(ForcedMateTest, MateInTwo) {
    std::pair<std::string, score_t> testcases[] = { 
        {"r2q1b1r/1pN1n1pp/p1n3k1/4Pb2/2BP4/8/PPP3PP/R1BQ1RK1 w - - 1 0", MATING_SCORE - 3},
        {"4kb1r/p2n1ppp/4q3/4p1B1/4P3/1Q6/PPP2PPP/2KR4 w k - 1 0", MATING_SCORE - 3},
        {"r1b2k1r/ppp1bppp/8/1B1Q4/5q2/2P5/PPP2PPP/R3R1K1 w - - 1 0", MATING_SCORE - 3},
        {"5rkr/pp2Rp2/1b1p1Pb1/3P2Q1/2n3P1/2p5/P4P2/4R1K1 w - - 1 0", MATING_SCORE - 3},
        {"1r1kr3/Nbppn1pp/1b6/8/6Q1/3B1P2/Pq3P1P/3RR1K1 w - - 1 0", MATING_SCORE - 3},
        {"1rb4r/pkPp3p/1b1P3n/1Q6/N3Pp2/8/P1P3PP/7K w - - 1 0", MATING_SCORE - 3},
    };
    Board board = Board();
    PrincipleLine line;
    uint depth = 10;
    line.reserve(depth);

    for (const auto &[fen, score] : testcases) {
        board.fen_decode(fen);
        EXPECT_EQ(iterative_deepening(board, depth, line), score);
    }
}

TEST(ForcedMateTest, MateInThree) {
    std::pair<std::string, score_t> testcases[] = { 
        {"r1b1kb1r/pppp1ppp/5q2/4n3/3KP3/2N3PN/PPP4P/R1BQ1B1R b kq - 0 1", MATING_SCORE - 5},
        {"4N1nk/p5R1/4b2p/3pPp1Q/2pB1P1K/2P3PP/7r/2q5 w - - 1 0", MATING_SCORE - 5},
        {"r6r/pp1Q2pp/2p4k/4R3/5P2/2q5/P1P3PP/R5K1 w - - 1 0", MATING_SCORE - 5},
        {"4r1k1/5bpp/2p5/3pr3/8/1B3pPq/PPR2P2/2R2QK1 b - - 0 1", MATING_SCORE - 5},
        {"r3kr2/6Qp/1Pb2p2/pB3R2/3pq2B/4n3/1P4PP/4R1K1 w - - 1 0", MATING_SCORE - 5},
        {"r5rk/ppq2p2/2pb1P1B/3n4/3P4/2PB3P/PP1QNP2/1K6 w - - 1 0", MATING_SCORE - 5},
    };
    Board board = Board();
    PrincipleLine line;
    uint depth = 10;
    line.reserve(depth);

    for (const auto &[fen, score] : testcases) {
        board.fen_decode(fen);
        EXPECT_EQ(iterative_deepening(board, depth, line), score);
    }
}


TEST(ForcedMateTest, MateInFour) {
    std::pair<std::string, score_t> testcases[] = { 
        {"2r2b2/p2q1P1p/3p2k1/4pNP1/4P1RQ/7K/2pr4/5R2 w - - 1 0", MATING_SCORE - 7},
        {"4r3/p4pkp/q7/3Bbb2/P2P1ppP/2N3n1/1PP2KPR/R1BQ4 b - - 0 1", MATING_SCORE - 7},
    };
    Board board = Board();
    PrincipleLine line;
    uint depth = 10;
    line.reserve(depth);

    for (const auto &[fen, score] : testcases) {
        board.fen_decode(fen);
        EXPECT_EQ(iterative_deepening(board, depth, line), score);
    }
}
