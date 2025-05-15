#include <gtest/gtest.h>
#include "network.hpp"
#include "weights.hpp"

using namespace Neural;

class QuantisedLinearLayerTest : public ::testing::Test {
protected:
    // Define test parameters
    using layer0_t = LinearLayer<nn_t, 128, 64>;
    static constexpr size_t INPUT_SIZE = layer0_t::In;
    static constexpr size_t OUTPUT_SIZE = layer0_t::Out;
    static constexpr uint8_t ACC_BITS = 16;
    static constexpr int8_t ACC_SCALE_SHIFT = 4;
    
    using FloatingLayer = LinearLayer<nn_t, INPUT_SIZE, OUTPUT_SIZE>;
    using QuantisedLayer = QuantisedLinearLayer<INPUT_SIZE, OUTPUT_SIZE, ACC_BITS, ACC_SCALE_SHIFT>;
    
    // Test vectors
    Vector<nn_t, INPUT_SIZE> float_input;
    Vector<QuantisedLayer::accT, INPUT_SIZE> fixed_input;
    
    // Layers for testing
    std::unique_ptr<FloatingLayer> float_layer;
    std::unique_ptr<QuantisedLayer> quantised_layer;
    
    void SetUp() override {
        // Create a random floating point layer
        float_layer = generated::gen_layer_0();
        
        // Create equivalent quantised layer
        quantised_layer = std::make_unique<QuantisedLayer>(*float_layer);
        
        // Generate random input data
        for (size_t i = 0; i < INPUT_SIZE; i++) {
            // Generate values between -1 and 1 for stable testing
            float_input[i] = (static_cast<nn_t>(rand()) / RAND_MAX) * 2.0f - 1.0f;
            fixed_input[i] = QuantisedLayer::accT::from_float(float_input[i]);
        }
    }
    
    // Helper function to convert fixed point results to float for comparison
    Vector<nn_t, OUTPUT_SIZE> fixed_to_float(const Vector<QuantisedLayer::accT, OUTPUT_SIZE>& fixed_vec) {
        Vector<nn_t, OUTPUT_SIZE> result;
        for (size_t i = 0; i < OUTPUT_SIZE; i++) {
            result[i] = fixed_vec[i].template as<nn_t>();
        }
        return result;
    }
};

TEST_F(QuantisedLinearLayerTest, ConstructionFromFloating) {
    // Simply test that a quantised layer can be constructed from a floating layer
    QuantisedLayer layer(*float_layer);
    SUCCEED() << "Successfully constructed QuantisedLinearLayer from LinearLayer";
}

TEST_F(QuantisedLinearLayerTest, ApproximatelyMatchesFloating) {
    // Forward pass with floating point layer
    auto float_output = float_layer->forward(float_input);
    
    // Forward pass with quantised layer
    auto fixed_output = quantised_layer->forward(fixed_input);
    auto fixed_as_float = fixed_to_float(fixed_output);
    
    // Compare results - they should be close but not identical due to quantisation
    for (size_t i = 0; i < OUTPUT_SIZE; i++) {
        // Tolerance depends on the bit width and scale - adjust as needed
        // With 16 bits, we expect pretty good precision
        // The activations ate ~ N(0,1) by design, this is in standard deviations
        EXPECT_NEAR(float_output[i], fixed_as_float[i], 0.2f)
            << "At index " << i << ": "
            << "Float: " << float_output[i] << ", "
            << "Quantised: " << fixed_as_float[i];
    }
}

TEST_F(QuantisedLinearLayerTest, CorrectWithZeroInput) {
    // Create zero input vectors
    auto zero_float_input = Vector<nn_t, INPUT_SIZE>::zeros();
    auto zero_fixed_input = Vector<QuantisedLayer::accT, INPUT_SIZE>::zeros();
    
    // Forward pass with both layers
    auto float_output = float_layer->forward(zero_float_input);
    auto fixed_output = quantised_layer->forward(zero_fixed_input);
    auto fixed_as_float = fixed_to_float(fixed_output);
    
    // With zero input, the output should be the bias
    // Check that fixed and floating point results are close
    for (size_t i = 0; i < OUTPUT_SIZE; i++) {
        EXPECT_NEAR(float_output[i], fixed_as_float[i], 0.01f)
            << "Bias values differ at index " << i;
    }
}


TEST_F(QuantisedLinearLayerTest, ConsistentResultsAcrossMultipleRuns) {
    // Test that repeated forward passes with the same input produce the same result
    
    auto first_output = quantised_layer->forward(fixed_input);
    
    // Run multiple iterations and check consistency
    for (int run = 0; run < 5; run++) {
        auto output = quantised_layer->forward(fixed_input);
        
        // Results should be bit-exact between runs
        for (size_t i = 0; i < OUTPUT_SIZE; i++) {
            EXPECT_EQ(first_output[i].raw_value(), output[i].raw_value())
                << "Inconsistent results at index " << i << " on run " << run;
        }
    }
}

TEST_F(QuantisedLinearLayerTest, CorrectPrecisionScaling) {
    // This test verifies that the lower_precision method used in forward() 
    // properly scales the values
    
    // First create some values to test
    auto test_value = QuantisedLayer::accT::from_float(0.5f);
    auto scaled_value = test_value.to_scale<QuantisedLayer::intermediate_shift>();
    
    // The scaled value should be representable with fewer bits
    // but maintain approximately the same value when converted back to float
    float original = test_value.as<float>();
    float scaled = scaled_value.as<float>();
    
    // Expect them to be close but not necessarily identical
    EXPECT_NEAR(original, scaled, 0.01f)
        << "Precision scaling changed the value significantly";
        
    // Also verify that multiplying the scaled value works as expected
    nn_t test_float = 0.5f;
    auto test_fixed = QuantisedLayer::accT::from_float(test_float);
    auto scaled_fixed = test_fixed.to_scale<QuantisedLayer::intermediate_shift>();
    
    auto weight_float = 1.5f;
    auto weight_fixed = QuantisedLayer::wT::from_float(weight_float);
    
    // Calculate expected result
    float expected = test_float * weight_float;
    
    // Calculate actual result using the quantised approach
    auto actual_fixed = QuantisedLayer::accT::multiply(weight_fixed, scaled_fixed);
    float actual = actual_fixed.as<float>();
    
    // Compare
    EXPECT_NEAR(expected, actual, 0.05f)
        << "Multiply operation with scaled values produced unexpected result";
}
