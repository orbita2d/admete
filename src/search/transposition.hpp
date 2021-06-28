#include "../game/piece.hpp"
#include "../game/types.hpp"
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
    TransElement(score_t eval, score_t a, score_t b, depth_t d, Move m, ply_t ply)
        : score(eval_to_tt(eval, ply)), _depth(d), info((eval <= a)   ? UPPER
                                                        : (eval >= b) ? LOWER
                                                                      : EXACT),
          hash_move(pack_move(m)){};
    score_t eval(ply_t ply) const { return eval_from_tt(score, ply); }
    bool lower() const { return (info & bound_mask) == LOWER; }
    bool upper() const { return (info & bound_mask) == UPPER; }
    bool exact() const { return (info & bound_mask) == EXACT; }
    bool is_delete() const { return (info & DELETE) == DELETE; }
    bool is_cache_hit() const { return (info & CACHEHIT) == CACHEHIT; }
    void set_delete() { info |= TransState::DELETE; }
    void set_cache_hit() { info |= TransState::CACHEHIT; }
    tt_flags_t flags() { return info; }
    depth_t depth() const { return _depth; }
    DenseMove move() const { return hash_move; }

  private:
    score_t score = 0;
    depth_t _depth = 0;
    tt_flags_t info = EXACT;
    DenseMove hash_move = NULL_DMOVE;
};

typedef std::pair<long, TransElement> tt_pair;
typedef std::unordered_map<long, TransElement> tt_map;
// Limit transposition table to 64MB
constexpr size_t tt_max = (1 << 26) / sizeof(tt_pair);

class TranspositionTable {
  public:
    TranspositionTable() = default;
    bool probe(const long);
    TransElement last_hit() const { return _last_hit; };
    void store(const long hash, const score_t eval, const score_t lower, const score_t upper, const depth_t depth,
               const Move move, const ply_t ply);
    void replace(const size_t index, const long new_hash, const TransElement elem);
    void clear() {
        _data.clear();
        index = 0ul;
    }
    depth_t min_depth() { return _min_depth; }
    void min_depth(depth_t d) { _min_depth = d; }
    bool is_enabled() { return enabled; }
    void enable() { enabled = true; }
    void disable() { enabled = false; }
    void set_delete();

  private:
    tt_map _data;
    TransElement _last_hit;
    depth_t _min_depth = 0;
    size_t index;
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

void init();
} // namespace Cache