#pragma once
#include <array>
#include <assert.h>
#include <inttypes.h>
#include <string>
#include <vector>

typedef unsigned int uint;
typedef uint64_t zobrist_t;

// Colours

enum Colour : int { WHITE, BLACK, N_COLOUR };

constexpr Colour operator~(Colour c) {
    return Colour(c ^ Colour::BLACK); // Toggle color
}

enum Rank { RANK1, RANK2, RANK3, RANK4, RANK5, RANK6, RANK7, RANK8 };
enum File { FILEA, FILEB, FILEC, FILED, FILEE, FILEF, FILEG, FILEH, N_FILES, NO_FILE };

constexpr Rank relative_rank(const Colour c, const Rank r) {
    if (c == WHITE) {
        return r;
    } else {
        return (Rank)(7 - r);
    }
}

constexpr Rank back_rank(const Colour us) {
    if (us == WHITE) {
        return RANK1;
    } else {
        return RANK8;
    }
}

enum Direction {
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
    WNW = 6,
    SS = -16,
    EE = 2,
    NN = 16,
    WW = -2,
};

constexpr Direction forwards(const Colour us) {
    if (us == WHITE) {
        return Direction::N;
    } else {
        return Direction::S;
    }
}

constexpr Direction backwards(const Colour us) {
    if (us == WHITE) {
        return Direction::S;
    } else {
        return Direction::N;
    }
}

constexpr int N_SQUARE = 64;
class Square {
  public:
    typedef unsigned int square_t;
    constexpr Square(const square_t val) : value(val){};
    constexpr Square(const Rank rank, const File file) { value = 8 * rank + file; };
    Square(const std::string rf);
    Square() = default;

    bool operator==(const Square that) const { return value == that.value; }
    bool operator!=(const Square that) const { return value != that.value; }

    constexpr square_t get_value() const { return value; }

    constexpr square_t rank_index() const { return value / 8; }
    constexpr square_t file_index() const { return value % 8; }

    constexpr File file() const { return File(value % 8); }
    constexpr Rank rank() const { return Rank(value / 8); }
    // constexpr File file() const { return File(value & 0x07); }
    // constexpr Rank rank() const { return Rank((value & 0x38) >> 3); }

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

enum CastlingSide { KINGSIDE, QUEENSIDE, N_CASTLE };
enum CastlingRights { NO_RIGHTS = 0, WHITE_KINGSIDE = 1, WHITE_QUEENSIDE = 2, BLACK_KINGSIDE = 4, BLACK_QUEENSIDE = 8 };

constexpr CastlingRights get_rights(const Colour c, const CastlingSide side) {
    if (c == WHITE) {
        if (side == KINGSIDE) {
            return WHITE_KINGSIDE;
        } else {
            return WHITE_QUEENSIDE;
        }
    } else {
        if (side == KINGSIDE) {
            return BLACK_KINGSIDE;
        } else {
            return BLACK_QUEENSIDE;
        }
    }
}

constexpr unsigned get_rights(const Colour c) {
    if (c == WHITE) {
        return WHITE_KINGSIDE | WHITE_QUEENSIDE;
    } else {
        return BLACK_KINGSIDE | BLACK_QUEENSIDE;
    }
}

// Squares the rooks sit on (for castling).
constexpr std::array<std::array<Square, 2>, 2> RookSquares = {
    {{Square(RANK1, FILEH), Square(RANK1, FILEA)}, {Square(RANK8, FILEH), Square(RANK8, FILEA)}}};

// Squares the rooks move to when castling.
constexpr std::array<std::array<Square, 2>, 2> RookCastleSquares = {
    {{Square(RANK1, FILEF), Square(RANK1, FILED)}, {Square(RANK8, FILEF), Square(RANK8, FILED)}}};

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

typedef uint_fast8_t depth_t;
constexpr depth_t MAX_DEPTH = 127;

typedef unsigned int ply_t;
constexpr ply_t MAX_PLY = 512;
typedef int_fast16_t score_t;

constexpr score_t MAX_SCORE = 32767;
constexpr score_t MIN_SCORE = -MAX_SCORE;

// Space for mating scores [MIN_MATE_SCORE, MATING_SCORE]
constexpr score_t MATING_SCORE = MAX_SCORE - 1;
constexpr score_t MIN_MATE_SCORE = MATING_SCORE - MAX_PLY;

// Space for Tablebase wins [TBWIN_MIN, TBWIN]
constexpr score_t TBWIN = MIN_MATE_SCORE - 1;
constexpr score_t TBWIN_MIN = TBWIN - MAX_PLY;

enum Bounds { LOWER, UPPER, EXACT };

inline bool is_mating(const score_t score) { return (score >= MIN_MATE_SCORE) && (score <= MATING_SCORE); }

inline ply_t mate_score_to_ply(const score_t score) {
    assert(score < MATING_SCORE);
    return MATING_SCORE - score;
}

inline score_t ply_to_mate_score(const ply_t ply) {
    assert(ply < MAX_PLY);
    return MATING_SCORE - ply;
}

constexpr int NEG_INF = -1000000000;
constexpr int POS_INF = +1000000000;

// Class for a score object with opening and endgame scores.
class Score {
  public:
    Score() = default;
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
    score_t opening_score = 0;
    score_t endgame_score = 0;
};
inline bool operator==(const Score s1, const Score s2) {
    return s1.opening_score == s2.opening_score && s1.endgame_score == s2.endgame_score;
}

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
    constexpr bool is_castle() const { return is_king_castle() || is_queen_castle(); };
    constexpr CastlingSide get_castleside() const {
        assert(is_castle());
        return is_king_castle() ? KINGSIDE : QUEENSIDE;
    }
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
