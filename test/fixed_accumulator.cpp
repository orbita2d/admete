#include <gtest/gtest.h>
#include "network.hpp"
#include "weights.hpp"
#include "board.hpp"

using namespace Neural;

class FixedAccumulatorTest : public ::testing::Test {
protected:
    Board board;
    using FloatingAcc = FloatingAccumulatorLayer<nn_t, N_FEATURES, N_ACCUMULATED>;
    using FixedAcc = FixedAccumulatorLayer<N_FEATURES, N_ACCUMULATED, 32, ACC_SHIFT>;
    
    // Create a custom instance of both fixed and floating accumulators for comparison
    std::unique_ptr<FloatingAcc> floating_layer;
    std::unique_ptr<FixedAcc> fixed_layer;
    
    // Actual accumulators using these layers
    std::unique_ptr<Accumulator<N_FEATURES, N_ACCUMULATED, ACC_SHIFT>> fixed_accumulator;
    
    // Test positions to verify behavior across different board states
    std::vector<std::string> test_positions = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1"
    };
    
    void SetUp() override {
        // Get layers from your weights 
        floating_layer = Neural::generated::gen_accumulator();
        fixed_layer = std::make_unique<FixedAcc>(*floating_layer);
        
        // Create an accumulator with the fixed layer
        fixed_accumulator = std::make_unique<Accumulator<N_FEATURES, N_ACCUMULATED, ACC_SHIFT>>(*fixed_layer);
    }
    
    // Helper to create a floating accumulator for comparison
    auto create_floating_accumulator() {
        return Accumulator<N_FEATURES, N_ACCUMULATED, ACC_SHIFT>(*floating_layer);
    }
    
    // Convert fixed accumulator values to floating point for comparison
    Vector<nn_t, N_ACCUMULATED> fixed_to_float(const Vector<typename FixedAcc::accT, N_ACCUMULATED>& fixed_vec) {
        Vector<nn_t, N_ACCUMULATED> result;
        for (size_t i = 0; i < N_ACCUMULATED; i++) {
            result[i] = fixed_vec[i].template as<nn_t>();
        }
        return result;
    }
};

TEST_F(FixedAccumulatorTest, ApproximatelyMatchesFloating) {
    // For each test position, verify that fixed and floating give similar results
    for (const auto& fen : test_positions) {
        board.fen_decode(fen);
        
        // Create a floating accumulator for comparison
        auto floating_accumulator = create_floating_accumulator();
        
        // Initialize both accumulators with the same board
        fixed_accumulator->initialise(board);
        floating_accumulator.initialise(board);
        
        // Compare values for both colors
        for (Colour c : {WHITE, BLACK}) {
            auto fixed_values = fixed_accumulator->get_as<nn_t>(c);
            auto floating_values = floating_accumulator.get_as<nn_t>(c);
            
            // Check approximate equality for each value
            for (size_t i = 0; i < N_ACCUMULATED; i++) {
                EXPECT_NEAR(fixed_values[i], floating_values[i], 1e-4)
                    << "Position: " << fen << "\n"
                    << "Color: " << (c == WHITE ? "WHITE" : "BLACK") << "\n"
                    << "Index: " << i << "\n"
                    << "Fixed: " << fixed_values[i] << "\n"
                    << "Floating: " << floating_values[i];
            }
        }
    }
}

TEST_F(FixedAccumulatorTest, PreservesStateAfterMoveUnmove) {
    for (const auto& fen : test_positions) {
        board.fen_decode(fen);
        fixed_accumulator->initialise(board);
        
        // Save the initial state for both colors
        auto initial_white = fixed_accumulator->get(WHITE);
        auto initial_black = fixed_accumulator->get(BLACK);
        
        // Get legal moves for the current position
        auto moves = board.get_moves();
        for (auto move : moves) {
            // Make and unmake the move, tracking accumulator state
            board.make_move(move);
            fixed_accumulator->make_move(move, ~board.who_to_play());
            fixed_accumulator->unmake_move(move, ~board.who_to_play());
            board.unmake_move(move);
            
            // Verify state is preserved after move+unmove
            auto after_white = fixed_accumulator->get(WHITE);
            auto after_black = fixed_accumulator->get(BLACK);
            
            for (size_t i = 0; i < N_ACCUMULATED; i++) {
                EXPECT_EQ(initial_white[i], after_white[i])
                    << "Position: " << fen << "\n"
                    << "Move: " << move.pretty() << "\n"
                    << "Index: " << i << "\n"
                    << "Initial: " << initial_white[i].raw_value() << "\n"
                    << "After: " << after_white[i].raw_value();
                
                EXPECT_EQ(initial_black[i], after_black[i])
                    << "Position: " << fen << "\n"
                    << "Move: " << move.pretty() << "\n"
                    << "Index: " << i << "\n"
                    << "Initial: " << initial_black[i].raw_value() << "\n"
                    << "After: " << after_black[i].raw_value();
            }
        }
    }
}

