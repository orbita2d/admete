#include "types.hpp"
#include <unordered_map>

namespace GameCache {

// Cache for pawn structure evals which are expensive to compute every time.
struct PawnCacheElem {
    PawnCacheElem() = default;
    PawnCacheElem(const Score score) : score_op(score.opening_score), score_eg(score.endgame_score){};
    Score eval() const { return Score(score_op, score_eg); }

  private:
    int16_t score_op = 0;
    int16_t score_eg = 0;
};

typedef std::pair<zobrist_t, PawnCacheElem> tt_pair;
typedef std::unordered_map<zobrist_t, PawnCacheElem> tt_map;

// 1MiB cache for pawn sturcture evals.
constexpr size_t cache_max = (1 * (1 << 20)) / sizeof(tt_pair);

class PawnCache {
  public:
    PawnCache() = default;
    bool probe(const zobrist_t, PawnCacheElem &hit);
    void store(const zobrist_t hash, const Score score);
    void replace(const size_t index, const zobrist_t new_hash, const PawnCacheElem elem);
    void clear() {
        _data.clear();
        index = 0ul;
    }
    depth_t min_depth() { return _min_depth; }
    void min_depth(depth_t d) { _min_depth = d; }
    bool is_enabled() { return enabled; }
    void enable() { enabled = true; }
    void disable() { enabled = false; }
    void set_delete();

  private:
    tt_map _data;
    depth_t _min_depth = 0;
    size_t index = 0;
    bool enabled = true;
    std::array<zobrist_t, cache_max> key_array;
};
inline PawnCache pawn_cache;
void init();
}