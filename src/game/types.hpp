#pragma once
#include <array>
#include <inttypes.h>
#include <string>
#include <vector>
#include <assert.h>

typedef unsigned int uint;
typedef uint64_t zobrist_t;

enum Direction : int {
    N = 8,
    E = 1,
    S = -8,
    W = -1,
    NW = 7,
    NE = 9,
    SE = -7,
    SW = -9,
    NNW = 15,
    NNE = 17,
    ENE = 10,
    ESE = -6,
    SSE = -15,
    SSW = -17,
    WSW = -10,
    WNW = 6
};

constexpr int N_SQUARE = 64;
class Square {
  public:
    typedef unsigned int square_t;
    constexpr Square(const square_t val) : value(val){};
    constexpr Square(const square_t rank, const square_t file) { value = 8 * rank + file; };
    Square(const std::string rf);
    Square() = default;

    bool operator==(const Square that) const { return value == that.value; }
    bool operator!=(const Square that) const { return value != that.value; }

    constexpr Square operator-(const Square that) const { return Square(value - that.value); };
    constexpr Square operator|(const Square that) const { return Square(value | that.value); };

    constexpr square_t get_value() const { return value; }

    constexpr square_t rank_index() const { return value / 8; }
    constexpr square_t file_index() const { return value % 8; }

    constexpr Square file() const { return value & 0x07; }

    constexpr Square rank() const { return value & 0x38; }

    constexpr square_t diagonal() const { return rank_index() + file_index(); }
    constexpr square_t anti_diagonal() const { return rank_index() - file_index() + 7; }

    square_t to_north() const { return 7 - rank_index(); };
    square_t to_east() const { return 7 - file_index(); };
    square_t to_south() const { return rank_index(); };
    square_t to_west() const { return file_index(); };

    square_t to_northeast() const { return std::min(to_north(), to_east()); };
    square_t to_southeast() const { return std::min(to_south(), to_east()); };
    square_t to_southwest() const { return std::min(to_south(), to_west()); };
    square_t to_northwest() const { return std::min(to_north(), to_west()); };

    constexpr operator square_t() const { return value; };

    inline Square &operator+=(const Direction dir) {
        value += dir;
        return *this;
    }

    std::string pretty() const;

  private:
    square_t value = 0;
};
std::ostream &operator<<(std::ostream &os, const Square square);

namespace Squares {
static constexpr Square Rank1 = 0 * 8;
static constexpr Square Rank2 = 1 * 8;
static constexpr Square Rank3 = 2 * 8;
static constexpr Square Rank4 = 3 * 8;
static constexpr Square Rank5 = 4 * 8;
static constexpr Square Rank6 = 5 * 8;
static constexpr Square Rank7 = 6 * 8;
static constexpr Square Rank8 = 7 * 8;

static constexpr Square FileA = 0;
static constexpr Square FileB = 1;
static constexpr Square FileC = 2;
static constexpr Square FileD = 3;
static constexpr Square FileE = 4;
static constexpr Square FileF = 5;
static constexpr Square FileG = 6;
static constexpr Square FileH = 7;

} // namespace Squares

constexpr int N_CASTLE = 2;
enum CastlingSide { KINGSIDE, QUEENSIDE };

// Squares the rooks sit on (for castling).
constexpr std::array<std::array<Square, 2>, 2> RookSquares = {
    {{Squares::FileH | Squares::Rank1, Squares::FileA | Squares::Rank1},
     {Squares::FileH | Squares::Rank8, Squares::FileA | Squares::Rank8}}};

// Squares the rooks move to when castling.
constexpr std::array<std::array<Square, 2>, 2> RookCastleSquares = {
    {{Squares::FileF | Squares::Rank1, Squares::FileD | Squares::Rank1},
     {Squares::FileF | Squares::Rank8, Squares::FileD | Squares::Rank8}}};

// Colours

enum Colour : int { WHITE, BLACK, N_COLOUR };

constexpr Colour operator~(Colour c) {
    return Colour(c ^ Colour::BLACK); // Toggle color
}
// Pieces

