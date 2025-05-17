#include <gtest/gtest.h>
#include "fixed.hpp"
#include <cmath>
#include <limits>

using namespace Neural;

// Test fixture for fixed point tests
class FixedPointTest : public ::testing::Test {
protected:
    // Common test values
    static constexpr float kEpsilon = 1e-5f;
};

// Basic construction and conversion tests
TEST_F(FixedPointTest, BasicConstruction) {
    // Test default constructor
    Fixed<8, 0> a;
    EXPECT_EQ(a.raw_value(), 0);
    
    // Test integer constructor
    Fixed<8, 0> b(42);
    EXPECT_EQ(b.raw_value(), 42);
    
    // Test float constructor (no scaling)
    auto c = Fixed<8, 8>::from_float(42.0f);
    EXPECT_EQ(c.raw_value(), 42);
    
    // Test float constructor (with scaling)
    auto d = Fixed<8, 7>::from_float(42.0f);
    EXPECT_EQ(d.raw_value(), 42 << 1);

    // Negative value
    auto e = Fixed<8, 8>::from_float(-42.0f);
    EXPECT_EQ(e.raw_value(), -42);
}

TEST_F(FixedPointTest, FloatingPointConversion) {
    // Q8.0 format (no fractional bits)
    Fixed<8, 8> a(42);
    EXPECT_FLOAT_EQ(a.as<float>(), 42.0f);
    
    // Q4.4 format (4 integer bits, 4 fractional bits)
    auto b = Fixed<8, 4>::from_float(2.5f);
    EXPECT_FLOAT_EQ(b.as<float>(), 2.5f);
    
    // Q0.8 format (0 integer bits, 8 fractional bits - values in [-1, 1))
    auto c = Fixed<8, 1>::from_float(0.5f);
    EXPECT_FLOAT_EQ(c.as<float>(), 0.5f);

    auto d = Fixed<8, 1>::from_float(-0.5f);
    EXPECT_FLOAT_EQ(d.as<float>(), -0.5f);
    
    // Large scale shift (for large integers)
    auto e = Fixed<8, 12>::from_float(1024.0f);
    EXPECT_FLOAT_EQ(e.as<float>(), 1024.0f);
}

TEST_F(FixedPointTest, TypePromotionConstruction) {
    // Construct larger fixed point from smaller, same represented range
    auto small = Fixed<8, 4>::from_float(2.5f);
    auto large = small.promote<16>(); // Incresed precision, value should be the same
    
    EXPECT_FLOAT_EQ(large.as<float>(), small.as<float>());
}

// Basic arithmetic tests
TEST_F(FixedPointTest, Addition) {
    Fixed<8, 4> a = Fixed<8, 4>::from_float(2.5f);
    Fixed<8, 4> b = Fixed<8, 4>::from_float(1.75f);
    Fixed<8, 4> sum = a + b;
    
    EXPECT_FLOAT_EQ(sum.as<float>(), 4.25f);
}

TEST_F(FixedPointTest, Subtraction) {
    Fixed<8, 4> a = Fixed<8, 4>::from_float(5.0f);
    Fixed<8, 4> b = Fixed<8, 4>::from_float(2.25f);
    Fixed<8, 4> diff = a - b;
    
    EXPECT_FLOAT_EQ(diff.as<float>(), 2.75f);
}

TEST_F(FixedPointTest, IncrementDecrement) {
    Fixed<8, 4> a = Fixed<8, 4>::from_float(2.5f);
    
    // Test pre-increment
    Fixed<8, 4> b = ++a;
    EXPECT_FLOAT_EQ(a.as<float>(), 2.5f + 1.0f/16.0f);
    EXPECT_FLOAT_EQ(b.as<float>(), 2.5f + 1.0f/16.0f);
    
    // Test pre-decrement
    Fixed<8, 4> c = --a;
    EXPECT_FLOAT_EQ(a.as<float>(), 2.5f);
    EXPECT_FLOAT_EQ(c.as<float>(), 2.5f);
}

TEST_F(FixedPointTest, CompoundAssignment) {
    Fixed<8, 4> a = Fixed<8, 4>::from_float(2.5f);
    Fixed<8, 4> b = Fixed<8, 4>::from_float(1.75f);
    
    a += b;
    EXPECT_FLOAT_EQ(a.as<float>(), 4.25f);
    
    a -= b;
    EXPECT_FLOAT_EQ(a.as<float>(), 2.5f);
}

// Precision rediction tests
TEST_F(FixedPointTest, PrecisionReduction) {
    auto a = Fixed<8, 4>::from_float(2.5f);
    auto b = a.to_scale<6>(); // Reduce precision to Q8.2
    
    EXPECT_FLOAT_EQ(b.as<float>(), 2.5f); // Should be same value
    EXPECT_EQ(b.raw_value(), 10); // Q8.2 representation of 2.5 is 10
}

TEST_F(FixedPointTest, PrecisionChange) {
    auto a = Fixed<8, 4>::from_float(2.5f);
    auto b = a.to_scale<3>();
    
    EXPECT_FLOAT_EQ(b.as<float>(), 2.5f); // Should be same value
    EXPECT_EQ(b.raw_value(), 80);
}

