#include "types.hpp"
#include <iostream>
#include <unordered_map>

namespace Cache {
enum TransState {
    EXACT = 0,   // Stored score is exact for the node
    LOWER = 1,   // Stored score is a lower bound
    UPPER = 2,   // Stored score is an upper bound
    DELETE = 4,  // Entry has been marked for deletion
    CACHEHIT = 8 // Entry has had cache hit before
};
typedef int8_t tt_flags_t;
constexpr tt_flags_t bound_mask = 0x03;

score_t eval_to_tt(const score_t eval, const ply_t ply);
score_t eval_from_tt(const score_t eval, const ply_t ply);
// 6 bytes
struct TransElement {
    TransElement() = default;
    TransElement(score_t eval, Bounds bound, depth_t d, Move m)
        : score(eval), _depth(d),
          info((bound == Bounds::UPPER) ? TransState::UPPER
                                        : (bound == Bounds::LOWER) ? TransState::LOWER : TransState::EXACT),
          hash_move(pack_move(m)){};
    score_t eval(ply_t ply) const { return eval_from_tt(score, ply); }
    bool lower() const { return (info & bound_mask) == TransState::LOWER; }
    bool upper() const { return (info & bound_mask) == TransState::UPPER; }
    bool exact() const { return (info & bound_mask) == TransState::EXACT; }
    bool is_delete() const { return (info & TransState::DELETE) == TransState::DELETE; }
    bool is_cache_hit() const { return (info & TransState::CACHEHIT) == TransState::CACHEHIT; }
    void set_delete() { info |= TransState::DELETE; }
    void set_cache_hit() { info |= TransState::CACHEHIT; }
    tt_flags_t flags() { return info; }
    depth_t depth() const { return _depth; }
    DenseMove move() const { return hash_move; }

  private:
    int16_t score = 0;
    uint8_t _depth = 0;
    tt_flags_t info = EXACT;
    DenseMove hash_move = NULL_DMOVE;
};

typedef std::pair<zobrist_t, TransElement> tt_pair;
typedef std::unordered_map<zobrist_t, TransElement> tt_map;

constexpr unsigned hash_default = 64u;
constexpr unsigned hash_min = 1u;
constexpr unsigned hash_max = 512u;

inline size_t tt_max = (hash_default * (1 << 20)) / sizeof(tt_pair);

class TranspositionTable {
  public:
    TranspositionTable();
    bool probe(const zobrist_t, TransElement &hit);
    void store(const zobrist_t hash, const score_t eval, const Bounds bound, const depth_t depth, const Move move,
               const ply_t ply);
    void clear() { _data.clear(); }
    bool is_enabled() { return enabled; }
    void enable() { enabled = true; }
    void disable() { enabled = false; }
    void set_delete();

  private:
    std::vector<tt_pair> _data;
    size_t max_index;
    bool enabled = true;
};

inline TranspositionTable transposition_table;

class KillerTable {
    // Table for the killer heuristic;
  public:
    KillerTable() = default;
    KillerTableRow probe(const ply_t ply);
    void store(const ply_t ply, const Move move);
    bool is_enabled() { return enabled; }
    void enable() { enabled = true; }
    void disable() { enabled = false; }

  private:
    std::array<KillerTableRow, MAX_PLY> _data;
    std::array<int, MAX_PLY> indicies;
    bool enabled = true;
};

inline KillerTable killer_table;

class HistoryTable {
    // Table for the history heuristic;
  public:
    HistoryTable() = default;
    uint probe(const Move);
    void store(const depth_t depth, const Move move);
    bool is_enabled() { return enabled; }
    void enable() { enabled = true; }
    void disable() { enabled = false; }

    void clear();

  private:
    uint _data[N_PIECE][N_SQUARE];
    bool enabled = true;
};
inline HistoryTable history_table;

class CountermoveTable {
    // Table for the history heuristic;
  public:
    CountermoveTable() = default;
    DenseMove probe(const Move prev_move);
    void store(const Move prev_move, const Move move);
    bool is_enabled() { return enabled; }
    void enable() { enabled = true; }
    void disable() { enabled = false; }

    void clear();

  private:
    DenseMove _data[N_PIECE][N_SQUARE];
    bool enabled = true;
};
inline CountermoveTable countermove_table;
void init();
void reinit();
} // namespace Cache