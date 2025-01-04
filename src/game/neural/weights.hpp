#pragma once
#include <network.hpp>

namespace Neural {

constexpr size_t N_ACCUMULATED = 256;
static_assert(N_FEATURES == 384, "Feature size mismatch");

namespace generated {

LinearLayer<nn_t, 384, 256> gen_accumulator();

LinearLayer<nn_t, 512, 128> gen_layer_0();
LinearLayer<nn_t, 128, 1> gen_layer_1();

} // namespace generated

typedef Accumulator<N_FEATURES, N_ACCUMULATED> accumulator_t;
accumulator_t get_accumulator();

typedef Network<N_FEATURES, N_ACCUMULATED, 128, 1> network_t;
network_t get_network();

} // namespace Neural
