#include "cache.hpp"

namespace GameCache {

void init() { pawn_cache = PawnCache(); }
bool PawnCache::probe(const zobrist_t hash, PawnCacheElem &hit) {
    assert(_data.size() <= cache_max);
    assert(key_array.size() == cache_max);
    if (is_enabled() == false) {
        return false;
    }
    tt_map::iterator it = _data.find(hash);
    if (it == _data.end()) {
        return false;
    } else {
        hit = it->second;
        return true;
    }
}

void PawnCache::replace(const size_t index, const zobrist_t new_hash, const PawnCacheElem elem) {
    assert(index < key_array.size());
    const zobrist_t old_hash = key_array[index];
    key_array[index] = new_hash;
    _data.erase(old_hash);
    _data[new_hash] = elem;
    assert(_data.size() <= cache_max);
}

void PawnCache::store(const zobrist_t hash, const Score score) {
    assert(_data.size() <= cache_max);
    assert(key_array.size() == cache_max);

    if (is_enabled() == false) {
        return;
    }
    const PawnCacheElem elem = PawnCacheElem(score);
    if (index < cache_max) {
        // We are doing the first fill of the table;
        _data[hash] = elem;
        key_array[index] = hash;
        index++;
    } else {
        replace(index % cache_max, hash, elem);
        index++;
    }
}

} // namespace GameCache