#include <gtest/gtest.h>
#include "network.hpp"
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

TEST(NeuralNetwork, LinearLayerSparse) {
    Matrix<nn_t, 2, 3> weights;
    weights.at(0,0) = 1; weights.at(0,1) = 2; weights.at(0,2) = 3;
    weights.at(1,0) = 4; weights.at(1,1) = 5; weights.at(1,2) = 6;
    
    Vector<nn_t, 2> bias;
    bias[0] = 1; bias[1] = 2;
    
    LinearLayer<nn_t, 3, 2> layer(weights, bias);
    
    // Test sparse delta calculation
    SparseVector<nn_t, 3> sparse_input;
    sparse_input.set(0, 1);
    sparse_input.set(2, 3);
    
    auto delta = layer.delta(sparse_input);
    EXPECT_EQ(delta[0], 10);  // 1*1 + 3*3
    EXPECT_EQ(delta[1], 22);  // 4*1 + 6*3
}


TEST(NeuralNetwork, ReLUBehavior) {
    Vector<nn_t, 4> input{-100, -1, 0, 100};
    auto output = relu(input);
    EXPECT_EQ(output[0], 0);    // Large negative
    EXPECT_EQ(output[1], 0);    // Small negative  
    EXPECT_EQ(output[2], 0);    // Zero
    EXPECT_EQ(output[3], 100);  // Positive passes through
}

class AccumulatorTest : public ::testing::Test {
protected:
    Board board;
    Neural::Accumulator accum;
    
    void SetUp() override {
        board = Board();
        Neural::ENABLED = true;

        // initialise some random weights for the network
        
        auto weights = Neural::Matrix<nn_t, Neural::AccumulatorSize, Neural::N_FEATURES>::zeros();
        auto bias = Neural::Vector<nn_t, Neural::AccumulatorSize>::zeros();
        for (size_t i = 0; i < Neural::AccumulatorSize; i++) {
            for (size_t j = 0; j < Neural::N_FEATURES; j++) {
                weights.at(i, j) = 0.01 * (i + j) + 0x314159;
            }
            bias[i] = 0.01 * i + 0x271828;
        }

        Neural::network.accumulator_layer = Neural::LinearLayer<nn_t, Neural::N_FEATURES, Neural::AccumulatorSize>(weights, bias); 
    }

    std::vector<std::string> test_positions = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"
        // Added common test positions but kept list shorter for faster testing
    };

    void verify_accumulator_update(Move& move) {
        // Get initial state
        accum.initialise(board);
        auto initial_white = accum.get(WHITE);
        auto initial_black = accum.get(BLACK);
        
        // Make move incremental
        const Colour us = board.who_to_play();
        board.make_move(move);
        accum.make_move(move, us);
        
        // Get fresh accumulator for comparison
        Neural::Accumulator direct;
        direct.initialise(board);
        
        // Compare incremental vs direct
        EXPECT_EQ(accum.get(WHITE), direct.get(WHITE));
        EXPECT_EQ(accum.get(BLACK), direct.get(BLACK));
        
        // Verify unmake restores state
        board.unmake_move(move);
        accum.unmake_move(move, us);
        
        EXPECT_EQ(accum.get(WHITE), initial_white);
        EXPECT_EQ(accum.get(BLACK), initial_black);
    }
};

TEST_F(AccumulatorTest, IncrementalUpdates) {
    for (const auto& fen : test_positions) {
        board.fen_decode(fen);
        auto moves = board.get_moves();
        
        for (auto& move : moves) {
            verify_accumulator_update(move);
        }
    }
}

TEST_F(AccumulatorTest, ColourSymmetry) {
  for (const auto & fen : test_positions) {
    board.fen_decode(fen);
    accum.initialise(board);
    auto start_w = accum.get(WHITE);
    auto start_b = accum.get(BLACK);
    board.flip();
    accum.initialise(board);
    auto flipped_w = accum.get(WHITE);
    auto flipped_b = accum.get(BLACK);
    EXPECT_EQ(start_w, flipped_b);
    EXPECT_EQ(start_b, flipped_w);
  }
}

class NetworkIntegrationTest : public ::testing::Test {
protected:
    Board board;
    Network saved_network;  // To restore the global network after tests
    
    void SetUp() override {
        // Save current global network
        saved_network = network;
        
        // Initialize random-ish weights in global network
        auto acc_weights = Matrix<nn_t, AccumulatorSize, N_FEATURES>::zeros();
        auto acc_bias = Vector<nn_t, AccumulatorSize>::zeros();
        for (size_t i = 0; i < AccumulatorSize; i++) {
            for (size_t j = 0; j < N_FEATURES; j++) {
                acc_weights.at(i, j) = 0.01 * (i + j) + 0x314159;
            }
            acc_bias[i] = 0.01 * i + 0x271828;
        }
        network.accumulator_layer = LinearLayer<nn_t, N_FEATURES, AccumulatorSize>(acc_weights, acc_bias);

        auto out_weights = Matrix<nn_t, 1, AccumulatorSize*2>::zeros();
        auto out_bias = Vector<nn_t, 1>::zeros();
        for (size_t i = 0; i < AccumulatorSize*2; i++) {
            out_weights.at(0, i) = 0.01 * i + 0x161803;
        }
        out_bias[0] = 0x112358;
        network.output_layer = LinearLayer<nn_t, AccumulatorSize*2, 1>(out_weights, out_bias);
    }

    void TearDown() override {
        // Restore global network
        network = saved_network;
    }

    std::vector<std::string> test_positions = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"
    };
};

TEST_F(NetworkIntegrationTest, EvaluationSymmetry) {
    Accumulator accum;
    
    for (const auto& fen : test_positions) {
        board.fen_decode(fen);
        accum.initialise(board);
        auto score = network.forward(accum, WHITE);
        
        board.flip();
        accum.initialise(board);
        auto flipped_score = network.forward(accum, BLACK);
        
        EXPECT_EQ(score, flipped_score) 
            << "Position: " << fen << "\n"
            << "Original score: " << score << "\n"
            << "Flipped score: " << flipped_score;
    }
}

TEST_F(NetworkIntegrationTest, NonZeroOutput) {
    Accumulator accum;
    board.fen_decode(test_positions[0]);  // Use starting position
    accum.initialise(board);
    
    auto score = network.forward(accum, WHITE);
    EXPECT_NE(score, 0) << "Network should produce non-zero output with initialized weights";
}