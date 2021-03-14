#pragma once
#include <map>
#include <array>
#include <vector>
#include <string>

#include "types.hpp"
#include "piece.hpp"
#include "bitboard.hpp"

class Move {
public:
    Move(const Square o, const Square t) : origin(o), target(t) {};
    constexpr Move() {};

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
    Piece captured_peice = 0;
private:
    bool promotion = 0;
    bool capture = 0;
    bool special1 = 0;
    bool special2 = 0;
};
constexpr Move NULL_MOVE = Move();
std::ostream& operator<<(std::ostream& os, const Move move);

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


struct AuxilliaryInfo {
    // Information that is game history dependent, that would otherwise need to be encoded in a move.
    // Access linke castling_rights[WHITE][KINGSIDE]
    std::array<std::array<bool, 2>, 2> castling_rights;
    uint halfmove_clock = 0;
    Square en_passent_target;
    std::array<Square, 16> pinned_pieces;
    uint number_checkers;
    std::array<Square, 2> checkers;
    bool is_check = false;

};

void init_zobrist();
class Board {
public:
    Board() {
        pieces_array.fill(Pieces::Blank);
        init_zobrist();
    };

    void fen_decode(const std::string& fen);
    std::string fen_encode() const;
    void initialise();
    void initialise_starting_position() { fen_decode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"); }

    void pretty() const;

    std::string print_move(Move move, std::vector<Move> &legal_moves);
    bool is_free(const Square target) const;
    bool is_colour(const Colour c, const Square target) const;
    Square slide_to_edge(const Square origin, const Square direction, const uint to_edge) const;
    void get_pseudolegal_moves(std::vector<Move> &quiet_moves, std::vector<Move> &captures) const;
    std::vector<Move> get_evasion_moves() const;
    std::vector<Move> get_moves();
    std::vector<Move> get_captures();
    std::vector<Move> get_sorted_moves();
    bool is_attacked(const Square square, const Colour colour) const;
    void update_checkers();

    std::array<Square, 2> checkers() const {return _checkers;}
    int number_checkers() const {return _number_checkers;}
    bool is_check() const{ return aux_info.is_check;};

    long int hash() const;
    std::array<Piece, 64> pieces() const{return pieces_array;}
    Piece pieces(Square sq) const{return pieces_array.at(sq);}
    Piece pieces(int sq) const{return pieces_array[sq];}

    void make_move(Move &move);
    void unmake_move(const Move move);

    void try_move(const std::string move_sting);
    bool try_uci_move(const std::string move_sting);
    Square find_king(const Colour colour) const;
    void search_kings();
    void slide_bishop_pin(const Square origin, const Square direction, const uint to_edge, const Colour colour, const int idx);
    void slide_rook_pin(const Square origin, const Square direction, const uint to_edge, const Colour colour, const int idx);
    bool is_pinned(const Square origin) const;
    void build_occupied_bb();
    Colour who_to_play() const { return whos_move; }
    bool is_black_move() const{ return whos_move == BLACK; }
    bool is_white_move() const{ return whos_move == WHITE; }
    bool can_castle(const Colour c) const{ return aux_info.castling_rights[c][KINGSIDE] | aux_info.castling_rights[c][QUEENSIDE];}
    bool can_castle(const Colour c, const CastlingSide s) const { return aux_info.castling_rights[c][s]; }
    Square en_passent() const { return aux_info.en_passent_target; }
    AuxilliaryInfo aux_info;
private:
    std::array<Piece, N_SQUARE> pieces_array;
    Bitboard occupied_bb;
    std::array<Bitboard, N_COLOUR> colour_bb;
    std::array<Bitboard, N_PIECE> piece_bb;
    std::array<Square, 16> pinned_pieces;
    uint _number_checkers;
    std::array<Square, 2> _checkers;
    Colour whos_move = WHITE;
    uint fullmove_counter = 1;
    uint ply_counter = 0;
    std::array<Square, 2> king_square;
    // Array of absolute pins for legal move generation. Max 8 pieces per king.
    std::array<AuxilliaryInfo, 256> aux_history;
};

constexpr int mating_score = 100100;
bool is_mating(int score);
std::string print_score(int);

constexpr Direction forwards(const Colour colour) {
    if (colour == WHITE) {
        return Direction::N;
    } else {
        return Direction::S;
    }
}

constexpr Square back_rank(const Colour colour) {
    if (colour == WHITE) {
        return Squares::Rank1;
    } else {
        return Squares::Rank8;
    }
}

bool interposes(const Square origin, const Square target, const Square query);
bool in_line(const Square, const Square, const Square);
bool in_line(const Square, const Square);


constexpr Square relative_rank (const Colour c, const Square sq) {
    // Square from black's perspective perspective;
    if (c == Colour::WHITE) {
        return sq;
    } else {
        return Square(56 - sq.rank()) | sq.file();
    }
}