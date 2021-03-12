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

void TranspositionTable::store(const long hash, const int eval) {
    TransElement elem = TransElement(eval);
    _data.insert(tt_pair(hash, eval));
}


void TranspositionTable::store(const long hash, const int eval, const int nodes) {
    TransElement elem = TransElement(eval, nodes);
    _data.insert(tt_pair(hash, elem));
}

void TranspositionTable::store(const long hash, const int eval, const int nodes, const int depth) {
    TransElement elem = TransElement(eval, nodes, depth);
    _data.insert(tt_pair(hash, elem));
}