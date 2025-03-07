#pragma once
#include <memory>

#include <network.hpp>

namespace Neural {

// Model generated from training run 2025-03-01_18-47-33

constexpr size_t N_ACCUMULATED = 128;
static_assert(N_FEATURES == 384, "Feature size mismatch");

namespace generated {

std::unique_ptr<FloatingAccumulatorLayer<nn_t, N_FEATURES, N_ACCUMULATED>> gen_accumulator();

LinearLayer<nn_t, 128, 64> gen_layer_0();
LinearLayer<nn_t, 64, 1> gen_layer_1();

} // namespace generated

typedef Accumulator<N_FEATURES, N_ACCUMULATED> accumulator_t;
accumulator_t get_accumulator();

typedef Network<N_FEATURES, N_ACCUMULATED, 64, 1> network_t;
network_t get_network();

} // namespace Neural
