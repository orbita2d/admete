#include "cache.hpp"
#include <bit>

namespace GameCache {

void init() { pawn_cache = PawnCache(); }

PawnCache::PawnCache() {
    // Limit our index to a power of two.
    max_index = std::bit_floor(cache_max);
    bitmask = max_index - 1;
    _data.resize(max_index);
}

bool PawnCache::probe(const zobrist_t hash, PawnCacheElem &hit) {
    assert(_data.size() <= cache_max);
    if (is_enabled() == false) {
        return false;
    }
    const zobrist_t index = hash & bitmask;
    hit = _data.at(index);
    if (hit.hash() == hash) {
        return true;
    } else {
        return false;
    }
}

void PawnCache::store(const zobrist_t hash, const Score score) {
    assert(_data.size() <= cache_max);

    if (is_enabled() == false) {
        return;
    }
    const PawnCacheElem elem = PawnCacheElem(hash, score);
    const zobrist_t index = hash & bitmask;

    _data.at(index) = elem;
}

} // namespace GameCache