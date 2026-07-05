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
  
  [[gnu::always_inline]] inline __m256i dpbusd(__m256i a, __m256i b, __m256i c) {
    // DPUSD is an AVX-VNNI instruction for a dot product over 8bit integers (unsigned, signed), accumulated into 32bit integers.
    // It's quite quick. 
    #if defined(__AVXVNNI__)
      return _mm256_dpbusd_avx_epi32(a, b, c);
    #else
      // AVX2 emulation of dpbusd
      // ~ 10% performance hit.
      auto p = _mm256_maddubs_epi16(b, c);
      return _mm256_add_epi32(a, _mm256_madd_epi16(p, _mm256_set1_epi16(1)));
    #endif
  }

  template <size_t Input, size_t Output>
  class LinearLayer {
  typedef int8_t weight_T;
  typedef int32_t acc_T;

  public:
  static constexpr size_t In = Input;
  static constexpr size_t Out = Output;

    LinearLayer(const float* weights_data, const float* bias_data, const float rng_x)  {
      auto w_span = std::span<const float>(weights_data, Input * Output);
      auto max_abs_w = std::ranges::max(w_span, {}, [](const float x) { return std::abs(x); });
      scale_w = std::abs(max_abs_w) / 127.0f;
      scale_x = rng_x / 255.0f;
      for (size_t j = 0; j < Input; j++) {
        for (size_t i = 0; i < Output; i++) {
          weights.at(i, j) = static_cast<int8_t>(std::clamp(std::lround(weights_data[j * Output + i] / scale_w), -127L, 127L));;
        }
      }
      for (size_t i = 0; i < Output; i++) {
        bias[i] = static_cast<float>(bias_data[i]);
      }
    }

    LinearLayer() = default;

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
            for (size_t k = 0; k < 8; k++) {
              auto w = _mm256_load_si256((const __m256i*)&weights.data[(i+k) * Input + j]);
              acc[k] = dpbusd(acc[k], x, w);
            }
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
            acc = dpbusd(acc, x, w);
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

  template<size_t HalfIn, size_t Out, uint8_t acc_bits, int8_t acc_scale_shift>
  class FixedAccumulatorLayer {
    public:
    using inT = feature_t;
    static_assert(std::is_integral_v<inT>, "FixedAccumulatorLayer only supports integral types");
    using accT = Fixed<acc_bits, acc_scale_shift>;
    static constexpr size_t In = HalfIn * 2;

    FixedAccumulatorLayer() = default; 

    FixedAccumulatorLayer(const float* weights_data, const float* bias_data) {
      for (size_t j = 0; j < In; j++) {
        for (size_t i = 0; i < Out; i++) {
          weights.at(j, i) = accT::from_float(weights_data[j * Out + i]);
        }
      }
      for (size_t i = 0; i < Out; i++) {
        bias[i] = accT::from_float(bias_data[i]);
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

  template <size_t FeaturesSize, size_t AccumulatorSize, uint8_t AccumulatorBits, uint8_t AccumulatorShift>
  class Accumulator {
  public:
      using layer_t = FixedAccumulatorLayer<FeaturesSize, AccumulatorSize, AccumulatorBits, AccumulatorShift>;
      using accT = layer_t::accT;

      Accumulator() = default;

      explicit Accumulator(std::unique_ptr<layer_t> layer)
          : acc_layer(std::move(layer)) {}
        
      explicit Accumulator(const layer_t& layer)
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

  template<size_t FeaturesSize, size_t AccumulatorSize, uint8_t AccumulatorBits, uint8_t AccumulatorShift>
  void Accumulator<FeaturesSize, AccumulatorSize, AccumulatorBits, AccumulatorShift>::initialise(const Board &board) {
      auto encoded = encode(board);
      for (Colour c : {WHITE, BLACK}) {
          accumulated[c] = acc_layer->forward(encoded[c], encoded[~c]);
      }
  }

  template<size_t FeaturesSize, size_t AccumulatorSize, uint8_t AccumulatorBits, uint8_t AccumulatorShift>
  void Accumulator<FeaturesSize, AccumulatorSize, AccumulatorBits, AccumulatorShift>::make_move(const Move &move, const Colour side) {
      auto diff = increment(move, side, true);
      acc_layer->increment(accumulated[side], diff[side], diff[~side]);
      acc_layer->increment(accumulated[~side], diff[~side], diff[side]);
  }

  template<size_t FeaturesSize, size_t AccumulatorSize, uint8_t AccumulatorBits, uint8_t AccumulatorShift>
  void Accumulator<FeaturesSize, AccumulatorSize, AccumulatorBits, AccumulatorShift>::unmake_move(const Move &move, const Colour side) {
      auto diff = increment(move, side, false);

      acc_layer->increment(accumulated[side], diff[side], diff[~side]);
      acc_layer->increment(accumulated[~side], diff[~side], diff[side]);
  }

  template<size_t FeaturesSize, size_t AccumulatorSize, uint8_t AccumulatorBits, uint8_t AccumulatorShift, size_t... LayerSizes>
  class Network {
  private:    
    // Recursive template to build a tuple of layers
    // Base case - just one layer left
    template<size_t In, size_t Out, size_t... Rest>
    struct LayerTypes {
        using type = std::tuple<std::unique_ptr<LinearLayer<In, Out>>>;
    };
    // Recursive case - concatenate current layer with rest of layers
    template<size_t In, size_t Mid, size_t Out, size_t... Rest>
    struct LayerTypes<In, Mid, Out, Rest...> {
        using type = decltype(std::tuple_cat(
            std::declval<std::tuple<std::unique_ptr<LinearLayer<In, Mid>>>>(),
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

      nn_t forward(const Accumulator<FeaturesSize, AccumulatorSize, AccumulatorBits, 
        AccumulatorShift>& accm, Colour us) const {
        auto input = accm.template get_as<nn_t>(us);
        return forward_impl(relu(input))[0];  // [0] since final layer outputs size-1 vector
    }

    template<size_t I>
    void set_layer(std::unique_ptr<typename std::tuple_element_t<I, Layers>::element_type> layer) {
      static_assert(I < sizeof...(LayerSizes), "Index out of bounds");
      std::get<I>(layers) = std::move(layer);
    }
  };

} // namespace Neural