#include <gtest/gtest.h>
#include "board.hpp"

class NeuralFeaturesTest : public ::testing::Test {
protected:
    Board board;

    void SetUp() override {
        // Start with empty board
        board = Board();
    }


    std::vector<std::string> test_positions = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1",
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
        "3k4/3p4/8/K1P4r/8/8/8/8 b - - 0 1",
        "8/8/4k3/8/2p5/8/B2P2K1/8 w - - 0 1",
        "8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 0 1",
        "5k2/8/8/8/8/8/8/4K2R w K - 0 1",
        "3k4/8/8/8/8/8/8/R3K3 w Q - 0 1",
        "r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq - 0 1",
        "r3k2r/8/3Q4/8/8/5q2/8/R3K2R b KQkq - 0 1",
        "2rr4/2k2p2/1pn3p1/p3b3/P3P3/1B3P2/1P4P1/2R1K2R w K - 1 24",
        "8/pk6/8/3P4/8/8/2K5/8 b - - 0 1"
    };

    // Helper to verify feature vectors match after move application
    void verify_feature_update(Move& move, const Colour side) {
        // Get initial features
        auto initial_features = Neural::encode(board);
                
        // Apply move
        board.make_move(move);
        auto expected_diff = Neural::increment(move, side, true);
        
        // Get new features
        auto new_features = Neural::encode(board);
        
        // For both colors, verify features match expected values
        for (Colour c : {WHITE, BLACK}) {
          auto diff = expected_diff[c].as_dense();
            for (size_t i = 0; i < Neural::N_FEATURES; i++) {
                // The new features should equal initial features plus the diff
                EXPECT_EQ(new_features[c][i], 
                         initial_features[c][i] + diff[i]) 
                    << "Mismatch at index " << i << " for color " << (c == WHITE ? "WHITE" : "BLACK");
            }
        }
        
        // Verify reverse diff works
        board.unmake_move(move);
        auto reverse_diff = Neural::increment(move, side, false);

        
        // Verify we're back to initial position
        auto final_features = Neural::encode(board);
        for (Colour c : {WHITE, BLACK}) {
            for (size_t i = 0; i < Neural::N_FEATURES; i++) {
                EXPECT_EQ(initial_features[c][i], final_features[c][i])
                    << "Position not properly restored at index " << i;
            }
        }
    }
};


TEST_F(NeuralFeaturesTest, IncrementalUpdates) {
    for (const auto& fen : test_positions) {
        board.fen_decode(fen);
        
        // Get legal moves in this position
        auto moves = board.get_moves();
        
        // Test each legal move
        for (auto& move : moves) {
            verify_feature_update(move, board.who_to_play());
        }
    }
}

TEST_F(NeuralFeaturesTest, ColourSymmetry) {
    for (const auto & fen : test_positions) {
        board.fen_decode(fen);
        auto start_w = Neural::encode(board)[WHITE];
        auto start_b = Neural::encode(board)[BLACK];
        board.flip();
        auto flipped_w = Neural::encode(board)[WHITE];
        auto flipped_b = Neural::encode(board)[BLACK];

        // Verify flipped features are the same as original
        for (size_t i = 0; i < Neural::N_FEATURES; i++) {
            EXPECT_EQ(start_w[i], flipped_b[i]);
            EXPECT_EQ(start_b[i], flipped_w[i]);
        }
    }
}