#include "types.hpp"

namespace GameCache {

// Cache for pawn structure evals which are expensive to compute every time.
struct PawnCacheElem {
    PawnCacheElem() = default;
    PawnCacheElem(const zobrist_t hash, const Score score)
        : _hash(hash), score_op(score.opening_score), score_eg(score.endgame_score){};
    Score eval() const { return Score(score_op, score_eg); }
    zobrist_t hash() const { return _hash; }

  private:
    zobrist_t _hash = 0;
    int16_t score_op = 0;
    int16_t score_eg = 0;
};

// 1MiB cache for pawn sturcture evals.
constexpr size_t cache_max = (1 * (1 << 20)) / sizeof(PawnCacheElem);

class PawnCache {
  public:
    PawnCache();
    bool probe(const zobrist_t, PawnCacheElem &hit);
    void store(const zobrist_t hash, const Score score);
    void clear() { _data.clear(); }
    bool is_enabled() { return enabled; }
    void enable() { enabled = true; }
    void disable() { enabled = false; }
    void set_delete();

  private:
    std::vector<PawnCacheElem> _data;
    size_t max_index;
    zobrist_t bitmask;
    bool enabled = true;
};
inline PawnCache pawn_cache;
void init();
} // namespace GameCache