// Multiplication tests
TEST_F(FixedPointTest, MultiplicationStatic) {
    auto a = Fixed<8, 4>::from_float(2.5f); // 2.5 in Q4.4
    auto b = Fixed<8, 4>::from_float(1.75f);  // 1.75 in Q4.4
    
    // Result in Q8.8 format
    auto prod = Fixed<16, 8>::multiply(a, b);
    
    // EXPECT_NEAR(prod.as<float>(), 2.5f * 1.75f, kEpsilon);
    EXPECT_FLOAT_EQ(prod.as<float>(), 4.375f);
}
TEST_F(FixedPointTest, MultiplicationStatic2) {
    auto a = Fixed<16, 5>::from_float(2.5f).to_scale<13>(); // Squeeze into 8 bits
    auto b = Fixed<16, 5>::from_float(1.75f).to_scale<13>();

    // Result in Q8.8 format
    auto prod = Fixed<16, 5>::multiply(a, b);
    
    // EXPECT_NEAR(prod.as<float>(), 2.5f * 1.75f, kEpsilon);
    EXPECT_FLOAT_EQ(prod.as<float>(), 4.375f);
}

TEST_F(FixedPointTest, SmallMultiply) {
    Fixed<8, 4> a = Fixed<8, 4>::from_float(2.5f); // 2.5 in Q4.4
    int factor = 3;
    
    auto result = a.small_multiply(factor);
    
    EXPECT_FLOAT_EQ(result.as<float>(), 7.5f);
}

// Edge case tests
TEST_F(FixedPointTest, ScalingEdgeCases) {
    auto a = Fixed<8, 8>::from_float(42.0f);
    EXPECT_FLOAT_EQ(a.as<float>(), 42.0f);
    
    // Small numbers
    auto b = Fixed<8, -1>::from_float(0.001953125f); // 0.001953125 = 2^(-9) = 1/512
    EXPECT_FLOAT_EQ(b.as<float>(), 0.001953125);
    
    // Large numbers
    auto c = Fixed<8, 20>::from_float(262144.0f); // 262144 = 2^18
    EXPECT_FLOAT_EQ(c.as<float>(), 262144.0f);
}

TEST_F(FixedPointTest, Precision) {
    // Test smallest representable value in Q4.4
    Fixed<8, 4> a = Fixed<8, 4>::from_float(0.0625f); // // 0.0625 = 1/16 (smallest value with 4 fractional bits)
    EXPECT_FLOAT_EQ(a.as<float>(), 0.0625f);
    
    // Test precision limit
    // Fixed<8, 4> b(0.03125f);  // 0.03125 = 1/32 (below precision)
    Fixed<8, 4> b = Fixed<8, 4>::from_float(0.03125f); // 0.03125 = 1/32 (below precision)
    EXPECT_FLOAT_EQ(b.as<float>(), 0.0f);  // Should round to 0
}

TEST_F(FixedPointTest, Range) {
    // Test upper range for Q4.4 (3 integer bits = values up to 7+15/16)
    Fixed<8, 4> max_val = Fixed<8, 4>::from_float(7.9375f); // 15.9375 = 15 + 15/16
    EXPECT_FLOAT_EQ(max_val.as<float>(), 7.9375f);
    EXPECT_EQ(max_val.raw_value(), 127); // 7.9375 in Q4.4 is 127 in raw value
    
    // Test overflow behavior
    Fixed<8, 4> overflow = Fixed<8, 4>::from_float(8.0f);  // Beyond 8-bit range with 4 fractional bits
    EXPECT_NE(overflow.as<float>(), 8.0f);  // Should not be 20.0 due to overflow
    
    // Test negative values
    Fixed<8, 4> negative = Fixed<8, 4>::from_float(-2.5f); // -2.5 in Q4.4
    EXPECT_FLOAT_EQ(negative.as<float>(), -2.5f);

    // Max negative value
    auto max_neg = Fixed<8, 4>::from_float(-8.0f); // Should be able to express -8.0
    EXPECT_FLOAT_EQ(max_neg.as<float>(), -8.0f);
}

// Tests for different bit widths
TEST_F(FixedPointTest, DifferentBitWidths) {
    // 16-bit fixed point
    auto a = Fixed<16, 8>::from_float(123.456f);
    EXPECT_NEAR(a.as<float>(), 123.456f, 0.01f);
    
    // 32-bit fixed point
    Fixed<32, 16> b = Fixed<32, 16>::from_float(31415.926f); // max range is 2^15 (becuase signed)
    EXPECT_NEAR(b.as<double>(), 31415.926f, 0.01);
}

// Test the smallest_int_with_bits template
TEST_F(FixedPointTest, IntegerTypeSelection) {
    // Test some typical use cases
    static_assert(std::is_same<smallest_int_with_bits<7, true>::type, int8_t>::value, 
                  "7 signed bits should map to int8_t");
    
    static_assert(std::is_same<smallest_int_with_bits<8, false>::type, uint8_t>::value, 
                  "8 unsigned bits should map to uint8_t");
    
    static_assert(std::is_same<smallest_int_with_bits<9, true>::type, int16_t>::value, 
                  "9 signed bits should map to int16_t");
    
    static_assert(std::is_same<smallest_int_with_bits<16, false>::type, uint16_t>::value, 
                  "16 unsigned bits should map to uint16_t");
    
    static_assert(std::is_same<smallest_int_with_bits<32, true>::type, int32_t>::value, 
                  "32 signed bits should map to int32_t");
    
    static_assert(std::is_same<smallest_int_with_bits<64, false>::type, uint64_t>::value, 
                  "64 unsigned bits should map to uint64_t");
}
