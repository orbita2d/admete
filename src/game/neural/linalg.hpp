#pragma once
#include <types.hpp>
#include <unordered_map>
#include <vector>


namespace Neural {
    // Linear algebra functions and classes

    // 1D vector
    template <typename T, size_t N> 
    struct Vector {
        alignas(cache_line_size) T data[N];
        T& at(size_t i) { assert(i < N); return data[i];}
        static Vector<T, N> zeros() {
            Vector<T, N> result;
            for (size_t i = 0; i < N; i++) {
                result[i] = 0;
            }
            return result;
        }

        static Vector<T, N> random() {
            Vector<T, N> result;
            for (size_t i = 0; i < N; i++) {
                result[i] = (rand() % 2 == 0 ? 1 : -1) * (rand() % 1000);
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

        template <typename U>
        Vector<U, N> cast_as() const {
            Vector<U, N> result;
            for (size_t i = 0; i < N; i++) {
                result[i] = static_cast<U>(data[i]);
            }
            return result;
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
            #ifndef NDEBUG
            for (auto& [index, _] : data) {
                assert(index != i);
            }
            #endif
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

        template <typename U>
        SparseVector<U, N> cast_as() const {
            SparseVector<U, N> result;
            for (const auto& [index, value] : data) {
                result.set(index, static_cast<U>(value));
            }
            return result;
        }
    };

    // Matrix
    template <typename T, size_t M, size_t N>
    struct Matrix {
        alignas(cache_line_size) T data[M * N];
        T& at(const size_t i, const size_t j) { assert(i < M && j < N);  return data[i * N + j]; }
        const T& at(const size_t i, const size_t j) const { assert(i < M && j < N); return data[i * N + j]; }

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

        static Matrix<T, M, N> random() {
            Matrix<T, M, N> result;
            for (size_t i = 0; i < M; i++) {
                for (size_t j = 0; j < N; j++) {
                    result.at(i, j) = (rand() % 2 == 0 ? 1 : -1) * (rand() % 1000);
                }
            }
            return result;
        }

        // Transpose
        Matrix<T, N, M> transpose() const {
            Matrix<T, N, M> result;
            for (size_t i = 0; i < M; i++) {
                for (size_t j = 0; j < N; j++) {
                    result.at(j, i) = at(i, j);
                }
            }
            return result;
        }
    };

    template<typename T, size_t M, size_t N, size_t BlockSize>
    struct BlockAccessOptimisedMatrix {
        // This matrix is optimised for matrix-vector multiplication, with small maximum sizes in the cache, but better vectorisation performance than naive row-major storage
        alignas(cache_line_size) T data[M * N];
        static_assert(M % BlockSize == 0, "M must be divisible by BlockSize");
        T& at(const size_t i, const size_t j) {
            assert(i < M && j < N); 
            const size_t block_i = i / BlockSize;
            const size_t in_block_i = i % BlockSize;
            return data[block_i * N * BlockSize + j * BlockSize + in_block_i];
        }
        const T& at(const size_t i, const size_t j) const { 
            assert(i < M && j < N); 
            const size_t block_i = i / BlockSize;
            const size_t in_block_i = i % BlockSize;
            return data[block_i * N * BlockSize + j * BlockSize + in_block_i];
        }

        constexpr size_t rows() const { return M; }
        constexpr size_t cols() const { return N; }

        static BlockAccessOptimisedMatrix<T, M, N, BlockSize> zeros() {
            BlockAccessOptimisedMatrix<T, M, N, BlockSize> result;
            for (size_t i = 0; i < M; i++) {
                for (size_t j = 0; j < N; j++) {
                    result.at(i, j) = 0;
                }
            }
            return result;
        }

        static BlockAccessOptimisedMatrix<T, M, N, BlockSize> random() {
            BlockAccessOptimisedMatrix<T, M, N, BlockSize> result;
            for (size_t i = 0; i < M; i++) {
                for (size_t j = 0; j < N; j++) {
                    result.at(i, j) = (rand() % 2 == 0 ? 1 : -1) * (rand() % 1000);
                }
            }
            return result;
        }

        // Copy from a normal matrix
        static BlockAccessOptimisedMatrix<T, M, N, BlockSize> from_matrix(const Matrix<T, M, N>& mat) {
            BlockAccessOptimisedMatrix<T, M, N, BlockSize> result;
            for (size_t i = 0; i < M; i++) {
                for (size_t j = 0; j < N; j++) {
                    result.at(i, j) = mat.at(i, j);
                }
            }
            return result;
        }
    };
}