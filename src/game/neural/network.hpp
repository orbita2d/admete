#pragma once
#include <types.hpp>
#include <linalg.hpp>
#include <features.hpp>

namespace Neural {
  typedef float nn_t; // TODO: Quantisation?
  inline bool ENABLED = false;
  // Feature vectors calculated from the initial board state
  // InitialFeatureDetectionLayer is idenditcal for each colour, a single matrix
  // Accumulator stores the output of that first layer
  // Incremental changes to the feature vectors can be added projected into the Accumulator
  // Then there's *the rest* of the neural network


  template <typename T, size_t Input, size_t Output>
  class LinearLayer {
    public:
      LinearLayer(const Matrix<T, Output, Input>& weights, const Vector<T, Output>& bias)
        : weights(weights), bias(bias) {}
      
      LinearLayer() = default;

      Vector<T, Output> forward(const Vector<T, Input>& input) const {
        return weights.matmul(input) + bias;
      }

      // Calculate the change in output for a given change in input
      Vector<T, Output> delta(const SparseVector<T, Input>& input) const {
        return weights.matmul(input);
      }

      // factory methods
      static LinearLayer zeros() {
        auto mat = Matrix<T, Output, Input>::zeros();
        auto bias = Vector<T, Output>::zeros();
        return LinearLayer(mat, bias);
      }

      static LinearLayer random() {
        auto mat = Matrix<T, Output, Input>::random();
        auto bias = Vector<T, Output>::random();
        return LinearLayer(mat, bias);
      }

    private:
      Matrix<T, Output, Input> weights;
      Vector<T, Output> bias;
  };

  template <typename T>
  T relu(T x) {
    return std::max(x, T{0});
  }
  template <typename T, size_t N>
  Vector<T, N> relu(const Vector<T, N>& x) {
    Vector<T, N> result;
    for (size_t i = 0; i < N; i++) {
      result[i] = relu(x[i]);
    }
    return result;
  }

  template <size_t InputSize, size_t AccumulatorSize>
  class Accumulator {
    LinearLayer<nn_t, InputSize, AccumulatorSize> acc_layer;
  public:
      Accumulator() : acc_layer(LinearLayer<nn_t, InputSize, AccumulatorSize>::zeros()) {}

      explicit Accumulator(const LinearLayer<nn_t, InputSize, AccumulatorSize>& layer) 
          : acc_layer(layer) {}
      
      // Copy constructor needs to copy the reference
      Accumulator(const Accumulator& other) = default;
      Accumulator& operator=(const Accumulator& other) = default;

      Accumulator(Accumulator&& other) noexcept 
          : acc_layer(std::move(other.acc_layer))
          , accumulated(std::move(other.accumulated)) {}
      
      Accumulator& operator=(Accumulator&& other) noexcept {
          acc_layer = std::move(other.acc_layer);
          accumulated = std::move(other.accumulated);
          return *this;
      }

      void initialise(const Board& board);
      void make_move(const Move& move, const Colour side);
      void unmake_move(const Move& move, const Colour side);
      const Vector<nn_t, AccumulatorSize>& get(Colour c) const { return accumulated[c]; }

    private:
      per_colour<Vector<nn_t, AccumulatorSize>> accumulated;
  };

  template<size_t InputSize, size_t AccumulatorSize>
  void Accumulator<InputSize, AccumulatorSize>::initialise(const Board &board) {
      auto encoded = encode(board);
      for (Colour c : {WHITE, BLACK}) {
          accumulated[c] = acc_layer.forward(encoded[c].cast_as<nn_t>());
      }
  }

  template<size_t InputSize, size_t AccumulatorSize>
  void Accumulator<InputSize, AccumulatorSize>::make_move(const Move &move, const Colour side) {
      // Assuming increment() returns per_colour<SparseVector<nn_t, InputSize>>
      auto diff = increment(move, side, true);
      accumulated[side] += acc_layer.delta(diff[side].cast_as<nn_t>());
      accumulated[~side] += acc_layer.delta(diff[~side].cast_as<nn_t>());
  }

  template<size_t InputSize, size_t AccumulatorSize>
  void Accumulator<InputSize, AccumulatorSize>::unmake_move(const Move &move, const Colour side) {
      auto diff = increment(move, side, false);
      accumulated[side] += acc_layer.delta(diff[side].cast_as<nn_t>());
      accumulated[~side] += acc_layer.delta(diff[~side].cast_as<nn_t>());
  }

  template<size_t InputSize, size_t AccumulatorSize, size_t... LayerSizes>
  class Network {
  private:    
    // Recursive template to build a tuple of layers
    // Base case - just one layer left
    template<size_t In, size_t Out, size_t... Rest>
    struct LayerTypes {
        using type = std::tuple<LinearLayer<nn_t, In, Out>>;
    };
    // Recursive case - concatenate current layer with rest of layers
    template<size_t In, size_t Mid, size_t Out, size_t... Rest>
    struct LayerTypes<In, Mid, Out, Rest...> {
        using type = decltype(std::tuple_cat(
            std::declval<std::tuple<LinearLayer<nn_t, In, Mid>>>(),
            std::declval<typename LayerTypes<Mid, Out, Rest...>::type>()
        ));
    };
    // Tuple of all the layers, excluding the accumulator layer
    using Layers = typename LayerTypes<2*AccumulatorSize, LayerSizes...>::type;

    // Helper for recursive forward pass
    template<size_t I = 0>
    auto forward_impl(const auto& input) const {
        if constexpr (I == sizeof...(LayerSizes) - 1) {
            // Base case - last layer
            return std::get<I>(layers).forward(input);
        } else {
            // Recursive case - apply layer, relu, then continue
            auto& layer = std::get<I>(layers);
            return forward_impl<I + 1>(relu(layer.forward(input)));
        }
    }

    public:
      Layers layers;

      nn_t forward(const Accumulator<InputSize, AccumulatorSize>& accm, Colour us) const {
        // First handle the accumulator concatenation
        auto input_us = accm.get(us);
        auto input_them = accm.get(~us);
        Vector<nn_t, AccumulatorSize*2> input;
        for (size_t i = 0; i < AccumulatorSize; i++) {
            input[i] = input_us[i];
            input[i + AccumulatorSize] = input_them[i];
        }

        return forward_impl(relu(input))[0];  // [0] since final layer outputs size-1 vector
    }
  };

} // namespace Neural