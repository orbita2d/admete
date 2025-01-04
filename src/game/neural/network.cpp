#include "network.hpp"

void Neural::Accumulator::initialise(const Board &board) {
    auto encoded = encode(board);
    for (Colour c : {WHITE, BLACK}) {
        accumulated[c] = network.accumulator_layer.forward(encoded[c]);
    }
}

void Neural::Accumulator::make_move(const Move &move, const Colour side) {
    auto diff = increment(move, side, true);
    accumulated[side] += network.accumulator_layer.delta(diff[side]);
    accumulated[~side] += network.accumulator_layer.delta(diff[~side]);
}

void Neural::Accumulator::unmake_move(const Move &move, const Colour side) {
    auto diff = increment(move, side, false);
    accumulated[side] += network.accumulator_layer.delta(diff[side]);
    accumulated[~side] += network.accumulator_layer.delta(diff[~side]);
}