enum PieceType { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, N_PIECE, NO_PIECE };
inline PieceType operator++(PieceType &p, int) { return p = PieceType(int(p) + 1); };
inline PieceType operator--(PieceType &p, int) { return p = PieceType(int(p) - 1); };

class Piece {
  public:
    Piece() = default;
    constexpr Piece(const Colour c, const PieceType p) : colour(c), piece(p){};
    Colour get_colour() const { return colour; };
    PieceType get_piece() const { return piece; };
    // Returns true if the piece represents a blank square
    bool is_blank() const { return piece == NO_PIECE; };
    std::string pretty() const;

  private:
    Colour colour;
    PieceType piece;
};

constexpr Piece BLANK_PIECE = Piece(WHITE, NO_PIECE);

std::ostream &operator<<(std::ostream &os, const Piece piece);

typedef uint8_t depth_t;
constexpr depth_t MAX_DEPTH = 127;

typedef unsigned int ply_t;
constexpr ply_t MAX_PLY = 512;
typedef int16_t score_t;

constexpr score_t MAX_SCORE = 32767;
constexpr score_t MIN_SCORE = -MAX_SCORE;

// Space for mating scores [MIN_MATE_SCORE, MATING_SCORE]
constexpr score_t MATING_SCORE = 32766;
constexpr score_t MIN_MATE_SCORE = MATING_SCORE - MAX_PLY;

// Space for Tablebase wins [TBWIN_MIN, TBWIN]
constexpr score_t TBWIN = MIN_MATE_SCORE - 1;
constexpr score_t TBWIN_MIN = TBWIN - MAX_PLY;

enum Bounds { LOWER, UPPER, EXACT };

inline bool is_mating(const score_t score) { return (score >= MIN_MATE_SCORE) && (score <= MATING_SCORE); }

inline ply_t mate_score_to_ply(const score_t score) { assert(score < MATING_SCORE); return MATING_SCORE - score; }

inline score_t ply_to_mate_score(const ply_t ply) { assert(ply < MAX_PLY); return MATING_SCORE - ply; }

constexpr int NEG_INF = -1000000000;
constexpr int POS_INF = +1000000000;

enum BishopTypes { LIGHTSQUARE, DARKSQUARE, N_BISHOPTYPES };

// Moves
// Uses four bits to store additional info about moves
enum MoveType {
    QUIETmv = 0,
    CAPTURE = 1,
    PROMOTION = 2,
    SPECIAL1 = 4,
    SPECIAL2 = 8,
    EN_PASSENT = 9,
    DOUBLE_PUSH = 8,
    KING_CASTLE = 4,
    QUEEN_CASTLE = 12
};

struct DenseMove {
    constexpr DenseMove() = default;
    constexpr DenseMove(const Square o, const Square t) : v((o.get_value() << 6) | t.get_value()){};
    constexpr DenseMove(const Square o, const Square t, const MoveType m)
        : v((m << 12) | (o.get_value() << 6) | t.get_value()){};
    Square target() const { return v & 0x003f; }
    Square origin() const { return (v >> 6) & 0x003f; }
    MoveType type() const { return (MoveType)((v >> 12) & 0x000f); }
    int16_t v = 0;
};

inline bool operator==(const DenseMove m1, const DenseMove m2) { return (m1.v == m2.v); }
constexpr DenseMove NULL_DMOVE = DenseMove();

struct Move {
    Move(const PieceType p, const Square o, const Square t) : origin(o), target(t), moving_piece(p){};
    Move(const PieceType p, const Square o, const Square t, const MoveType m)
        : origin(o), target(t), moving_piece(p), type(m){};
    constexpr Move(){};

    Square origin;
    Square target;
    PieceType moving_piece = NO_PIECE;
    PieceType captured_piece = NO_PIECE;
    MoveType type = QUIETmv;
    int score = 0;

    std::string pretty() const {
        std::string move_string;
        move_string = origin.pretty() + target.pretty();
        if (is_knight_promotion()) {
            move_string = move_string + "n";
        } else if (is_bishop_promotion()) {
            move_string = move_string + "b";
        } else if (is_rook_promotion()) {
            move_string = move_string + "r";
        } else if (is_queen_promotion()) {
            move_string = move_string + "q";
        }
        return move_string;
    }

