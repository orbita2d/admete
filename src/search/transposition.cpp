#include "transposition.hpp"
#include <array>
#include <algorithm>

std::array<long, Cache::tt_max> key_array;

bool Cache::TranspositionTable::probe(const long hash) {
    if (is_enabled() == false) {
        return false;
    }
    tt_map::iterator it = _data.find(hash);
    if (it == _data.end()) {
        return false;
    } else {
        _last_hit = it->second;
        return true;
    }
}

void Cache::TranspositionTable::store(const long hash, const int eval, const int lower, const int upper, const depth_t depth, const Move move) {
    if (is_enabled() == false) {
        return;
    }
    const TransElement elem = TransElement(eval, lower, upper, depth, move);
    if (index < tt_max) {
        // We are doing the first fill of the table;
        _data[hash] = elem;
        key_array[index] = hash;
    } else {
        // Replace oldest value
        const size_t i = index % tt_max;
        const long old_hash = key_array[i];
        key_array[i] = hash;
        _data.erase(old_hash);
        _data[hash] = elem;
    }
    index++;
}

DenseMove Cache::KillerTable::probe(const ply_t ply) {
    if (is_enabled() == false) {
        return NULL_DMOVE;
    }
    return _data[ply];
}


void Cache::KillerTable::store(const ply_t ply, const Move move) {
    if (is_enabled() == false) {
        return;
    }
    // Store the move, only if it's a quiet move.
    if (move.type != QUIETmv) { return; }
    // The move is quiet, store it in the table.
    _data[ply] = pack_move(move);
}


void Cache::init() {
    killer_table = KillerTable();
    transposition_table = TranspositionTable();
}