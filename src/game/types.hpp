#pragma once
#include <string>
#include <array>
#include <inttypes.h>


enum Direction : int {
    N = -8,
    E =  1,
    S = 8,
    W = -1,
    NW = -9,
    NE = -7,
    SE = 9,
    SW = 7,
    NNW = -17,
    NNE = -15,
    ENE = -6,
    ESE = 10,
    SSE = 17,
    SSW = 15,
    WSW = 6,
    WNW = -10
};


constexpr int N_SQUARE = 64;
class Square {
public:
    typedef signed int square_t;
    constexpr Square(const square_t val) : value(val) {};
    constexpr Square(const square_t rank, const square_t file) {
        value = to_index(rank, file);
    };
    Square(const std::string rf);
    Square() = default;

    bool operator==(const Square that) const { return value == that.value; }
    bool operator!=(const Square that) const { return value != that.value; }

    constexpr Square operator-(const Square that) const {return Square(value - that.value);};
    constexpr Square operator|(const Square that) const {return Square(value | that.value);};

    constexpr square_t get_value() const{
        return value;
    }

    constexpr square_t rank_index() const{
        return value / 8;
    }
    constexpr square_t file_index() const{
        return value % 8;
    }

    constexpr Square file() const{
        return value & 0x07;
    }

    constexpr Square rank() const{
        return value & 0x38;
    }

    constexpr square_t diagonal() const{
        return rank_index() + file_index();
    }
    constexpr square_t anti_diagonal() const{
        return rank_index() - file_index() + 7;
    }

    uint to_north() const { return rank_index(); };
    uint to_east() const { return 7-file_index(); };
    uint to_south() const { return 7-rank_index(); };
    uint to_west() const { { return file_index(); }};

    square_t to_northeast() const { return std::min(to_north(), to_east()); };
    square_t to_southeast() const { return std::min(to_south(), to_east()); };
    square_t to_southwest() const { return std::min(to_south(), to_west()); };
    square_t to_northwest() const { return std::min(to_north(), to_west()); };
    square_t to_dirx(const int dirx) const {
        switch (dirx)
        {
        case 0:
            return to_north();
        case 1:
            return to_east();
        case 2:
            return to_south();
        case 3:
            return to_west();
        case 4:
            return to_northeast();
        case 5:
            return to_southeast();
        case 6:
            return to_southwest();
        case 7:
            return to_northwest();
        default:
            return 0;
        }
    }

    static constexpr square_t to_index(const square_t rank, const square_t file){
        return 8 * rank + file;
    }

    constexpr operator square_t() const {
        return value;
    };
    
    inline Square &operator += (const Direction dir) {
        value += dir;
        return *this;
    }

    /*
    constexpr Square operator+(Direction v) {
        return Square(value + v);
    }
    */

    std::string pretty_print() const;
    operator std::string() const {
        return pretty_print();
    };
private:
    square_t value = 0;
};
std::ostream& operator<<(std::ostream& os, const Square square);

namespace Squares {    
    static constexpr Square Rank1 = 7 * 8;
    static constexpr Square Rank2 = 6 * 8;
    static constexpr Square Rank3 = 5 * 8;
    static constexpr Square Rank4 = 4 * 8;
    static constexpr Square Rank5 = 3 * 8;
    static constexpr Square Rank6 = 2 * 8;
    static constexpr Square Rank7 = 1 * 8;
    static constexpr Square Rank8 = 0 * 8;

    static constexpr Square FileA = 0;
    static constexpr Square FileB = 1;
    static constexpr Square FileC = 2;
    static constexpr Square FileD = 3;
    static constexpr Square FileE = 4;
    static constexpr Square FileF = 5;
    static constexpr Square FileG = 6;
    static constexpr Square FileH = 7;

    static constexpr std::array<Square, 8> by_dirx = {N, E, S, W, NE, SE, SW, NW};
}

constexpr int N_CASTLE = 2;
enum CastlingSide {
    KINGSIDE,
    QUEENSIDE
};
constexpr std::array<std::array<Square, 2>, 2> RookSquare = {{{Squares::FileH | Squares::Rank1, Squares::FileA | Squares::Rank1},{Squares::FileH | Squares::Rank8, Squares::FileA | Squares::Rank8}}};


enum Colour : int {
    WHITE,
    BLACK,
    N_COLOUR
};

enum PieceType {
    PAWN,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING,
    N_PIECE,
    NO_PIECE
};

typedef uint8_t depth_t;
typedef unsigned int ply_t;
constexpr ply_t MAX_PLY = 512;
typedef int16_t score_t;

constexpr score_t MIN_SCORE = -16382;
constexpr score_t MAX_SCORE = -MIN_SCORE;

constexpr score_t MATING_SCORE = 16381;

inline bool is_mating(score_t score) {
    return (score > (MATING_SCORE-500));
}

constexpr int NEG_INF = -1000000000;
constexpr int POS_INF = +1000000000;
