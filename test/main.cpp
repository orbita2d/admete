#include <gtest/gtest.h>
#include "bitboard.hpp"

int main(int argc, char **argv)
{
    ::Bitboards::init();
    ::testing::InitGoogleTest(&argc, argv);
    int ret = RUN_ALL_TESTS();
    return ret;
}