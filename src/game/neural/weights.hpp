#pragma once
#include <network.hpp>

namespace Neural {

constexpr size_t N_ACCUMULATED = 256;
static_assert(N_FEATURES == 384, "Feature size mismatch");

namespace generated {

typedef int32_t acc_t;
QuantizedAccumulatorLayer<int16_t, acc_t, nn_t, nn_t, N_FEATURES, N_ACCUMULATED> gen_accumulator();

QuantizedLayer<int16_t, acc_t, nn_t, nn_t, 512, 128> gen_layer_0();
QuantizedLayer<int16_t, acc_t, nn_t, nn_t, 128, 1> gen_layer_1();

} // namespace generated

typedef Accumulator<N_FEATURES, N_ACCUMULATED> accumulator_t;
accumulator_t get_accumulator();

typedef Network<N_FEATURES, N_ACCUMULATED, 128, 1> network_t;
network_t get_network();

} // namespace Neural