TEST_F(FixedAccumulatorTest, CorrectAfterMoves) {
    for (const auto& fen : test_positions) {
        board.fen_decode(fen);
        
        // Get legal moves for the current position
        auto moves = board.get_moves();
        for (auto move : moves) {
            // Initialize the accumulator
            fixed_accumulator->initialise(board);
            auto initial_white = fixed_accumulator->get(WHITE).copy(); // copy because we really have a reference
            auto initial_black = fixed_accumulator->get(BLACK).copy();
            
            // Make the move in both the board and accumulator
            board.make_move(move);
            fixed_accumulator->make_move(move, ~board.who_to_play()); // ~who_to_play because make_move changes the player
            
            // Create a fresh accumulator and initialize with the new board state
            auto fresh_accumulator = std::make_unique<Accumulator<N_FEATURES, N_ACCUMULATED, ACC_SHIFT>>(*fixed_layer);
            fresh_accumulator->initialise(board);
            
            // Compare incremental update vs. full initialization
            for (Colour c : {WHITE, BLACK}) {
                auto incremental = fixed_accumulator->get(c).copy();
                auto fresh = fresh_accumulator->get(c).copy();
                
                for (size_t i = 0; i < N_ACCUMULATED; i++) {
                    EXPECT_EQ(incremental[i], fresh[i])
                        << "Position: " << fen << "\n"
                        << "Move: " << move.pretty() << "\n"
                        << "Color: " << (c == WHITE ? "WHITE" : "BLACK") << "\n"
                        << "Index: " << i << "\n"
                        << "Incremental: " << incremental[i].raw_value() << "\n"
                        << "Fresh: " << fresh[i].raw_value();
                }
                auto any_changed = false;
                auto initial = (c == WHITE) ? initial_white : initial_black;
                for (size_t i = 0; i < N_ACCUMULATED; i++) {
                    if (incremental[i] != initial[i]) {
                        any_changed = true;
                        break;
                    }
                }
                EXPECT_TRUE(any_changed)
                    << "Position: " << fen << "\n"
                    << "Move: " << move.pretty() << "\n"
                    << "Color: " << (c == WHITE ? "WHITE" : "BLACK") << "\n"
                    << "No changes detected after move.";
            }
            
            // Unmake the move to restore the board for the next iteration
            board.unmake_move(move);
        }
    }
}

// Optional: Test with a network to ensure evaluation consistency
TEST_F(FixedAccumulatorTest, NetworkEvaluationConsistency) {
    network_t network = Neural::get_network();
    
    for (const auto& fen : test_positions) {
        board.fen_decode(fen);
        
        fixed_accumulator->initialise(board);
        auto initial_eval = network.forward(*fixed_accumulator, board.who_to_play());
        
        // Get legal moves
        auto moves = board.get_moves();
        for (auto move : moves) {
            // Make move
            board.make_move(move);
            fixed_accumulator->make_move(move, ~board.who_to_play());
            auto after_move_eval = network.forward(*fixed_accumulator, board.who_to_play());
            
            // Create fresh accumulator
            auto fresh_accumulator = std::make_unique<Accumulator<N_FEATURES, N_ACCUMULATED, ACC_SHIFT>>(*fixed_layer);
            fresh_accumulator->initialise(board);
            auto fresh_eval = network.forward(*fresh_accumulator, board.who_to_play());
            
            // Compare network evaluations
            EXPECT_NEAR(after_move_eval, fresh_eval, 1e-3)
                << "Position: " << fen << "\n"
                << "Move: " << move.pretty() << "\n"
                << "After move eval: " << after_move_eval << "\n"
                << "Fresh eval: " << fresh_eval;
            
            // Unmake move
            board.unmake_move(move);
            fixed_accumulator->unmake_move(move, board.who_to_play());
            auto restored_eval = network.forward(*fixed_accumulator, board.who_to_play());
            
            // Check that evaluation is restored after unmake
            EXPECT_NEAR(initial_eval, restored_eval, 1e-3)
                << "Position: " << fen << "\n"
                << "Move: " << move.pretty() << "\n"
                << "Initial eval: " << initial_eval << "\n"
                << "Restored eval: " << restored_eval;
        }
    }
}