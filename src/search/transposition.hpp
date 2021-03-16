#include <map>

enum TransState {
    EXACT = 0,
    LOWER = 1,
    UPPER = 2
};

struct TransElement {
    TransElement() = default;
    TransElement(int eval, int a, int b, int d) : score(eval), _depth(d), info((eval <= a) ? UPPER : (eval >= b) ? LOWER : EXACT) {}; 
    int eval() const{ return score; }
    bool lower() const{ return info == LOWER; }
    bool upper() const{ return info == LOWER; }
    unsigned int depth() const{ return _depth; }
private:
    int score = 0;
    unsigned int _depth = 0;
    TransState info = EXACT;
};


typedef std::pair<long, TransElement> tt_pair;
typedef std::map<long, TransElement> tt_map ;

class TranspositionTable {
public:
    TranspositionTable() = default;
    bool probe(const long);
    TransElement last_hit() const { return _last_hit; };
    void store(const long hash, const int eval, const int lower, const int upper, const unsigned int depth);
    void clear() {_data.clear(); }
    unsigned int min_depth() {return _min_depth; }
    void min_depth(unsigned int d) { _min_depth = d; }
private:
    tt_map _data;
    TransElement _last_hit;
    unsigned int _min_depth = 0;
};

static TranspositionTable transposition_table;