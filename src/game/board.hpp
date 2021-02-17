#include <map>
#include <array>
#include <vector>
#include <string>
#include "piece.hpp"


class Square {
public:
    typedef signed int square_t;
    Square(const square_t val) : value(val) {};
    Square(const square_t rank, const square_t file) {
        value = to_index(rank, file);
    };
    Square(const std::string rf);
    Square() = default;

    static const square_t N = -8;
    static const square_t E =  1;
    static const square_t S =  8;
    static const square_t W = -1;
    static const square_t NW = N + W;
    static const square_t NE = N + E;
    static const square_t SE = S + E;
    static const square_t SW = S + W;
    static const square_t NNW = N + N + W;
    static const square_t NNE = N + N + E;
    static const square_t ENE = E + N + E;
    static const square_t ESE = E + S + E;
    static const square_t SSE = S + S + E;
    static const square_t SSW = S + S + W;
    static const square_t WSW = W + S + W;
    static const square_t WNW = W + N + W;

    static const square_t Rank1 = 7 * 8;
    static const square_t Rank2 = 6 * 8;
    static const square_t Rank3 = 5 * 8;
    static const square_t Rank4 = 4 * 8;
    static const square_t Rank5 = 3 * 8;
    static const square_t Rank6 = 2 * 8;
    static const square_t Rank7 = 1 * 8;
    static const square_t Rank8 = 0 * 8;

    static const square_t FileA = 0 ;
    static const square_t FileB = 1;
    static const square_t FileC = 2;
    static const square_t FileD = 3;
    static const square_t FileE = 4;
    static const square_t FileF = 5;
    static const square_t FileG = 6;
    static const square_t FileH = 7;

    bool operator==(const Square that) const { return value == that.value; }
    bool operator!=(const Square that) const { return value != that.value; }

    Square operator+(const Square that) const {return Square(value + that.value);};
    Square operator-(const Square that) const {return Square(value - that.value);};
    Square operator|(const Square that) const {return Square(value | that.value);};

    square_t get_value() const{
        return value;
    }

    square_t rank() const{
        return value / 8;
    }
    square_t file() const{
        return value % 8;
    }

    square_t squares_to_north() const;
    square_t squares_to_south() const;
    square_t squares_to_east() const;
    square_t squares_to_west()const;

    static square_t to_index(const square_t rank, const square_t file){
        return 8 * rank + file;
    }

    operator square_t() {
        return value;
    };

    std::string pretty_print() const;
    operator std::string() const {
        return pretty_print();
    };
    std::vector<Square> knight_moves() const;
private:
    square_t value = 0;
};
std::ostream& operator<<(std::ostream& os, const Square square);


class Move {
public:
    Move(const Square o, const Square t) : origin(o), target(t) {};
    Move() = default;
    std::string pretty_print() const{
        if (capture){ return std::string(origin) + "x" + std::string(target); }
        else {return std::string(origin) + std::string(target);}
    }
    void make_quiet() {
        promotion = false;
        capture = false;
        special1 = false;
        special2 = false;
    }
    void make_double_push() {
        promotion = false;
        capture = false;
        special1 = false;
        special2 = true;
    }
    void make_king_castle() {
        promotion = false;
        capture = false;
        special1 = true;
        special2 = false;
    }
    void make_queen_castle() {
        promotion = false;
        capture = false;
        special1 = true;
        special2 = true;
    }
    void make_capture() {
        promotion = false;
        capture = true;
        special1 = false;
        special2 = false;
    }
    void make_en_passent() {
        promotion = false;
        capture = true;
        special1 = false;
        special2 = true;
    }
private:
    Square origin;
    Square target;
    bool promotion = 0;
    bool capture = 0;
    bool special1 = 0;
    bool special2 = 0;
};
std::ostream& operator<<(std::ostream& os, const Move move);

constexpr bool white_move = false;
constexpr bool black_move = true;

class Board {
public:
    Board() {
        pieces.fill(Piece::Blank);
    };

    void fen_decode(const std::string& fen);

    void print_board_idx();

    void print_board();

    void print_board_extra();

    std::vector<Move> get_pawn_moves(const Square origin, const Piece colour) const;
    void get_moves() const;
private:
    std::array<Piece, 64> pieces;
    bool whos_move = white_move;
    bool castle_white_kingside = true;
    bool castle_white_queenside = true;
    bool castle_black_kingside = true;
    bool castle_black_queenside = true;
    Square en_passent_target = 0;
    uint halfmove_clock = 0;
    uint fullmove_counter = 1;
};
