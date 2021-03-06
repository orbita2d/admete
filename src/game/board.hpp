#pragma once
#include <map>
#include <array>
#include <vector>
#include <string>
#include "piece.hpp"

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

class KnightMoveArray {
    // Class just to hold the iterator for a knight move so it doesn't have to be a vector.
public:
    KnightMoveArray() = default;
    Square operator[](const unsigned int i) { return move_array[i];};
    Square operator[](const unsigned int i) const { return move_array[i];};
    unsigned int len = 0;
    typedef Square * iterator;
    typedef const Square * const_iterator;
    iterator begin() { return &move_array[0]; }
    iterator end() { return &move_array[len]; }
    void push_back(const Square in) {
        move_array[len] = in;
        len++;
    }
private:
    std::array<Square, 8> move_array; 
};


KnightMoveArray knight_moves(const Square origin);

class Move {
public:
    Move(const Square o, const Square t) : origin(o), target(t) {};
    Move() = default;

    Square origin;
    Square target;

    std::string pretty_print() const{
        std::string move_string;
        move_string = std::string(origin) + std::string(target);
        if (is_knight_promotion()) {move_string= move_string + "n";}
        else if (is_bishop_promotion()) {move_string= move_string + "b";}
        else if (is_rook_promotion()) {move_string= move_string + "r";}
        else if (is_queen_promotion()) {move_string= move_string + "q";}
        return move_string;
    }

    bool operator==(const Move that) {
        return  origin == that.origin &&
                target == that.target &&
                special1 == that.special1 && 
                special2 == that.special2;
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

    void make_bishop_promotion() {
        promotion = true;
        special1 = false;
        special2 = false;
    }
    void make_knight_promotion() {
        promotion = true;
        special1 = false;
        special2 = true;
    }
    void make_rook_promotion() {
        promotion = true;
        special1 = true;
        special2 = false;
    }
    void make_queen_promotion() {
        promotion = true;
        special1 = true;
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
    constexpr bool is_knight_promotion() const {
        return ((promotion == true) & 
                (special1 == false) & 
                (special2 == false));
    }
    constexpr bool is_bishop_promotion() const {
        return ((promotion == true) & 
                (special1 == false) & 
                (special2 == true));
    }
    constexpr bool is_rook_promotion() const {
        return ((promotion == true) & 
                (special1 == true) & 
                (special2 == false));
    }
    constexpr bool is_queen_promotion() const {
        return ((promotion == true) & 
                (special1 == true) & 
                (special2 == true));
    }
    constexpr bool is_promotion() const {
        return promotion;
    }
    Piece captured_peice;
private:
    bool promotion = 0;
    bool capture = 0;
    bool special1 = 0;
    bool special2 = 0;
};
std::ostream& operator<<(std::ostream& os, const Move move);

constexpr bool white_move = false;
constexpr bool black_move = true;

typedef uint64_t bitboard;

struct AuxilliaryInfo {
    // Information that is game history dependent, that would otherwise need to be encoded in a move.
    bool castle_white_kingside = true;
    bool castle_white_queenside = true;
    bool castle_black_kingside = true;
    bool castle_black_queenside = true;
    uint halfmove_clock = 0;
    Square en_passent_target;
    std::array<Square, 16> pinned_pieces;
    uint number_checkers;
    std::array<Square, 2> checkers;
    bool is_check = false;

};

class Board {
public:
    Board() {
        pieces_array.fill(Pieces::Blank);
    };

    void fen_decode(const std::string& fen);
    std::string fen_encode() const;
    void initialise();
    void initialise_starting_position() { fen_decode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"); }

    void print_board_idx();
    void print_board();
    void print_board_extra();
    void print_bitboard(const bitboard bb);

    std::string print_move(Move move, std::vector<Move> &legal_moves);
    bool is_free(const Square target) const;
    Square slide_to_edge(const Square origin, const Square direction, const uint to_edge) const;
    std::vector<Move> get_pseudolegal_moves() const;
    std::vector<Move> get_moves();
    std::vector<Move> get_sorted_moves();
    bool is_check(const Square square, const Piece colour) const;
    bool in_check() const{ return aux_info.is_check;};
    bool is_in_check() const;
    void update_checkers();

    std::array<Piece, 64> pieces() const{return pieces_array;}
    Piece pieces(Square sq) const{return pieces_array[sq];}
    Piece pieces(int sq) const{return pieces_array[sq];}

    void make_move(Move &move);
    void unmake_move(const Move move);
    void unmake_last() {unmake_move(last_move); }

    void try_move(const std::string move_sting);
    bool try_uci_move(const std::string move_sting);
    Square find_king(const Piece colour) const;
    void search_kings();
    void slide_bishop_pin(const Square origin, const Square direction, const uint to_edge, const Piece colour, const int idx);
    void slide_rook_pin(const Square origin, const Square direction, const uint to_edge, const Piece colour, const int idx);
    bool is_pinned(const Square origin) const;
    void build_occupied_bb();
    bool is_black_move() const{ return whos_move; }
    bool is_white_move() const{ return !whos_move; }
    bool can_castle(Colour c) const{ return c==WHITE ? (aux_info.castle_white_kingside | aux_info.castle_white_queenside) : (aux_info.castle_black_kingside | aux_info.castle_black_queenside);}
    AuxilliaryInfo aux_info;
private:
    std::array<Piece, 64> pieces_array;
    bitboard occupied;
    std::array<Square, 16> pinned_pieces;
    uint number_checkers;
    std::array<Square, 2> checkers;
    bool whos_move = white_move;
    uint fullmove_counter = 1;
    uint ply_counter = 0;
    Move last_move;
    std::array<Square, 2> king_square;
    // Array of absolute pins for legal move generation. Max 8 pieces per king.
    std::array<AuxilliaryInfo, 256> aux_history;
};

constexpr int mating_score = 100100;
bool is_mating(int score);
std::string print_score(int);
constexpr Square forwards(const Piece colour);
constexpr Square back_rank(const Piece colour);
bool interposes(const Square origin, const Square target, const Square query);
bool in_line(const Square, const Square, const Square);
bool in_line(const Square, const Square);

