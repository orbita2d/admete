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

    static const Square N;
    static const Square E;
    static const Square S;
    static const Square W;
    static const Square NW;
    static const Square NE;
    static const Square SE;
    static const Square SW;
    static const Square NNW;
    static const Square NNE;
    static const Square ENE;
    static const Square ESE;
    static const Square SSE;
    static const Square SSW;
    static const Square WSW;
    static const Square WNW;
    
    static const Square Rank1;
    static const Square Rank2;
    static const Square Rank3;
    static const Square Rank4;
    static const Square Rank5;
    static const Square Rank6;
    static const Square Rank7;
    static const Square Rank8;

    static const Square FileA;
    static const Square FileB;
    static const Square FileC;
    static const Square FileD;
    static const Square FileE;
    static const Square FileF;
    static const Square FileG;
    static const Square FileH;

    bool operator==(const Square that) const { return value == that.value; }
    bool operator!=(const Square that) const { return value != that.value; }

    Square operator+(const Square that) const {return Square(value + that.value);};
    Square operator-(const Square that) const {return Square(value - that.value);};
    Square operator|(const Square that) const {return Square(value | that.value);};

    square_t get_value() const{
        return value;
    }

    square_t rank_index() const{
        return value / 8;
    }
    square_t file_index() const{
        return value % 8;
    }

    Square file() const{
        return value & 0x07;
    }

    Square rank() const{
        return value & 0x38;
    }

    square_t squares_to_north() const;
    square_t squares_to_south() const;
    square_t squares_to_east() const;
    square_t squares_to_west()const;

    static square_t to_index(const square_t rank, const square_t file){
        return 8 * rank + file;
    }

    operator square_t() const {
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

    Square origin;
    Square target;

    std::string pretty_print() const{
        if (capture){ return std::string(origin) + "x" + std::string(target); }
        else if(is_king_castle()) {return "O-O"; }
        else if(is_queen_castle()) {return "O-O-O"; }
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
    constexpr bool is_capture() const {
        return capture;
    }
    constexpr bool is_ep_capture() const {
        return ((promotion == false) & 
                (capture == true) & 
                (special1 == false) & 
                (special2 == true));
    }
    constexpr bool is_double_push() const {
        return ((promotion == false) & 
                (capture == false) & 
                (special1 == false) & 
                (special2 == true));
    }
    constexpr bool is_king_castle() const {
        return ((promotion == false) & 
                (capture == false) & 
                (special1 == true) & 
                (special2 == false));
    }
    constexpr bool is_queen_castle() const {
        return ((promotion == false) & 
                (capture == false) & 
                (special1 == true) & 
                (special2 == true));
    }
    Piece captured_peice;
    Square en_passent_target;
    uint halfmove_clock;
private:
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

    std::string print_move(const Move move, std::vector<Move> &legal_moves) const;
    bool is_free(const Square target) const;
    void get_sliding_moves(const Square origin, const Piece colour, const Square direction, const uint to_edge, std::vector<Move> &moves) const;
    void get_step_moves(const Square origin, const Square target, const Piece colour, std::vector<Move> &moves) const;
    void get_pawn_moves(const Square origin, const Piece colour, std::vector<Move> &moves) const;
    void get_bishop_moves(const Square origin, const Piece colour, std::vector<Move> &moves) const;
    void get_rook_moves(const Square origin, const Piece colour, std::vector<Move> &moves) const;
    void get_queen_moves(const Square origin, const Piece colour, std::vector<Move> &moves) const;
    void get_knight_moves(const Square origin, const Piece colour, std::vector<Move> &moves) const;
    void get_king_moves(const Square origin, const Piece colour, std::vector<Move> &moves) const;
    void get_castle_moves(const Piece colour, std::vector<Move> &moves) const;
    std::vector<Move> get_moves() const;

    void make_move(Move &move);
    void unmake_move(const Move move);
    void unmake_last() {unmake_move(last_move); }

    void try_move(const std::string move_sting);

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
    Move last_move;
};
