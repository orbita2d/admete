#pragma once
#include <types.hpp>
#include <linalg.hpp>
#include <features.hpp>
#include <algorithm>
#include <limits>
#include <memory>
#include <fixed.hpp>

namespace Neural {
  typedef float nn_t;
  // Feature vectors calculated from the initial board state
  // InitialFeatureDetectionLayer is idenditcal for each colour, a single matrix
  // Accumulator stores the output of that first layer
  // Incremental changes to the feature vectors can be added projected into the Accumulator
  // Then there's *the rest* of the neural network

  template <typename T, size_t Input, size_t Output>
  class LinearLayer {
  static_assert(std::is_floating_point_v<T>, "LinearLayer only supports arithmetic types");

  public:
    LinearLayer(const Matrix<T, Output, Input>& weights, const Vector<T, Output>& bias)
      : weights(weights.transpose()), bias(bias) {}
    LinearLayer() = default;

    Vector<T, Output> forward(const Vector<T, Input>& input) const {
      Vector<T, Output> result = bias;
      // There are no tricks to be had here, gcc will happily vectorise this loop *really* effectively with -ffast-math on.
      for (size_t j = 0; j < Input; j++) {
        for (size_t i = 0; i < Output; i++) {
          result[i] += weights.at(j, i) * input[j];
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
    Matrix<T, Input, Output> weights;
    Vector<T, Output> bias;
  };

  template<typename T, size_t HalfIn, size_t Out>
  class FloatingAccumulatorLayer {
    static_assert(std::is_floating_point_v<T>, "FloatingAccumulatorLayer only supports floating point types");
    static constexpr size_t In = HalfIn * 2;
    public:
      FloatingAccumulatorLayer(const Matrix<T, Out, In>& weights, const Vector<T, Out>& bias)
        : weights(weights.transpose()), bias(bias) {}
      
      FloatingAccumulatorLayer() = default;

      Vector<T, Out> forward(const Vector<T, HalfIn>& input_left, const Vector<T, HalfIn>& input_right) const {
        Vector<T, Out> result = bias;

        for (size_t j = 0; j < HalfIn; j++) {
          for (size_t i = 0; i < Out; i++) {
            result[i] += weights.at(j, i) * input_left[j];
            result[i] += weights.at(j + HalfIn, i) * input_right[j];
          }
        }
        return result;
      }

      void increment(Vector<T, Out>& reference, const SparseVector<T, HalfIn>& input_left, const SparseVector<T, HalfIn>& input_right) const {
        // We want to concatenate the two inputs, and then propogate. This allows us to do them without copying.
        for (const auto& [index, value] : input_left.data) {
          for (size_t i = 0; i < Out; i++) {
            reference[i] += weights.at(index, i) * value;
          }
        }
        for (const auto& [index, value] : input_right.data) {
          for (size_t i = 0; i < Out; i++) {
            reference[i] += weights.at(index + HalfIn, i) * value;
          }
        }
      }

      T& weight_at(size_t i, size_t j) { return weights.at(j, i); }
      T& bias_at(size_t i) { return bias[i]; }

    private:
      Matrix<T,  In, Out> weights;
      Vector<T, Out> bias;
  };

  template<size_t HalfIn, size_t Out, uint8_t acc_bits, int8_t acc_scale_shift>
  class FixedAccumulatorLayer {
    public:
    using inT = feature_t;
    static_assert(std::is_integral_v<inT>, "FixedAccumulatorLayer only supports integral types");
    using accT = Fixed<acc_bits, acc_scale_shift>;
    static constexpr size_t In = HalfIn * 2;

    FixedAccumulatorLayer() = default; 

    template<typename floatT>
    FixedAccumulatorLayer(FloatingAccumulatorLayer<floatT, HalfIn, Out>& layer) {
        for (size_t j = 0; j < In; j++) {
            for (size_t i = 0; i < Out; i++) {
                floatT float_weight = layer.weight_at(i, j);
                weights.at(j, i) = accT(float_weight);
            }
        }
        
        for (size_t i = 0; i < Out; i++) {
            floatT float_bias = layer.bias_at(i);
            bias[i] = accT(float_bias);
        }
    }

    Vector<accT, Out> forward(const Vector<inT, HalfIn>& input_left, const Vector<inT, HalfIn>& input_right) const {
      Vector<accT, Out> result = bias;

      for (size_t j = 0; j < HalfIn; j++) {
        for (size_t i = 0; i < Out; i++) {
          result[i] += weights.at(j, i).small_multiply(input_left[j]);
          result[i] += weights.at(j + HalfIn, i).small_multiply(input_right[j]);
        }
      }
      return result;
    }

    void increment(Vector<accT, Out>& reference, const SparseVector<inT, HalfIn>& input_left, const SparseVector<inT, HalfIn>& input_right) const {
      // We want to concatenate the two inputs, and then propogate. This allows us to do them without copying.
      for (const auto& [index, value] : input_left.data) {
        for (size_t i = 0; i < Out; i++) {
          reference[i] += weights.at(index, i).small_multiply(value);
        }
      }
      for (const auto& [index, value] : input_right.data) {
        for (size_t i = 0; i < Out; i++) {
          reference[i] += weights.at(index + HalfIn, i).small_multiply(value);
        }
      }
    }

    private:
      Matrix<accT, In, Out> weights;
      Vector<accT, Out> bias;
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

  template <typename T, size_t N>
  void relu_inplace(Vector<T, N>& x) {
    for (size_t i = 0; i < N; i++) {
      x[i] = relu(x[i]);
    }
  }

  template <size_t FeaturesSize, size_t AccumulatorSize, uint8_t AccumulatorShift>
  class Accumulator {
  public:
      // using layer_t = FloatingAccumulatorLayer<nn_t, FeaturesSize, AccumulatorSize>;
      using layer_t = FixedAccumulatorLayer<FeaturesSize, AccumulatorSize, 32, AccumulatorShift>;
      using accT = layer_t::accT;
      using floating_t = FloatingAccumulatorLayer<nn_t, FeaturesSize, AccumulatorSize>;

      Accumulator() = default;

      explicit Accumulator(std::unique_ptr<layer_t> layer)
          : acc_layer(std::move(layer)) {}
        
      explicit Accumulator(const layer_t& layer)
          : acc_layer(std::make_unique<layer_t>(layer)) {}

      explicit Accumulator(std::unique_ptr<floating_t> layer)
          : acc_layer(std::make_unique<layer_t>(*layer)) {}
        
      explicit Accumulator(const floating_t& layer)
          : acc_layer(std::make_unique<layer_t>(layer)) {}
      
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
      const Vector<accT, AccumulatorSize>& get(Colour c) const { return accumulated[c]; }

      template<typename T>
      const Vector<T, AccumulatorSize> get_as(Colour c) const {
          auto values = Vector<T, AccumulatorSize>::zeros();
          for (size_t i = 0; i < AccumulatorSize; i++) {
              values[i] = accumulated[c][i].template as<T>();
          }
          return values;
      }

    private:
      per_colour<Vector<accT, AccumulatorSize>> accumulated;
      std::unique_ptr<layer_t> acc_layer;
  };

  template<size_t FeaturesSize, size_t AccumulatorSize, uint8_t AccumulatorShift>
  void Accumulator<FeaturesSize, AccumulatorSize, AccumulatorShift>::initialise(const Board &board) {
      auto encoded = encode(board);
      for (Colour c : {WHITE, BLACK}) {
          accumulated[c] = acc_layer->forward(encoded[c], encoded[~c]);
      }
  }

  template<size_t FeaturesSize, size_t AccumulatorSize, uint8_t AccumulatorShift>
  void Accumulator<FeaturesSize, AccumulatorSize, AccumulatorShift>::make_move(const Move &move, const Colour side) {
      auto diff = increment(move, side, true);
      acc_layer->increment(accumulated[side], diff[side], diff[~side]);
      acc_layer->increment(accumulated[~side], diff[~side], diff[side]);
  }

  template<size_t FeaturesSize, size_t AccumulatorSize, uint8_t AccumulatorShift>
  void Accumulator<FeaturesSize, AccumulatorSize, AccumulatorShift>::unmake_move(const Move &move, const Colour side) {
      auto diff = increment(move, side, false);

      acc_layer->increment(accumulated[side], diff[side], diff[~side]);
      acc_layer->increment(accumulated[~side], diff[~side], diff[side]);
  }

  template<size_t FeaturesSize, size_t AccumulatorSize, uint8_t AccumulatorShift, size_t... LayerSizes>
  class Network {
  private:    
    // Recursive template to build a tuple of layers
    // Base case - just one layer left
    // using _ac_t = int32_t;
    // using _w_t = int16_t;
    template<size_t In, size_t Out, size_t... Rest>
    struct LayerTypes {
        using type = std::tuple<std::unique_ptr<LinearLayer<nn_t, In, Out>>>;
    };
    // Recursive case - concatenate current layer with rest of layers
    template<size_t In, size_t Mid, size_t Out, size_t... Rest>
    struct LayerTypes<In, Mid, Out, Rest...> {
        using type = decltype(std::tuple_cat(
            std::declval<std::tuple<std::unique_ptr<LinearLayer<nn_t, In, Mid>>>>(),
            std::declval<typename LayerTypes<Mid, Out, Rest...>::type>()
        ));
    };
    // Tuple of all the layers, excluding the accumulator layer
    using Layers = typename LayerTypes<AccumulatorSize, LayerSizes...>::type;

    // Helper for recursive forward pass
    template<size_t I = 0>
    auto forward_impl(const auto& input) const {
        if constexpr (I == sizeof...(LayerSizes) - 1) {
            // Base case - last layer
            return std::get<I>(layers)->forward(input);
        } else {
            // Recursive case - apply layer, relu, then continue
            return forward_impl<I + 1>(relu(std::get<I>(layers)->forward(input)));
        }
    }

    public:
      Layers layers;

      nn_t forward(const Accumulator<FeaturesSize, AccumulatorSize, AccumulatorShift>& accm, Colour us) const {
        auto input = accm.template get_as<nn_t>(us);
        return forward_impl(relu(input))[0];  // [0] since final layer outputs size-1 vector
    }
  };

} // namespace Neural