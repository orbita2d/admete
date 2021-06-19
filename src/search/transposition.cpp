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
        it->second.set_cache_hit();
        _last_hit = it->second;
        return true;
    }
}


void Cache::TranspositionTable::replace(const size_t index, const long new_hash, const TransElement elem) {
    const long old_hash = key_array[index];
    key_array[index] = new_hash;
    _data.erase(old_hash);
    _data[new_hash] = elem;
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
        replace(index % tt_max, hash, elem);
    }
    index++;
}


void Cache::TranspositionTable::set_delete() {
    for (tt_pair t : _data) {
        // Iterates through the entire data structure
        t.second.set_delete();
    }
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