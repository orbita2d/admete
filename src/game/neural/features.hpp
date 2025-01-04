#pragma once
#include <vector>
#include <types.hpp>
#include <bitboard.hpp>
#include <linalg.hpp>

class Board;  // Forward declare

namespace Neural {
  // roughly the idea here is to encode the boart state into a vector for features for each color
  inline constexpr size_t N_FEATURES = 64 * 6; // a bitboard for each piece type
  typedef Vector<int, N_FEATURES> FeatureVector;
  typedef SparseVector<int, N_FEATURES> FeatureDiff;

  // We can keep these functions pure for now. So let's do that.

  per_colour<FeatureVector> encode(const Board &board);
  per_colour<FeatureDiff> increment(const Move &move, const Colour us, const bool forward);

  
} // namespace Neural