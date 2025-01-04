#pragma once
#include <types.hpp>
#include <unordered_map>
#include <vector>


namespace Neural {
    // Linear algebra functions and classes

    // 1D vector
    template <typename T, size_t N> 
    struct Vector {
        T data[N];
        static Vector<T, N> zeros() {
            Vector<T, N> result;
            for (size_t i = 0; i < N; i++) {
                result[i] = 0;
            }
            return result;
        }
        T& operator[](size_t i) { return data[i]; }
        const T& operator[](size_t i) const { return data[i]; }

        constexpr size_t size() const { return N; }

        Vector<T, N> operator+(const Vector<T, N>& other) const {
            Vector<T, N> result;
            for (size_t i = 0; i < N; i++) {
                result[i] = data[i] + other[i];
            }
            return result;
        }

        Vector<T, N> operator-(const Vector<T, N>& other) const {
            Vector<T, N> result;
            for (size_t i = 0; i < N; i++) {
                result[i] = data[i] - other[i];
            }
            return result;
        }

        Vector<T, N> operator*(const T scalar) const {
            Vector<T, N> result;
            for (size_t i = 0; i < N; i++) {
                result[i] = data[i] * scalar;
            }
            return result;
        }

        Vector<T, N> operator/(const T scalar) const {
            Vector<T, N> result;
            for (size_t i = 0; i < N; i++) {
                result[i] = data[i] / scalar;
            }
            return result;
        }

        T dot(const Vector<T, N>& other) const {
            T result = 0;
            for (size_t i = 0; i < N; i++) {
                result += data[i] * other[i];
            }
            return result;
        }

        // increment by other vector
        Vector<T, N>& operator+=(const Vector<T, N>& other) {
            for (size_t i = 0; i < N; i++) {
                data[i] += other[i];
            }
            return *this;
        }

        // decrement by other vector
        Vector<T, N>& operator-=(const Vector<T, N>& other) {
            for (size_t i = 0; i < N; i++) {
                data[i] -= other[i];
            }
            return *this;
        } 

        bool operator==(const Vector<T, N>& other) const {
            for (size_t i = 0; i < N; i++) {
                if (data[i] != other[i]) {
                    return false;
                }
            }
            return true;
        }
    };

    // sparse vector
    template <typename T, size_t N>
    struct SparseVector {
        std::vector<std::pair<size_t, T>> data;
        constexpr size_t size() const { return N; }

        // indexing
        void set(size_t i, T value) {
          // assume that it has not been set before (performance > safety here sorry future me)
          assert(i < N);
          for (auto& [index, _] : data) {
              assert(index != i);
          }
          data.push_back({i, value});
        }

        // dot product
        T dot(const Vector<T, N>& other) const {
            T result = 0;

            for (const auto& [index, value] : data) {
                result += value * other[index];
            }
            return result;
        }
        // Addition
        Vector<T, N> operator+(const Vector<T, N>& other) const {
            Vector<T, N> result;
            for (size_t i = 0; i < N; i++) {
                result[i] = other[i];
            }
            for (const auto& [index, value] : data) {
                result[index] += value;
            }
            return result;
        }

        Vector<T, N> as_dense() const {
            Vector<T, N> result;
            for (size_t i = 0; i < N; i++) {
                result[i] = 0;
            }
            for (const auto& [index, value] : data) {
                result[index] = value;
            }
            return result;
        }
    };

    // Matrix - just declare it, implementation in the cpp file
    template <typename T, size_t M, size_t N>
    struct Matrix {
        T data[N][M];
        T& at(size_t i, size_t j) { assert(i < M && j < N); return data[j][i];}
        const T& at(size_t i, size_t j) const { assert(i < M && j < N); return data[j][i];}

        constexpr size_t rows() const { return M; }
        constexpr size_t cols() const { return N; }

        static Matrix<T, M, N> zeros() {
            Matrix<T, M, N> result;
            for (size_t i = 0; i < M; i++) {
                for (size_t j = 0; j < N; j++) {
                    result.at(i, j) = 0;
                }
            }
            return result;
        }

        Vector<T, M> matmul(const Vector<T, N>& vec) const {
          Vector<T, M> result = Vector<T, M>::zeros();
          for (size_t i = 0; i < N; i++) {
            for (size_t j = 0; j < M; j++) {
                // We want to be reading from sequential memory.
                result[j] += data[i][j] * vec[i];
            }
          }
          return result;
        }

        Vector<T, M> matmul(const SparseVector<T, N>& vec) const {
          Vector<T, M> result = Vector<T, M>::zeros();
          for (const auto& [index, value] : vec.data) {
            // Compiler please unroll and SIMD optimize this
            for (size_t i = 0; i < M; i++) {
              result[i] += data[index][i] * value;
            }
          }
          return result;
        }
    };
}