#include <gtest/gtest.h>
#include "network.hpp"
#include "weights.hpp" // Actual weights, generated
#include "board.hpp"

using namespace Neural;

TEST(NeuralNetwork, LinearLayerForward) {
    // Simpler test with clear input->output relationship
    Matrix<nn_t, 2, 2> weights;
    weights.at(0,0) = 1; weights.at(0,1) = 0;  // First output = x
    weights.at(1,0) = 0; weights.at(1,1) = 1;  // Second output = y
    
    Vector<nn_t, 2> bias;
    bias[0] = 10; bias[1] = 20;  // Fixed offsets
    
    LinearLayer<nn_t, 2, 2> layer(weights, bias);
    
    Vector<nn_t, 2> input{5, 7};
    auto output = layer.forward(input);
    EXPECT_EQ(output[0], 15);  // 5*1 + 7*0 + 10
    EXPECT_EQ(output[1], 27);  // 5*0 + 7*1 + 20
}

TEST(NeuralNetwork, ReLUBehavior) {
    Vector<nn_t, 4> input{-100, -1, 0, 100};
    auto output = relu(input);
    EXPECT_EQ(output[0], 0);    // Large negative
    EXPECT_EQ(output[1], 0);    // Small negative  
    EXPECT_EQ(output[2], 0);    // Zero
    EXPECT_EQ(output[3], 100);  // Positive passes through
}

class NetworkIntegrationTest : public ::testing::Test {
protected:
    Board board;
    network_t network;
    accumulator_t accumulator;
    
    void SetUp() override {
        // 
        accumulator = Neural::get_accumulator();
        network = Neural::get_network();
    }

    std::vector<std::string> test_positions = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"
    };
};

TEST_F(NetworkIntegrationTest, EvaluationSymmetry) {    
    for (const auto& fen : test_positions) {
        board.fen_decode(fen);
        accumulator.initialise(board);
        auto score = network.forward(accumulator, WHITE);
        
        board.flip();
        accumulator.initialise(board);
        auto flipped_score = network.forward(accumulator, BLACK);
        
        EXPECT_EQ(score, flipped_score) 
            << "Position: " << fen << "\n"
            << "Original score: " << score << "\n"
            << "Flipped score: " << flipped_score;
    }
}

TEST_F(NetworkIntegrationTest, NonZeroOutput) {
    // This is to ensure that the network is not returning zero for all positions, which has happened more than once
    bool any_nonzero = false;
    for (const auto& fen : test_positions) {
        board.fen_decode(fen);
        accumulator.initialise(board);
        auto score = network.forward(accumulator, WHITE);
        any_nonzero |= (score != 0);
    }
    EXPECT_TRUE(any_nonzero);
}

TEST_F(NetworkIntegrationTest, AccumulationCorrectness) {
    for (const auto& fen : test_positions) {
        board.fen_decode(fen);
        auto score0 = network.forward(board.accumulator(), board.who_to_play());
        // Make a move
        auto moves = board.get_moves();
        for (auto move : moves) {
            board.make_move(move);
            auto score1 = network.forward(board.accumulator(), board.who_to_play());
            accumulator.initialise(board);
            auto score2 = network.forward(accumulator, board.who_to_play());
            board.unmake_move(move);

            EXPECT_NEAR(score1, score2, 1e-5)
                << "Position: " << fen << "\n"
                << "Move: " << move.pretty() << "\n"
                << "Score1: " << score1 << "\n"
                << "Score2: " << score2;

            auto score3 = network.forward(board.accumulator(), board.who_to_play());
            EXPECT_NEAR(score0, score3, 1e-5)
                << "Position: " << fen << "\n"
                << "Move: " << move.pretty() << "\n"
                << "Score0: " << score0 << "\n"
                << "Score3: " << score3;

        }
    }

}