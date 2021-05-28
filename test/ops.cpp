#include <gtest/gtest.h>
#include "board.hpp"

TEST(Operations, PassedPawns) {
    Board board = Board();
    board.fen_decode("8/2p5/Pp2k3/8/8/8/1K1P2P1/8 w - - 0 1");
    EXPECT_EQ(board.passed_pawns(WHITE), 0x40000000010000);
    EXPECT_EQ(board.passed_pawns(BLACK), 0x20000);
}

TEST(Bitboards, Fills) {
    EXPECT_EQ(Bitboards::north_fill(0xFFFFFFFFFFFFFFFF), 0xFFFFFFFFFFFFFFFF);
    EXPECT_EQ(Bitboards::south_fill(0xFFFFFFFFFFFFFFFF), 0xFFFFFFFFFFFFFFFF);
    EXPECT_EQ(Bitboards::north_fill(0x0), 0x0);
    EXPECT_EQ(Bitboards::south_fill(0x0), 0x0);

    EXPECT_EQ(Bitboards::north_fill(0x20400), 0x20606);
    EXPECT_EQ(Bitboards::south_fill(0x20400), 0x606060606060400);
    
    EXPECT_EQ(Bitboards::south_fill(0x20400), Bitboards::forward_fill(BLACK, 0x20400));
    EXPECT_EQ(Bitboards::north_fill(0x20400), Bitboards::rear_fill(BLACK, 0x20400));
    EXPECT_EQ(Bitboards::south_fill(0x20400), Bitboards::rear_fill(WHITE, 0x20400));
    EXPECT_EQ(Bitboards::north_fill(0x20400), Bitboards::forward_fill(WHITE, 0x20400));
}