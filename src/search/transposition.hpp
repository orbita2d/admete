#include <map>


struct TransElement {
    TransElement() = default;
    TransElement(int eval) : _evaluation(eval) {}; 
    TransElement(int eval, int n) : _evaluation(eval), _nodes(n) {}; 
    TransElement(int eval, int n, int d) : _evaluation(eval), _nodes(n), _depth(d) {}; 
    int nodes() const{ return _nodes; }
    int evaluation() const{ return _evaluation; }
    int depth() const{ return _depth; }
private:
    int _evaluation = 0;
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
    void store(const long hash, const int eval);
    void store(const long hash, const int eval, const int nodes);
    void store(const long hash, const int eval, const int nodes, const int depth);
    void clear() {_data.clear(); }
private:
    tt_map _data;
    TransElement _last_hit;
};

static TranspositionTable transposition_table;