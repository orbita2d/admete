#include "board.hpp"

void print_tables();
score_t evaluate_material(Board &board);
score_t evaluate_lazy(Board &board, std::vector<Move> &legal_moves);
score_t piece_material(const PieceType);

typedef std::array<score_t, 64> position_board;
typedef std::array<position_board, 6> position_board_set;

class Score {
  public:
    constexpr Score(score_t o, score_t e) : opening_score(o), endgame_score(e) {}
    inline Score operator+(Score that) {
        return Score(opening_score + that.opening_score, endgame_score + that.endgame_score);
    }
    inline Score operator-(Score that) {
        return Score(opening_score - that.opening_score, endgame_score - that.endgame_score);
    }
    inline Score &operator+=(Score that) {
        opening_score += that.opening_score;
        endgame_score += that.endgame_score;
        return *this;
    }
    inline Score &operator-=(Score that) {
        opening_score -= that.opening_score;
        endgame_score -= that.endgame_score;
        return *this;
    }
    score_t opening_score;
    score_t endgame_score;
};

namespace Evaluation {
void init();
void load_tables(std::string filename);
score_t heuristic(Board &board);
score_t negamax_heuristic(Board &board);
score_t evaluate(Board &board);
score_t evaluate(Board &board, std::vector<Move> &legal_moves);
score_t piece_material(const PieceType p);
Score piece_value(const PieceType p);
score_t count_material(const Board &board);
score_t drawn_score(const Board &board);
constexpr score_t OPENING_MATERIAL = 6000;
constexpr score_t ENDGAME_MATERIAL = 2000;
constexpr score_t contempt = -15;
} // namespace Evaluation