    bool operator==(const Move that) const {
        return origin == that.origin && target == that.target && type == that.type;
    }
    bool operator!=(const Move that) const { return !operator==(that); }

    void make_quiet() { type = QUIETmv; }
    void make_double_push() { type = DOUBLE_PUSH; }
    void make_king_castle() { type = KING_CASTLE; }
    void make_queen_castle() { type = QUEEN_CASTLE; }
    void make_capture() { type = CAPTURE; }
    void make_en_passent() { type = EN_PASSENT; }

    void make_bishop_promotion() { type = (MoveType)((type & CAPTURE) | (PROMOTION)); }
    void make_knight_promotion() { type = (MoveType)((type & CAPTURE) | (PROMOTION | SPECIAL2)); }
    void make_rook_promotion() { type = (MoveType)((type & CAPTURE) | (PROMOTION | SPECIAL1)); }
    void make_queen_promotion() { type = (MoveType)((type & CAPTURE) | (PROMOTION | SPECIAL1 | SPECIAL2)); }
    constexpr bool is_quiet() const { return !is_capture(); }
    constexpr bool is_capture() const { return type & CAPTURE; }
    constexpr bool is_ep_capture() const { return type == EN_PASSENT; }
    constexpr bool is_double_push() const { return type == DOUBLE_PUSH; }
    constexpr bool is_king_castle() const { return type == KING_CASTLE; }
    constexpr bool is_queen_castle() const { return type == QUEEN_CASTLE; }
    constexpr bool is_knight_promotion() const { return (type & ~CAPTURE) == (PROMOTION); }
    constexpr bool is_bishop_promotion() const { return (type & ~CAPTURE) == (PROMOTION | SPECIAL2); }
    constexpr bool is_rook_promotion() const { return (type & ~CAPTURE) == (PROMOTION | SPECIAL1); }
    constexpr bool is_queen_promotion() const { return (type & ~CAPTURE) == (PROMOTION | SPECIAL1 | SPECIAL2); }
    constexpr bool is_promotion() const { return (type & PROMOTION); }
};

constexpr Move NULL_MOVE = Move();

constexpr PieceType get_promoted(const Move m) {
    if (m.is_knight_promotion()) {
        return KNIGHT;
    } else if (m.is_bishop_promotion()) {
        return BISHOP;
    } else if (m.is_rook_promotion()) {
        return ROOK;
    } else if (m.is_queen_promotion()) {
        return QUEEN;
    }
    return NO_PIECE;
}
std::ostream &operator<<(std::ostream &os, const Move move);

inline bool operator==(const Move m, const DenseMove dm) {
    return (dm.origin() == m.origin && dm.target() == m.target && dm.type() == m.type);
}
inline bool operator==(const DenseMove dm, const Move m) { return m == dm; }

// Length of a row in the Killer Table
constexpr size_t n_krow = 3;
typedef std::array<DenseMove, n_krow> KillerTableRow;
constexpr KillerTableRow NULL_KROW = KillerTableRow();

inline bool operator==(const DenseMove m, const KillerTableRow row) {
    for (DenseMove dm : row) {
        if (m == dm) {
            return true;
        }
    }
    return false;
}
inline bool operator==(const Move m, const KillerTableRow row) {
    for (DenseMove dm : row) {
        if (m == dm) {
            return true;
        }
    }
    return false;
}

typedef std::vector<Move> MoveList;

inline bool operator==(const DenseMove dm, const MoveList moves) {
    for (Move m : moves) {
        if (m == dm) {
            return true;
        }
    }
    return false;
}

inline DenseMove pack_move(const Move m) { return DenseMove(m.origin, m.target, m.type); }

inline Move unpack_move(const DenseMove dm, const MoveList &moves) {
    for (auto move : moves) {
        if (move == dm) {
            return move;
        }
    }
    return NULL_MOVE;
}

inline bool is_legal(const Move move, const MoveList &legal_moves) {
    for (const Move lm : legal_moves) {
        if (move == lm) {
            return true;
        }
    }
    return false;
}

enum NodeType { PVNODE, ALLNODE, CUTNODE };
