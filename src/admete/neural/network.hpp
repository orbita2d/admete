#pragma once
#include <types.hpp>
#include <linalg.hpp>
#include <features.hpp>
#include <algorithm>
#include <limits>
#include <memory>
#include <fixed.hpp>
#include <immintrin.h>
#include <span>

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
  static constexpr size_t In = Input;
  static constexpr size_t Out = Output;
  typedef T value_type;
    LinearLayer(const Matrix<T, Output, Input>& weights, const Vector<T, Output>& bias)
      : weights(weights.transpose()), bias(bias) {}
    LinearLayer(const T* weights_data, const T* bias_data) {
      for (size_t i = 0; i < Output; i++) {
      for (size_t j = 0; j < Input; j++) {
          weights.at(j, i) = weights_data[j * Output + i];
        }
      }
      for (size_t i = 0; i < Output; i++) {
        bias[i] = bias_data[i];
      }
    }
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

    T bias_at(size_t i) const { return bias[i]; }
    T& bias_at(size_t i) { return bias[i]; }
    T weight_at(size_t i, size_t j) const { return weights.at(j, i); }
    T& weight_at(size_t i, size_t j) { return weights.at(j, i); }
    T* weights_data() { return weights.data; }
    const T* weights_data() const { return weights.data; }

  private:
    Matrix<T, Input, Output> weights;
    Vector<T, Output> bias;
  };

  template <size_t Input, size_t Output>
  class QuantisedLinearLayer {
  typedef int8_t weight_T;
  typedef int32_t acc_T;

  public:
  static constexpr size_t In = Input;
  static constexpr size_t Out = Output;
  typedef LinearLayer<float, Input, Output> float_layer_t;

    QuantisedLinearLayer(const Matrix<weight_T, Output, Input>& weights, const Vector<float, Output>& bias, const float scale_x, const float sca, const float scale_wle_w) : weights(weights), bias(bias), scale_x(scale_x), scale_w(scale_w) {}

    QuantisedLinearLayer(const float_layer_t& layer, const float rng_x)  {
      auto w_span = std::span<const typename float_layer_t::value_type>(layer.weights_data(), Input * Output);
      auto max_abs_w = std::ranges::max(w_span, {}, [](const float_layer_t::value_type& x) { return std::abs(x); });
      scale_w = std::abs(max_abs_w) / 127.0f;
      scale_x = rng_x / 255.0f;
      for (size_t j = 0; j < Input; j++) {
        for (size_t i = 0; i < Output; i++) {
          weights.at(i, j) = static_cast<int8_t>(std::clamp(std::lround(layer.weight_at(i, j) / scale_w), -127L, 127L));;
        }
      }
      for (size_t i = 0; i < Output; i++) {
        bias[i] = static_cast<float>(layer.bias_at(i));
      }
    }

    QuantisedLinearLayer() = default;

    Vector<float, Output> forward(const Vector<float, Input>& input) const {
      auto quantised_input = Vector<int8_t, Input>::zeros();
      static_assert(Input % 32 == 0, "Input size must be a multiple of 32 for AVX2");
      {
        const __m256 inv = _mm256_set1_ps(1.0f / scale_x);
        const __m256i max255 = _mm256_set1_epi8(255);
        // packs/packus interleave the two 128-bit lanes; this restores natural element order.
        const __m256i lane_fix = _mm256_setr_epi32(0, 4, 1, 5, 2, 6, 3, 7);
        for (size_t j = 0; j < Input; j += 32) {
          auto a = _mm256_cvtps_epi32(_mm256_mul_ps(_mm256_load_ps(&input.data[j +  0]), inv));
          auto b = _mm256_cvtps_epi32(_mm256_mul_ps(_mm256_load_ps(&input.data[j +  8]), inv));
          auto c = _mm256_cvtps_epi32(_mm256_mul_ps(_mm256_load_ps(&input.data[j + 16]), inv));
          auto d = _mm256_cvtps_epi32(_mm256_mul_ps(_mm256_load_ps(&input.data[j + 24]), inv));
          auto bytes = _mm256_packus_epi16(_mm256_packs_epi32(a, b), _mm256_packs_epi32(c, d)); // saturates negatives to 0
          bytes = _mm256_min_epu8(bytes, max255);
          bytes = _mm256_permutevar8x32_epi32(bytes, lane_fix);
          _mm256_store_si256((__m256i*)&quantised_input.data[j], bytes);
        }
      }
      // Reduce four dpbusd accumulators to one i32 dot-product per lane: [out0,out1,out2,out3].
      auto haddx4 = [](__m256i a, __m256i b, __m256i c, __m256i d) {
        auto s = _mm256_hadd_epi32(_mm256_hadd_epi32(a, b), _mm256_hadd_epi32(c, d));
        return _mm_add_epi32(_mm256_castsi256_si128(s), _mm256_extracti128_si256(s, 1));
      };
      if constexpr (Output % 8 == 0) {
        auto result = bias;
        const __m256 scale = _mm256_set1_ps(scale_x * scale_w);
        for (size_t i = 0; i < Output; i+=8) {
          __m256i acc[8];
          for (size_t k = 0; k < 8; k++) acc[k] = _mm256_setzero_si256();
          for (size_t j = 0; j < Input; j+= 32) {
            auto x =  _mm256_load_si256((const __m256i*)&quantised_input.data[j]);
            for (size_t k = 0; k < 8; k++)
              acc[k] = _mm256_dpbusd_avx_epi32(acc[k], x, _mm256_load_si256((const __m256i*)&weights.data[(i+k) * Input + j]));
          }
          __m256i sums = _mm256_set_m128i(haddx4(acc[4], acc[5], acc[6], acc[7]),
                                          haddx4(acc[0], acc[1], acc[2], acc[3]));
          __m256 scaled = _mm256_mul_ps(_mm256_cvtepi32_ps(sums), scale);
          _mm256_storeu_ps(&result[i], _mm256_add_ps(_mm256_loadu_ps(&result[i]), scaled));
        }
        return result;
      } else {
        // one output at a time, still vectorised for the input
        // TODO: clean this all up.
        auto result = bias;
        for (size_t i = 0; i < Output; i++) {
          auto acc = _mm256_setzero_si256();
          for (size_t j = 0; j < Input; j+= 32) {
            auto x =  _mm256_load_si256((const __m256i*)&quantised_input.data[j]);
            auto w = _mm256_load_si256((const __m256i*)&weights.data[i * Input + j]);
            acc = _mm256_dpbusd_avx_epi32(acc, x, w);
          }
          __m128i s = _mm_add_epi32(_mm256_castsi256_si128(acc), _mm256_extracti128_si256(acc, 1));
          s = _mm_hadd_epi32(s, s);
          s = _mm_hadd_epi32(s, s);
          auto acc_i32 = _mm_cvtsi128_si32(s);
          result[i] += static_cast<float>(acc_i32) * scale_x * scale_w;
        }
        return result;
      }
    }

  private:
    Matrix<weight_T, Output, Input> weights;
    Vector<float, Output> bias;
    float scale_x;
    float scale_w;
  };  

  template<typename T, size_t HalfIn, size_t Out>
  class FloatingAccumulatorLayer {
    static_assert(std::is_floating_point_v<T>, "FloatingAccumulatorLayer only supports floating point types");
    static constexpr size_t In = HalfIn * 2;
    public:
      FloatingAccumulatorLayer(const Matrix<T, Out, In>& weights, const Vector<T, Out>& bias)
        : weights(weights.transpose()), bias(bias) {}

      FloatingAccumulatorLayer(const float* weights_data, const float* bias_data) {
        for (size_t j = 0; j < In; j++) {
          for (size_t i = 0; i < Out; i++) {
            weights.at(j, i) = weights_data[j * Out + i];
          }
        }
        for (size_t i = 0; i < Out; i++) {
          bias[i] = bias_data[i];
        }
        
      }
      
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

      const T& weight_at(size_t i, size_t j) const { return weights.at(j, i); }
      const T& bias_at(size_t i) const { return bias[i]; }

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
    FixedAccumulatorLayer(const FloatingAccumulatorLayer<floatT, HalfIn, Out>& layer) {
        for (size_t j = 0; j < In; j++) {
            for (size_t i = 0; i < Out; i++) {
                floatT float_weight = layer.weight_at(i, j);
                weights.at(j, i) = accT::from_float(float_weight);
            }
        }
        
        for (size_t i = 0; i < Out; i++) {
            floatT float_bias = layer.bias_at(i);
            bias[i] = accT::from_float(float_bias);
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

  template <size_t FeaturesSize, size_t AccumulatorSize, uint8_t AccumulatorShift>
  class Accumulator {
  public:
      // using layer_t = FloatingAccumulatorLayer<nn_t, FeaturesSize, AccumulatorSize>;
      static constexpr uint8_t acc_bits = 16u;
      using layer_t = FixedAccumulatorLayer<FeaturesSize, AccumulatorSize, acc_bits, AccumulatorShift>;
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
    template<size_t In, size_t Out, size_t... Rest>
    struct LayerTypes {
        using type = std::tuple<std::unique_ptr<QuantisedLinearLayer<In, Out>>>;
    };
    // Recursive case - concatenate current layer with rest of layers
    template<size_t In, size_t Mid, size_t Out, size_t... Rest>
    struct LayerTypes<In, Mid, Out, Rest...> {
        using type = decltype(std::tuple_cat(
            std::declval<std::tuple<std::unique_ptr<QuantisedLinearLayer<In, Mid>>>>(),
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

    template<size_t I>
    void set_layer(std::unique_ptr<typename std::tuple_element_t<I, Layers>::element_type::float_layer_t> layer) {
      static_assert(I < sizeof...(LayerSizes), "Index out of bounds");
      // std::get<I>(layers) = std::move(layer);
      std::get<I>(layers) = std::make_unique<typename std::tuple_element_t<I, Layers>::element_type>(std::move(*layer), 8.0f); // Let's see if it compiles. aside, normalised to ~ N(0, 1)
    }
  };

} // namespace Neural