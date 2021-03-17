#include <gtest/gtest.h>
#include "bitboard.hpp"
#include "board.hpp"

int main(int argc, char **argv)
{
    ::Zorbist::init();
    ::Bitboards::init();
    ::testing::InitGoogleTest(&argc, argv);
    int ret = RUN_ALL_TESTS();
    return ret;
}