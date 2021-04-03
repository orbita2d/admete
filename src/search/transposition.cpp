#include "transposition.hpp"
#include <array>
#include <algorithm>

std::array<long, tt_max> key_array;

bool TranspositionTable::probe(const long hash) {
    tt_map::iterator it = _data.find(hash);
    if (it == _data.end()) {
        return false;
    } else {
        _last_hit = it->second;
        return true;
    }
}

void TranspositionTable::store(const long hash, const int eval, const int lower, const int upper, const depth_t depth, const Move move) {
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

bool KillerTable::probe(const ply_t ply, MoveList &moves) {
    Move killer = _data[ply];
    auto it = std::find(moves.begin(), moves.end(), killer);
    if (it == moves.end()) {
        return false;
    } else {
        _last_hit = *it;
        return true;
    }
}


void KillerTable::store(const ply_t ply, const Move move) {
    // Store the move, only if it's a quiet move.
    if (move.type != QUIETmv) { return; }
    // The move is quiet, store it in the table.
    _data[ply] = move;
}