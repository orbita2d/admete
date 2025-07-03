#pragma once
#include <memory>

#include <network.hpp>

namespace Neural {

// Model generated from training run 2025-06-09_06-41-10

constexpr uint8_t ACC_SHIFT = 4;
constexpr size_t N_ACCUMULATED = 256;
constexpr nn_t LOGISTIC_SCALING = 400.0f;
static_assert(N_FEATURES == 384, "Feature size mismatch");

namespace generated {

std::unique_ptr<FloatingAccumulatorLayer<nn_t, N_FEATURES, N_ACCUMULATED>> gen_accumulator();

std::unique_ptr<LinearLayer<nn_t, 256, 64>> gen_layer_0();
std::unique_ptr<LinearLayer<nn_t, 64, 1>> gen_layer_1();

} // namespace generated

typedef Accumulator<N_FEATURES, N_ACCUMULATED, ACC_SHIFT> accumulator_t;
accumulator_t get_accumulator();

typedef Network<N_FEATURES, N_ACCUMULATED, ACC_SHIFT, 64, 1> network_t;
network_t get_network();

} // namespace Neural
