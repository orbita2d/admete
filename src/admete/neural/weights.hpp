#pragma once
#include <memory>

#include <network.hpp>

namespace Neural {

// Model generated from training run 2025-04-29_23-12-44

constexpr size_t N_ACCUMULATED = 128;
static_assert(N_FEATURES == 384, "Feature size mismatch");

namespace generated {

std::unique_ptr<FloatingAccumulatorLayer<nn_t, N_FEATURES, N_ACCUMULATED>> gen_accumulator();

std::unique_ptr<LinearLayer<nn_t, 128, 64>> gen_layer_0();
std::unique_ptr<LinearLayer<nn_t, 64, 1>> gen_layer_1();

} // namespace generated

typedef Accumulator<N_FEATURES, N_ACCUMULATED> accumulator_t;
accumulator_t get_accumulator();

typedef Network<N_FEATURES, N_ACCUMULATED, 64, 1> network_t;
network_t get_network();

} // namespace Neural
