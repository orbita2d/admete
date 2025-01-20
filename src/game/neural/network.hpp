#pragma once
#include <types.hpp>
#include <linalg.hpp>
#include <features.hpp>
#include <algorithm>
#include <limits>


namespace Neural {
  typedef int16_t nn_t;
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
        // return weights.matmul(input) + bias
        Vector<T, Output> result = bias;
        for (size_t i = 0; i < Output; i++) {
          for (size_t j = 0; j < Input; j++) {
            result[i] += weights.at(i, j) * input[j];
          }
        }
        return result;
      }

      // Calculate the change in output for a given change in input
      Vector<T, Output> delta(const SparseVector<T, Input>& input) const {
        Vector<T, Output> result = Vector<T, Output>::zeros();
        for (const auto& [index, value] : input.data) {
          for (size_t i = 0; i < Output; i++) {
            result[i] += weights.at(i, index) * value;
          }
        }
        return result;
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

  template<typename Tw, typename Ta, typename Tin, typename Tout, size_t In, size_t Out>
  class QuantizedLayer {
    // Tw = weight type (e.g. int8_t)
    // Ta = accumulator type (e.g. int32_t)
    // Tnn = input/output type (e.g. int16_t)
    // In = input size
    // Out = output size
    private:
      Matrix<Tw, Out, In> weights;
      Vector<Ta, Out> bias;
      size_t acc_shift;
    public:
      QuantizedLayer(const Matrix<Tw, Out, In>& weights, const Vector<Ta, Out>& bias, size_t acc_shift)
        : weights(weights), bias(bias), acc_shift(acc_shift) {}
      
      QuantizedLayer() = default;

      Vector<Tout, Out> forward(const Vector<Tin, In>& input) const {
        auto out = Vector<Tout, Out>::zeros();
        for (size_t i = 0; i < Out; i++) {
          Ta acc = 0;
          for (size_t j = 0; j < In; j++) {
            // Maticies are stored in column-major order, so the transpose is useful.
            // This is mathematically x W^T + b, rather than Wx + b, but it doesn't matter.
            // This way we don't have to allocate a vector of Ta for the intermediate results.
            acc += static_cast<Ta>(weights.at(i, j)) * static_cast<Ta>(input[j]);
          }
          out.at(i) = static_cast<Tout>(std::clamp(((acc + bias[i]) >> acc_shift), static_cast<Ta>(std::numeric_limits<Tout>::min()), static_cast<Ta>(std::numeric_limits<Tout>::max())));
          // out.at(i) = static_cast<Tout>((acc + bias[i]) >> acc_shift);
        }
        return out;
      }
  };


  template<typename Tw, typename Ta, typename Tin, typename Tout, size_t In, size_t Out>
  class QuantizedAccumulatorLayer {
    // Tw = weight type (e.g. int8_t)
    // Ta = accumulator type (e.g. int32_t)
    // Tnn = input/output type (e.g. int16_t)
    // In = input size
    // Out = output size
    // Optimised for the accumulator layer, where the input is sparse

    public:
      using weight_t = Matrix<Tw, Out, In>;
      QuantizedAccumulatorLayer(const weight_t& weights, const Vector<Ta, Out>& bias, size_t acc_shift)
        : weights(weights.transpose()), bias(bias), acc_shift(acc_shift) {}
      
      QuantizedAccumulatorLayer() = default;

      Vector<Tout, Out> delta(const SparseVector<Tin, In>& input) const {
        auto acc = Vector<Ta, Out>::zeros();
        for (const auto& [index, value] : input.data) {
          for (size_t i = 0; i < Out; i++) {
            acc[i] += static_cast<Ta>(weights.at(index, i)) * static_cast<Ta>(value);
          }
        }
        auto out = Vector<Tout, Out>();
        for (size_t i = 0; i < Out; i++) {
          out[i] = static_cast<Tout>(std::clamp(acc.at(i) >> acc_shift, static_cast<Ta>(std::numeric_limits<Tout>::min()), static_cast<Ta>(std::numeric_limits<Tout>::max()))); 
        }
        return out;
      }

      Vector<Tout, Out> forward(const Vector<Tin, In>& input) const {
        // Warning for the user, this is poorly optimised for dense inputs, this is fine however to use in board initialisation.
        auto acc = bias;
        for (size_t i = 0; i < Out; i++) {
          for (size_t j = 0; j < In; j++) {
            acc.at(i) += static_cast<Ta>(weights.at(i, j)) * static_cast<Ta>(input[j]);
          }
        }
        auto out = Vector<Tout, Out>();
        for (size_t i = 0; i < Out; i++) {
          out.at(i) = static_cast<Tout>(std::clamp(acc.at(i) >> acc_shift, static_cast<Ta>(std::numeric_limits<Tout>::min()), static_cast<Ta>(std::numeric_limits<Tout>::max()))); 
        }
        return out;
      }
      private:
      Matrix<Tw, In, Out> weights;
      Vector<Ta, Out> bias;
      size_t acc_shift;
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
    // LinearLayer<nn_t, InputSize, AccumulatorSize> acc_layer;
  public:
      using layer_t = QuantizedAccumulatorLayer<int16_t, int32_t, int16_t, nn_t, InputSize, AccumulatorSize>;

      Accumulator() = default;

      explicit Accumulator(const layer_t& layer)
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
      layer_t acc_layer;
  };

  template<size_t InputSize, size_t AccumulatorSize>
  void Accumulator<InputSize, AccumulatorSize>::initialise(const Board &board) {
      auto encoded = encode(board);
      for (Colour c : {WHITE, BLACK}) {
          accumulated[c] = acc_layer.forward(encoded[c]);
      }
  }

  template<size_t InputSize, size_t AccumulatorSize>
  void Accumulator<InputSize, AccumulatorSize>::make_move(const Move &move, const Colour side) {
      auto diff = increment(move, side, true);
      accumulated[side] += acc_layer.delta(diff[side]);
      accumulated[~side] += acc_layer.delta(diff[~side]);
  }

  template<size_t InputSize, size_t AccumulatorSize>
  void Accumulator<InputSize, AccumulatorSize>::unmake_move(const Move &move, const Colour side) {
      auto diff = increment(move, side, false);
      accumulated[side] += acc_layer.delta(diff[side]);
      accumulated[~side] += acc_layer.delta(diff[~side]);
  }

  template<size_t InputSize, size_t AccumulatorSize, size_t... LayerSizes>
  class Network {
  private:    
    // Recursive template to build a tuple of layers
    // Base case - just one layer left
    using _ac_t = int32_t;
    using _w_t = int16_t;
    template<size_t In, size_t Out, size_t... Rest>
    struct LayerTypes {
        // using type = std::tuple<LinearLayer<nn_t, In, Out>>;
        using type = std::tuple<QuantizedLayer<_w_t, _ac_t, nn_t, nn_t, In, Out>>;
    };
    // Recursive case - concatenate current layer with rest of layers
    template<size_t In, size_t Mid, size_t Out, size_t... Rest>
    struct LayerTypes<In, Mid, Out, Rest...> {
        using type = decltype(std::tuple_cat(
            // std::declval<std::tuple<LinearLayer<nn_t, In, Mid>>>(),
            std::declval<std::tuple<QuantizedLayer<_w_t, _ac_t, nn_t, nn_t, In, Mid>>>(),
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