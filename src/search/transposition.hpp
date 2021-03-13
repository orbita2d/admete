#include <map>

struct TransElement {
    TransElement() = default;
    TransElement(int eval, int a, int b, int d) : _exact(eval), _lower(eval >= b), _upper(eval <= a), _depth(d) {}; 
    int exact() const{ return _exact; }
    bool lower() const{ return _lower; }
    bool upper() const{ return _upper; }
    int depth() const{ return _depth; }
private:
    int _exact = 0;
    int _depth = 0;
    bool _upper = 0;
    bool _lower = 0;
};


typedef std::pair<long, TransElement> tt_pair;
typedef std::map<long, TransElement> tt_map ;

class TranspositionTable {
public:
    TranspositionTable() = default;
    bool probe(const long);
    TransElement last_hit() const { return _last_hit; };
    void store(const long hash, const int eval, const int lower, const int upper, const int depth);
    void clear() {_data.clear(); }
private:
    tt_map _data;
    TransElement _last_hit;
};

static TranspositionTable transposition_table;