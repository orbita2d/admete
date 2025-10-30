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
  static constexpr size_t In = Input;
  static constexpr size_t Out = Output;
    LinearLayer(const Matrix<T, Output, Input>& weights, const Vector<T, Output>& bias)
      : weights(weights.transpose()), bias(bias) {}
    LinearLayer(const T* weights_data, const T* bias_data) {
      for (size_t j = 0; j < Input; j++) {
        for (size_t i = 0; i < Output; i++) {
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

  private:
    Matrix<T, Input, Output> weights;
    Vector<T, Output> bias;
  };
  
  template<typename T, size_t In, size_t Out>
  class FloatingAccumulatorLayer {
    static_assert(std::is_floating_point_v<T>, "FloatingAccumulatorLayer only supports floating point types");
    public:
        using accT = float;

      FloatingAccumulatorLayer(const float* weights_l_data, const float* weights_r_data, const float* weights_k_data, const float* bias_data) {
        for (size_t k = 0; k < N_SQUARE; k++) {
          for (size_t j = 0; j < In; j++) {
            for (size_t i = 0; i < Out; i++) {
              weights_l.at(k).at(j, i) = weights_l_data[k * In * Out + j * Out + i];
              weights_r.at(k).at(j, i) = weights_r_data[k * In * Out + j * Out + i];
            }
          }
        }
        for (size_t k = 0; k < N_SQUARE; k++) {
          for (size_t i = 0; i < Out; i++) {
            weights_k.at(k, i) = weights_k_data[k * Out + i];
            weights_k.at(k + N_SQUARE, i) = weights_k_data[(k + N_SQUARE) * Out + i];
          }
        }
        for (size_t i = 0; i < Out; i++) {
          bias[i] = bias_data[i];
        }
        
      }
      
      FloatingAccumulatorLayer() = default;

      /*
        f(xl, xr) = W_l^[k_l] @ x_l + W_r^[k_r] @ x_r + b_l^[k_l] + b_r^[k_r] + b
        dy = y(x + dx) - y(x)
        dy = W_l^[k_l] @ dx_l + W_r^[k_r] @ dx_r
      
      */

      template<typename U>
      Vector<T, Out> forward(const FeatureVectorType<U, In>& xl, const FeatureVectorType<U, In>& xr) const {
        // TODO: leverage sparsity here
        Vector<T, Out> result = bias;
        const auto kl = std::get<1>(xl).get_value();
        const auto kr = std::get<1>(xr).get_value();

        for (size_t j = 0; j < In; j++) {
          for (size_t i = 0; i < Out; i++) {
            result[i] += weights_l[kl].at(j, i) * std::get<0>(xl)[j];
            result[i] += weights_r[kr].at(j, i) * std::get<0>(xr)[j];
          }
        }
        for (size_t i = 0; i < Out; i++) {
          result[i] += weights_k.at(kl, i);
          result[i] += weights_k.at(kr + N_SQUARE, i);
        }
        return result;
      }

      template<typename U>
      void increment(Vector<T, Out>& reference, const FeatureVectorType<U, In>& xl, const FeatureVectorType<U, In>& xr, const FeatureDiffType<U, In>& dl, const FeatureDiffType<U, In>& dr) const {
        // We want to concatenate the two inputs, and then propogate. This allows us to do them without copying.
        const auto kl1 = std::get<1>(dl).get_value();
        const auto kl2 = std::get<2>(dl).get_value();
        const auto kr1 = std::get<1>(dr).get_value();
        const auto kr2 = std::get<2>(dr).get_value();
        
        // if the king square has changed, it's cheaper to just do the whole thing.
        // TODO: we should do left and right separately here, only one side can move king at a time.
        if ((kl1 != kl2) || (kr1 != kr2)) {
          reference = forward(xl, xr);
          return;
        }
        
        const auto kl = std::get<1>(xl).get_value();
        const auto kr = std::get<1>(xr).get_value();

        for (const auto& [index, value] : std::get<0>(dl).data) {
          for (size_t i = 0; i < Out; i++) {
            reference[i] += weights_l.at(kl).at(index, i) * value;
          }
        }
        for (const auto& [index, value] : std::get<0>(dr).data) {
          for (size_t i = 0; i < Out; i++) {
            reference[i] += weights_r.at(kr).at(index, i) * value;
          }
        }
      }

      T& weight_l_at(size_t i, size_t j, Square k) { return weights_l.at(k.get_value()).at(j, i); }
      T& weight_r_at(size_t i, size_t j, Square k) { return weights_r.at(k.get_value()).at(j, i); }
      T& weight_k_at(size_t i, Square k) { return weights_k.at(k.get_value(), i); }
      T& bias_at(size_t i) { return bias[i]; }


      const T& weight_l_at(size_t i, size_t j, Square k) const { return weights_l.at(k.get_value()).at(j, i); }
      const T& weight_r_at(size_t i, size_t j, Square k) const { return weights_r.at(k.get_value()).at(j, i); }
      const T& weight_k_at(size_t i, Square k) const { return weights_k.at(k.get_value(), i); }
      const T& bias_at(size_t i) const { return bias[i]; }

    private:
      std::array<Matrix<T, In, Out>, N_SQUARE> weights_l; // a matrix for each king square
      std::array<Matrix<T, In, Out>, N_SQUARE> weights_r; // ^
      Matrix<T, N_SQUARE * 2, Out> weights_k; // a bias for each king square
      Vector<T, Out> bias; // final bias
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
      using layer_t = FloatingAccumulatorLayer<nn_t, FeaturesSize, AccumulatorSize>;
      static constexpr uint8_t acc_bits = 16u;
      // using layer_t = FixedAccumulatorLayer<FeaturesSize, AccumulatorSize, acc_bits, AccumulatorShift>;
      using accT = layer_t::accT;
      using floating_t = FloatingAccumulatorLayer<nn_t, FeaturesSize, AccumulatorSize>;

      Accumulator() = default;

      explicit Accumulator(std::unique_ptr<layer_t> layer)
          : acc_layer(std::move(layer)) {}
        
      explicit Accumulator(const layer_t& layer)
          : acc_layer(std::make_unique<layer_t>(layer)) {}

      // explicit Accumulator(std::unique_ptr<floating_t> layer)
      //     : acc_layer(std::make_unique<layer_t>(*layer)) {}
        
      // explicit Accumulator(const floating_t& layer)
      //     : acc_layer(std::make_unique<layer_t>(layer)) {}
      
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
              if constexpr (std::is_floating_point_v<accT>) {
                  values[i] = accumulated[c][i];
              } else {
                  values[i] = accumulated[c][i].template as<T>();
              }
          }
          return values;
      }

      per_colour<Vector<accT, AccumulatorSize>> accumulated;
      per_colour<FeatureVector> features;
    private:
      std::unique_ptr<layer_t> acc_layer;
  };

  template<size_t FeaturesSize, size_t AccumulatorSize, uint8_t AccumulatorShift>
  void Accumulator<FeaturesSize, AccumulatorSize, AccumulatorShift>::initialise(const Board &board) {
      auto encoded = encode(board);
      for (Colour c : {WHITE, BLACK}) {
          accumulated[c] = acc_layer->forward(encoded[c], encoded[~c]);
          features[c] = encoded[c];
      }
  }

  template<size_t FeaturesSize, size_t AccumulatorSize, uint8_t AccumulatorShift>
  void Accumulator<FeaturesSize, AccumulatorSize, AccumulatorShift>::make_move(const Move &move, const Colour side) {
      auto diff = increment(move, side, true);
      update(features, diff);
      acc_layer->increment(accumulated[side], features[side], features[~side], diff[side], diff[~side]);
      acc_layer->increment(accumulated[~side], features[~side], features[side], diff[~side], diff[side]);
  }

  template<size_t FeaturesSize, size_t AccumulatorSize, uint8_t AccumulatorShift>
  void Accumulator<FeaturesSize, AccumulatorSize, AccumulatorShift>::unmake_move(const Move &move, const Colour side) {
      auto diff = increment(move, side, false);
      update(features, diff);
      acc_layer->increment(accumulated[side], features[side], features[~side], diff[side], diff[~side]);
      acc_layer->increment(accumulated[~side], features[~side], features[side], diff[~side], diff[side]);
  }

  template<size_t FeaturesSize, size_t AccumulatorSize, uint8_t AccumulatorShift, size_t... LayerSizes>
  class Network {
  private:    
    // Recursive template to build a tuple of layers
    // Base case - just one layer left
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

    template<size_t I>
    void set_layer(std::unique_ptr<typename std::tuple_element_t<I, Layers>::element_type> layer) {
      static_assert(I < sizeof...(LayerSizes), "Index out of bounds");
      std::get<I>(layers) = std::move(layer);
    }
  };

} // namespace Neural