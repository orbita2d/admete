#pragma once
#include <memory>

#include <network.hpp>

namespace Neural {

// Model generated from training run marvelous-quail-309 (king features concatenated into per-side block)

constexpr uint8_t ACC_SHIFT = 4;
constexpr uint8_t ACC_BITS = 16u;
constexpr size_t N_ACCUMULATED = 256;
constexpr nn_t LOGISTIC_SCALING = static_cast<nn_t>(400.0);
static_assert(N_FEATURES == 384, "Feature size mismatch");

namespace generated {

  //template<size_t HalfIn, size_t Out, uint8_t acc_bits, int8_t acc_scale_shift>
std::unique_ptr<FixedAccumulatorLayer<N_FEATURES, N_ACCUMULATED, ACC_BITS, ACC_SHIFT>> gen_accumulator();

std::unique_ptr<LinearLayer<256, 64>> gen_layer_0();
std::unique_ptr<LinearLayer<64, 1>> gen_layer_1();

} // namespace generated

typedef Accumulator<N_FEATURES, N_ACCUMULATED, ACC_BITS, ACC_SHIFT> accumulator_t;
accumulator_t get_accumulator();

typedef Network<N_FEATURES, N_ACCUMULATED, ACC_BITS, ACC_SHIFT, 64, 1> network_t;
network_t get_network();

} // namespace Neural