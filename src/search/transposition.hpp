#include <map>


struct TransElement {
    TransElement() = default;
    TransElement(int eval, int l, int u, int n, int d) : _exact(eval), _lower(l), _upper(u), _nodes(n), _depth(d) {}; 
    int nodes() const{ return _nodes; }
    int exact() const{ return _exact; }
    int upper() const{ return _upper; }
    int lower() const{ return _lower; }
    int depth() const{ return _depth; }
private:
    int _exact = 0;
    int _upper = 0;
    int _lower = 0;
    int _nodes = 0;
    int _depth = 0;
};


typedef std::pair<long, TransElement> tt_pair;
typedef std::map<long, TransElement> tt_map ;

class TranspositionTable {
public:
    TranspositionTable() = default;
    bool probe(const long);
    TransElement last_hit() const { return _last_hit; };
    void store(const long hash, const int eval, const int lower, const int upper, const int nodes, const int depth);
    void clear() {_data.clear(); }
private:
    tt_map _data;
    TransElement _last_hit;
};

static TranspositionTable transposition_table;