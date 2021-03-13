#include "transposition.hpp"

bool TranspositionTable::probe(const long hash) {
    //std::cerr << "probe( " <<hash << " )" << std::endl;
    tt_map::iterator it = _data.find(hash);
    if (it == _data.end()) {
        return false;
    } else {
        _last_hit = it->second;
        return true;
    }
}

void TranspositionTable::store(const long hash, const int eval, const int lower, const int upper, const int nodes, const int depth) {
    const TransElement elem = TransElement(eval, lower, upper, nodes, depth);
    _data[hash] = elem;
}