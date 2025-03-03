#pragma once
#include <types.hpp>
#include <linalg.hpp>
#include <features.hpp>
#include <algorithm>
#include <limits>

namespace Neural {
  typedef float nn_t;
  inline bool ENABLED = false;
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
      auto w = reinterpret_cast<alignas(cache_line_size) const T* __restrict__>(&weights.data);
      auto in = reinterpret_cast<alignas(cache_line_size) const T* __restrict__>(&input.data);
      auto out = reinterpret_cast<alignas(cache_line_size) T* __restrict__>(&result.data);

      #ifdef USE_AVX2
        constexpr size_t simd_block_size = 256 / sizeof(T); // 8 singles, or 4 doubles
        constexpr size_t loop_unroll = ((Output % (8 * simd_block_size) == 0) ? 8 : (Output % (4 * simd_block_size) == 0) ? 4 : (Output % (2 * simd_block_size) == 0) ? 2 : 1) * simd_block_size;
      #else
        constexpr size_t loop_unroll = 16; // 16 is basically an arbitrary guess, if we don't have any info about the CPU.
      #endif

      // On this laptop, 318k NPS as of now.
      constexpr size_t block_size = (Output % loop_unroll == 0) ? loop_unroll : 1;
      static_assert(Output % block_size == 0, "Output size must be a multiple of block size");
      // Naively, you'd expect this to be slower than the other way around, because of cache-locality. But modern CPUs are all about vectorisation and multiple independent execution units and this is really good for that.
      for (size_t j = 0; j < Input; j++) {
        for (size_t i = 0; i < Output; i+=block_size) {
          for (size_t k = 0; k < block_size; k++) {
            out[i + k] += w[j * Output + i + k] * in[j];
          }
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

        for (size_t j = 0; j< HalfIn; j++) {
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

    private:
      Matrix<T,  In, Out> weights;
      Vector<T, Out> bias;
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

  template <size_t FeaturesSize, size_t AccumulatorSize>
  class Accumulator {
  public:
      using layer_t = FloatingAccumulatorLayer<nn_t, FeaturesSize, AccumulatorSize>;

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

  template<size_t FeaturesSize, size_t AccumulatorSize>
  void Accumulator<FeaturesSize, AccumulatorSize>::initialise(const Board &board) {
      auto encoded = encode(board);
      for (Colour c : {WHITE, BLACK}) {
          accumulated[c] = acc_layer.forward(encoded[c], encoded[~c]);
      }
  }

  template<size_t FeaturesSize, size_t AccumulatorSize>
  void Accumulator<FeaturesSize, AccumulatorSize>::make_move(const Move &move, const Colour side) {
      auto diff = increment(move, side, true);
      acc_layer.increment(accumulated[side], diff[side], diff[~side]);
      acc_layer.increment(accumulated[~side], diff[~side], diff[side]);
  }

  template<size_t FeaturesSize, size_t AccumulatorSize>
  void Accumulator<FeaturesSize, AccumulatorSize>::unmake_move(const Move &move, const Colour side) {
      auto diff = increment(move, side, false);
      acc_layer.increment(accumulated[side], diff[side], diff[~side]);
      acc_layer.increment(accumulated[~side], diff[~side], diff[side]);
  }

  template<size_t FeaturesSize, size_t AccumulatorSize, size_t... LayerSizes>
  class Network {
  private:    
    // Recursive template to build a tuple of layers
    // Base case - just one layer left
    // using _ac_t = int32_t;
    // using _w_t = int16_t;
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
    using Layers = typename LayerTypes<AccumulatorSize, LayerSizes...>::type;

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

      nn_t forward(const Accumulator<FeaturesSize, AccumulatorSize>& accm, Colour us) const {
        // First handle the accumulator concatenation
        auto input = accm.get(us);
        return forward_impl(relu(input))[0];  // [0] since final layer outputs size-1 vector
    }
  };

} // namespace Neural