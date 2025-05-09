#pragma once
#include <vector>
#include <types.hpp>
#include <bitboard.hpp>
#include <linalg.hpp>
#include <tuple>

class Board;  // Forward declare

namespace Neural {
  // roughly the idea here is to encode the boart state into a vector for features for each color
  inline constexpr size_t N_FEATURES = 64 * 6; // a bitboard for each piece type (for each color)
  inline constexpr size_t N_FEATURES2 = 64 * 5; // a bitboard for each piece type except the king (for each color)
  using feature_t = int8_t;
  
  typedef Vector<feature_t, N_FEATURES> FeatureVector;
  typedef SparseVector<feature_t, N_FEATURES> FeatureDiff;
  typedef std::tuple<Vector<feature_t, N_FEATURES2>, Square> Feature2Vector;
  typedef std::tuple<SparseVector<feature_t, N_FEATURES2>, Square, Square> Feature2Diff;
  // We can keep these functions pure for now. So let's do that.

  per_colour<FeatureVector> encode(const Board &board);
  per_colour<Feature2Vector> encode2(const Board &board);
  per_colour<FeatureDiff> increment(const Move &move, const Colour us, const bool forward);

  
} // namespace Neural