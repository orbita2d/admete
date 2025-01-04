#include <gtest/gtest.h>
#include "linalg.hpp"

using namespace Neural;

TEST(LinAlg, VectorBasics) {
    Vector<float, 3> v1;
    v1[0] = 1.0f; v1[1] = 2.0f; v1[2] = 3.0f;
    
    Vector<float, 3> v2;
    v2[0] = 2.0f; v2[1] = 3.0f; v2[2] = 4.0f;
    
    // Test addition
    auto sum = v1 + v2;
    EXPECT_FLOAT_EQ(sum[0], 3.0f);
    EXPECT_FLOAT_EQ(sum[1], 5.0f);
    EXPECT_FLOAT_EQ(sum[2], 7.0f);
    
    // Test subtraction
    auto diff = v2 - v1;
    EXPECT_FLOAT_EQ(diff[0], 1.0f);
    EXPECT_FLOAT_EQ(diff[1], 1.0f);
    EXPECT_FLOAT_EQ(diff[2], 1.0f);
    
    // Test scalar multiplication
    auto scaled = v1 * 2.0f;
    EXPECT_FLOAT_EQ(scaled[0], 2.0f);
    EXPECT_FLOAT_EQ(scaled[1], 4.0f);
    EXPECT_FLOAT_EQ(scaled[2], 6.0f);
    
    // Test dot product
    float dot = v1.dot(v2);
    EXPECT_FLOAT_EQ(dot, 20.0f);  // 1*2 + 2*3 + 3*4
}

TEST(LinAlg, VectorInPlace) {
    Vector<float, 3> v1;
    v1[0] = 1.0f; v1[1] = 2.0f; v1[2] = 3.0f;
    
    Vector<float, 3> v2;
    v2[0] = 2.0f; v2[1] = 3.0f; v2[2] = 4.0f;
    
    v1 += v2;
    EXPECT_FLOAT_EQ(v1[0], 3.0f);
    EXPECT_FLOAT_EQ(v1[1], 5.0f);
    EXPECT_FLOAT_EQ(v1[2], 7.0f);
    
    v1 -= v2;
    EXPECT_FLOAT_EQ(v1[0], 1.0f);
    EXPECT_FLOAT_EQ(v1[1], 2.0f);
    EXPECT_FLOAT_EQ(v1[2], 3.0f);
}

TEST(LinAlg, SparseVector) {
    SparseVector<float, 5> sv;
    sv.set(1, 2.0f);
    sv.set(3, 4.0f);
    
    Vector<float, 5> dense;
    for (size_t i = 0; i < 5; i++) dense[i] = 1.0f;
    
    // Test dot product
    float dot = sv.dot(dense);
    EXPECT_FLOAT_EQ(dot, 6.0f);  // 2*1 + 4*1
    
    // Test conversion to dense
    auto converted = sv.as_dense();
    EXPECT_FLOAT_EQ(converted[0], 0.0f);
    EXPECT_FLOAT_EQ(converted[1], 2.0f);
    EXPECT_FLOAT_EQ(converted[2], 0.0f);
    EXPECT_FLOAT_EQ(converted[3], 4.0f);
    EXPECT_FLOAT_EQ(converted[4], 0.0f);
}

TEST(LinAlg, MatrixMultiplication) {
    Matrix<float, 2, 3> m;  // 2 rows, 3 columns
    // [1 2 3]
    // [4 5 6]
    m.at(0,0) = 1.0f; m.at(0,1) = 2.0f; m.at(0,2) = 3.0f;
    m.at(1,0) = 4.0f; m.at(1,1) = 5.0f; m.at(1,2) = 6.0f;
    
    // Test dense matmul
    Vector<float, 3> v;
    v[0] = 1.0f; v[1] = 2.0f; v[2] = 3.0f;
    
    auto result = m.matmul(v);
    EXPECT_FLOAT_EQ(result[0], 14.0f);  // 1*1 + 2*2 + 3*3
    EXPECT_FLOAT_EQ(result[1], 32.0f);  // 4*1 + 5*2 + 6*3
    
    // Test sparse matmul
    SparseVector<float, 3> sv;
    sv.set(0, 1.0f);
    sv.set(2, 3.0f);
    
    auto sparse_result = m.matmul(sv);
    EXPECT_FLOAT_EQ(sparse_result[0], 10.0f);  // 1*1 + 3*3
    EXPECT_FLOAT_EQ(sparse_result[1], 22.0f);  // 4*1 + 6*3
}

TEST(LinAlg, MatrixDimensions) {
    Matrix<float, 2, 3> m;
    EXPECT_EQ(m.rows(), 2);
    EXPECT_EQ(m.cols(), 3);
}