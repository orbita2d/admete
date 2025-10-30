#pragma once
#include <vector>
#include <types.hpp>
#include <bitboard.hpp>
#include <linalg.hpp>
#include <tuple>

class Board;  // Forward declare

namespace Neural {
  // roughly the idea here is to encode the boart state into a vector for features for each color
  inline constexpr size_t N_FEATURES = 64 * 5; // a bitboard for each piece type except the king (for each color)
  using feature_t = int8_t;

  template<typename T, size_t N>
  using FeatureVectorType = std::tuple<Vector<T, N>, Square>;
  template<typename T, size_t N>
  using FeatureDiffType = std::tuple<SparseVector<T, N>, Square, Square>;
  
  typedef FeatureVectorType<feature_t, N_FEATURES> FeatureVector;
  typedef FeatureDiffType<feature_t, N_FEATURES> FeatureDiff;
  // We can keep these functions pure for now. So let's do that.

  per_colour<FeatureVector> encode(const Board &board);
  per_colour<FeatureDiff> increment(const Move &move, const Colour us, const bool forward);
  void update(per_colour<FeatureVector>& features, const per_colour<FeatureDiff>& diffs);
  
} // namespace Neural