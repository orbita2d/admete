#include "transposition.hpp"
#include <algorithm>
#include <array>
#include <assert.h>
#include <bit>
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
#include <xmmintrin.h>
#endif

Cache::TranspositionTable::TranspositionTable() {
    // Limit our index to a power of two.
    max_index = std::bit_floor(Cache::tt_max);
    bitmask = max_index - 1;
    _data.resize(max_index);
}

bool Cache::TranspositionTable::probe(const zobrist_t hash, TransElement &hit) {
    assert(_data.size() <= max_index);
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

void Cache::TranspositionTable::store(const zobrist_t hash, const score_t eval, const Bounds bound, const depth_t depth,
                                      const Move move, const ply_t ply) {
    assert(_data.size() <= max_index);

    if (is_enabled() == false) {
        return;
    }
    const TransElement elem = TransElement(hash, eval_to_tt(eval, ply), bound, depth, move);
    const zobrist_t index = hash & bitmask;
    const TransElement oldelem = _data.at(index);

    if (oldelem.is_delete()) {
        // Always replace if the old value is due to be replaced.
        _data.at(index) = elem;
    }
    if (elem.hash() == oldelem.hash()) {
        // If the entries refer to the same position, we want to only replace if the new entry is better, i.e. it's
        // exact and wasn't or it's a higher depth.
        if ((elem.exact() == oldelem.exact()) && (elem.depth() >= oldelem.depth())) {
            _data.at(index) = elem;
        } else if (elem.exact() && !oldelem.exact()) {
            _data.at(index) = elem;
        }
    } else {
        _data.at(index) = elem;
    }
}

void Cache::TranspositionTable::prefetch(const zobrist_t hash) {
#if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER)
    __builtin_prefetch(&_data[hash & bitmask], 0, 3);
#elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
    _mm_prefetch((const char*)&_data[hash & bitmask], _MM_HINT_T0);
#else
    (void)hash;
#endif
}

void Cache::TranspositionTable::set_delete() {
    for (TransElement t : _data) {
        // Iterates through the entire data structure
        t.set_delete();
    }
}

KillerTableRow Cache::KillerTable::probe(const ply_t ply) {
    assert(ply < MAX_PLY);
    if (is_enabled() == false) {
        return NULL_KROW;
    }
    return _data[ply];
}

void Cache::KillerTable::store(const ply_t ply, const Move move) {
    assert(ply < MAX_PLY);
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

uint Cache::HistoryTable::probe(const Move move) {
    assert(move != NULL_MOVE);
    assert(move.is_quiet());
    assert(move.target < 64);
    assert(move.moving_piece >= PAWN);
    assert(move.moving_piece < N_PIECE);
    return _data[move.moving_piece][move.target];
}
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
    assert(_data[pt][to_square] < 4290000000);
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
    if (prev_move == NULL_MOVE) {
        return NULL_DMOVE;
    }
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