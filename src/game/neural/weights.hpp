#pragma once
#include <network.hpp>

namespace Neural {

constexpr size_t N_ACCUMULATED = 128;
static_assert(N_FEATURES == 384, "Feature size mismatch");

namespace generated {

typedef nn_t acc_t;
FloatingAccumulatorLayer<nn_t, N_FEATURES, N_ACCUMULATED> gen_accumulator();

LinearLayer<nn_t, 256, 128> gen_layer_0();
LinearLayer<nn_t, 128, 1> gen_layer_1();

} // namespace generated

typedef Accumulator<N_FEATURES, N_ACCUMULATED> accumulator_t;
accumulator_t get_accumulator();

typedef Network<N_FEATURES, N_ACCUMULATED, 128, 1> network_t;
network_t get_network();

} // namespace Neural
