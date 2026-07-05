#include "weights.hpp"

namespace Neural {
namespace generated {

alignas(32) static const uint8_t accumulator_weights[] = {
  #embed "accumulator-w.bin"
};
alignas(32) static const uint8_t accumulator_bias[] = {
  #embed "accumulator-b.bin"
};

std::unique_ptr<FixedAccumulatorLayer<N_FEATURES, N_ACCUMULATED, ACC_BITS, ACC_SHIFT>> gen_accumulator() {
    const float* w = reinterpret_cast<const float*>(accumulator_weights);
    const float* b = reinterpret_cast<const float*>(accumulator_bias);
    std::unique_ptr<FixedAccumulatorLayer<N_FEATURES, N_ACCUMULATED, ACC_BITS, ACC_SHIFT>> layer = std::make_unique<FixedAccumulatorLayer<N_FEATURES, N_ACCUMULATED, ACC_BITS, ACC_SHIFT>>(w, b);
    return layer;
}

alignas(32) static const uint8_t layer_0_weights[] = {
  #embed "layer-0-w.bin"
};
static_assert(sizeof(layer_0_weights) == 256 * 64 * sizeof(nn_t), "Layer 0 weights size mismatch");
alignas(32) static const uint8_t layer_0_bias[] = {
  #embed "layer-0-b.bin"
};
static_assert(sizeof(layer_0_bias) == 64 * sizeof(nn_t), "Layer 0 bias size mismatch");
std::unique_ptr<LinearLayer<256, 64>> gen_layer_0() {
  const float* w = reinterpret_cast<const float*>(layer_0_weights);
  const float* b = reinterpret_cast<const float*>(layer_0_bias);
  return std::make_unique<LinearLayer<256, 64>>(w, b, 8.0f);
}
alignas(32) static const uint8_t layer_1_weights[] = {
  #embed "layer-1-w.bin"
};
static_assert(sizeof(layer_1_weights) == 64 * 1 * sizeof(nn_t), "Layer 1 weights size mismatch");
alignas(32) static const uint8_t layer_1_bias[] = {
  #embed "layer-1-b.bin"
};
static_assert(sizeof(layer_1_bias) == 1 * sizeof(nn_t), "Layer 1 bias size mismatch");
std::unique_ptr<LinearLayer<64, 1>> gen_layer_1() {
  const float* w = reinterpret_cast<const float*>(layer_1_weights);
  const float* b = reinterpret_cast<const float*>(layer_1_bias);
  return std::make_unique<LinearLayer<64, 1>>(w, b, 8.0f);
}

} // namespace generated

accumulator_t get_accumulator() {
    auto layer = generated::gen_accumulator();
    return accumulator_t(std::move(layer));
}

network_t get_network() {
    network_t net;
    net.set_layer<0>(generated::gen_layer_0());
    net.set_layer<1>(generated::gen_layer_1());
    return net;
}

} // namespace Neural