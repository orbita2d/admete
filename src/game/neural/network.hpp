#pragma once
#include <types.hpp>
#include <linalg.hpp>
#include <features.hpp>

namespace Neural {
  typedef int nn_t;
  inline bool ENABLED = false;
  // Feature vectors calculated from the initial board state
  // InitialFeatureDetectionLayer is idenditcal for each colour, a single matrix
  // Accumulator stores the output of that first layer
  // Incremental changes to the feature vectors can be added projected into the Accumulator
  // Then there's *the rest* of the neural network

  template <typename T, size_t Input, size_t Output>
  class Layer {
    public:
      virtual ~Layer() = default;

      virtual Vector<T, Output> forward(const Vector<T, Input>& input) const = 0;

      constexpr size_t input_size() const { return Input; }
      constexpr size_t output_size() const { return Output; }
  };

  template <typename T, size_t Input, size_t Output>
  class LinearLayer : public Layer<T, Input, Output> {
    public:
      LinearLayer(const Matrix<T, Output, Input>& weights, const Vector<T, Output>& bias)
        : weights(weights), bias(bias) {}
      
      LinearLayer() = default;

      Vector<T, Output> forward(const Vector<T, Input>& input) const override {
        return weights.matmul(input) + bias;
      }

      // Calculate the change in output for a given change in input
      Vector<T, Output> delta(const SparseVector<T, Input>& input) const {
        return weights.matmul(input);
      }

      // factory methods
      static LinearLayer zeros() {
        auto mat = Matrix<T, Output, Input>();
        for (size_t i = 0; i < Output; i++) {
          for (size_t j = 0; j < Input; j++) {
            mat.at(i, j) = 0;
          }
        }
        auto bias = Vector<T, Output>();
        for (size_t i = 0; i < Output; i++) {
          bias[i] = 0;
        }
        return LinearLayer(mat, bias);
      }

    private:
      Matrix<T, Output, Input> weights;
      Vector<T, Output> bias;
  };

  template <typename T>
  T relu(T x) {
    return std::max(x, 0);
  }
  template <typename T, size_t N>
  Vector<T, N> relu(const Vector<T, N>& x) {
    Vector<T, N> result;
    for (size_t i = 0; i < N; i++) {
      result[i] = relu(x[i]);
    }
    return result;
  }

  constexpr size_t AccumulatorSize = 128;

  class Accumulator {
    public:
      Accumulator() = default;
      void initialise(const Board& board);
      void make_move(const Move& move, const Colour side);
      void unmake_move(const Move& move, const Colour side);
      const Vector<nn_t, AccumulatorSize>& get(Colour c) const { return accumulated[c]; }

    private:
      per_colour<Vector<nn_t, AccumulatorSize>> accumulated;
  };

  struct Network {
    LinearLayer<nn_t, N_FEATURES, AccumulatorSize> accumulator_layer;
    LinearLayer<nn_t, AccumulatorSize*2, 1> output_layer;

    Network() {
      accumulator_layer = LinearLayer<nn_t, N_FEATURES, AccumulatorSize>::zeros();
      output_layer = LinearLayer<nn_t, AccumulatorSize*2, 1>::zeros();
    }

    nn_t forward(const Accumulator& accm, Colour us) const {
      auto input_us = accm.get(us);
      auto input_them = accm.get(~us);
      auto input = Vector<nn_t, AccumulatorSize*2>();
      // concatenate the two vectors 
      // TODO: don't really need to copy here
      for (size_t i = 0; i < AccumulatorSize; i++) {
        input[i] = input_us[i];
      }
      for (size_t i = 0; i < AccumulatorSize; i++) {
        input[i + AccumulatorSize] = input_them[i];
      }
      input = relu(input);
      return output_layer.forward(input)[0];
    }

    void load(const std::string& path) {
      // TODO: implement
    }
    
  };

  inline Network network = Network();

} // namespace Neural