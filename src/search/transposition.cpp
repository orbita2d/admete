#include "transposition.hpp"
#include <algorithm>
#include <array>

Cache::TranspositionTable::TranspositionTable() {
    max_index = Cache::tt_max;
    index = 0;
    key_array.resize(max_index, 0);
}

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

score_t Cache::eval_to_tt(const score_t eval, const ply_t ply) {
    // We want to store a mating score in the TT as MATE - (ply to mate), but eval is MATE - (ply of checkmate)
    if (is_mating(eval)) {
        return eval + ply;
    } else if (is_mating(-eval)) {
        return eval - ply;
    } else {
        return eval;
    }
}

score_t Cache::eval_from_tt(const score_t eval, const ply_t ply) {
    // We want mating scoreeval is MATE - (ply of checkmate) but score in the TT as MATE - (ply to mate)
    if (is_mating(eval)) {
        return eval - ply;
    } else if (is_mating(-eval)) {
        return eval + ply;
    } else {
        return eval;
    }
}

void Cache::TranspositionTable::store(const long hash, const score_t eval, const score_t lower, const score_t upper,
                                      const depth_t depth, const Move move, const ply_t ply) {
    if (is_enabled() == false) {
        return;
    }
    const TransElement elem = TransElement(eval, lower, upper, depth, move, ply);
    if (index < max_index) {
        // We are doing the first fill of the table;
        _data[hash] = elem;
        key_array[index] = hash;
    } else {
        // Replace oldest value
        replace(index % max_index, hash, elem);
    }
    index++;
}

void Cache::TranspositionTable::set_delete() {
    for (tt_pair t : _data) {
        // Iterates through the entire data structure
        t.second.set_delete();
    }
}

KillerTableRow Cache::KillerTable::probe(const ply_t ply) {
    if (is_enabled() == false) {
        return NULL_KROW;
    }
    return _data[ply];
}

void Cache::KillerTable::store(const ply_t ply, const Move move) {
    if (is_enabled() == false) {
        return;
    }
    // Put this check here rather than in the search for simplicity
    if (move == NULL_MOVE) {
        return;
    }
    // Store the move, only if it's a quiet move.
    if (move.type != QUIETmv) {
        return;
    }
    // Don't keep duplicates
    if (move == _data[ply]) {
        return;
    }
    const DenseMove dmove = pack_move(move);
    int index = indicies[ply];
    _data[ply][index] = dmove;
    // Increment the counter so we use all the slots
    indicies[ply]++;
    indicies[ply] %= n_krow;
}

void Cache::init() {
    killer_table = KillerTable();
    transposition_table = TranspositionTable();
    history_table = HistoryTable();
    countermove_table = CountermoveTable();
}

void Cache::reinit() {
    // Create a new transposition table, of the new size.
    transposition_table = TranspositionTable();
}

uint Cache::HistoryTable::probe(const PieceType pt, const Square sq) { return _data[pt][sq]; }
void Cache::HistoryTable::store(const depth_t depth, const Move move) {

    if (is_enabled() == false) {
        return;
    }

    // Put this check here rather than in the search for simplicity
    if (move == NULL_MOVE) {
        return;
    }

    // Only record quiet moves.
    if (!move.is_quiet()) {
        return;
    }

    const PieceType pt = move.moving_piece;
    const Square to_square = move.target;
    // We don't check for overflow because it will simply never happen, UINT_MAX is 4294967295.
    _data[pt][to_square] += depth * depth;
}

void Cache::HistoryTable::clear() {
    for (int p = PAWN; p < N_PIECE; p++) {
        for (int sq = 0; sq < N_SQUARE; sq++) {
            _data[p][sq] = 0;
        }
    }
}

DenseMove Cache::CountermoveTable::probe(const Move prev_move) {
    const PieceType pt = prev_move.moving_piece;
    const Square to_square = prev_move.target;
    return _data[pt][to_square];
}

void Cache::CountermoveTable::store(const Move prev_move, const Move move) {

    if (is_enabled() == false) {
        return;
    }

    // Put this check here rather than in the search for simplicity
    if (move == NULL_MOVE) {
        return;
    }

    if (prev_move == NULL_MOVE) {
        return;
    }

    // Only record quiet moves.
    if (!move.is_quiet()) {
        return;
    }

    const PieceType pt = prev_move.moving_piece;
    const Square to_square = prev_move.target;

    _data[pt][to_square] = pack_move(move);
}

void Cache::CountermoveTable::clear() {
    for (int p = PAWN; p < N_PIECE; p++) {
        for (int sq = 0; sq < N_SQUARE; sq++) {
            _data[p][sq] = NULL_DMOVE;
        }
    }
}