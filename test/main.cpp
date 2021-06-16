#include <gtest/gtest.h>
#include "bitboard.hpp"
#include "board.hpp"
#include "evaluate.hpp"
#include "transposition.hpp"

int main(int argc, char **argv)
{
    ::Zorbist::init();
    ::Bitboards::init();
    ::Evaluation::init();
    ::Cache::init();
    ::testing::InitGoogleTest(&argc, argv);
    int ret = RUN_ALL_TESTS();
    return ret;
}