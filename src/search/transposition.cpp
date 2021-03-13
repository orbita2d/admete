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

void TranspositionTable::store(const long hash, const int eval, const int lower, const int upper, const int depth) {
    const TransElement elem = TransElement(eval, lower, upper, depth);
    _data[hash] = elem;
}