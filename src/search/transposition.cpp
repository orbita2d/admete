#include "transposition.hpp"

bool TranspositionTable::probe(const long hash) {
    tt_map::iterator it = _data.find(hash);
    if (it == _data.end()) {
        return false;
    } else {
        _last_hit = it->second;
        return true;
    }
}

void TranspositionTable::store(const long hash, const int eval, const int lower, const int upper, const unsigned int depth) {
    if (depth < min_depth()) {
        // Only store subtrees at least as deep as this minimum. 
        return;
    }
    const TransElement elem = TransElement(eval, lower, upper, depth);
    _data[hash] = elem;
}