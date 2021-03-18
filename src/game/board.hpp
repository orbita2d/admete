#pragma once
#include <map>
#include <array>
#include <vector>
#include <string>

#include "types.hpp"
#include "piece.hpp"
#include "bitboard.hpp"


struct AuxilliaryInfo {
    // Information that is game history dependent, that would otherwise need to be encoded in a move.
    // Access linke castling_rights[WHITE][KINGSIDE]
    std::array<std::array<bool, 2>, 2> castling_rights;
    uint halfmove_clock = 0;
    Square en_passent_target;
    Bitboard pinned;
    uint number_checkers;
    std::array<Square, 2> checkers;
    bool is_check = false;
    // Squares where, if a particular piece was placed, it would give check.
    Bitboard check_squares[N_PIECE];
    // Pieces belonging to the player to move, that if moved would give discovered check.
    Bitboard blockers;
    int material;

};

class Board {
public:
    Board() {
        aux_info = aux_history.begin();
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
    std::vector<Move> get_evasion_moves() const;
    std::vector<Move> get_moves();
    std::vector<Move> get_captures();
    std::vector<Move> get_sorted_moves();
    bool is_attacked(const Square square, const Colour us) const;
    void update_checkers();
    void update_check_squares();
    bool gives_check(Move move);
    Bitboard check_squares(const PieceEnum p) const {return aux_info->check_squares[p];};
    Bitboard blockers() const {return aux_info->blockers;};

    std::array<Square, 2> checkers() const {return _checkers;}
    int number_checkers() const {return _number_checkers;}
    bool is_check() const{ return aux_info->is_check;};

    long int hash() const;
    Piece pieces(const Square sq) const;
    Bitboard pieces() const {return occupied_bb;}
    Bitboard pieces(const PieceEnum p) const{return piece_bb[p];}
    Bitboard pieces(const Colour c) const{return colour_bb[c];}
    Bitboard pieces(const Colour c, const PieceEnum p) const{return colour_bb[c] & piece_bb[p];}
    Bitboard pieces(const Colour c, const PieceEnum p1, const PieceEnum p2) const{return colour_bb[c] & (piece_bb[p1] |piece_bb[p2]);}
    Bitboard pinned() const {return pinned_bb;}

    void make_move(Move &move);
    void unmake_move(const Move move);

    void try_move(const std::string move_sting);
    bool try_uci_move(const std::string move_sting);
    Square find_king(const Colour us) const;
    void search_kings();
    bool is_pinned(const Square origin) const;
    void build_occupied_bb();
    Colour who_to_play() const { return whos_move; }
    bool is_black_move() const{ return whos_move == BLACK; }
    bool is_white_move() const{ return whos_move == WHITE; }
    bool can_castle(const Colour c) const{ return aux_info->castling_rights[c][KINGSIDE] | aux_info->castling_rights[c][QUEENSIDE];}
    bool can_castle(const Colour c, const CastlingSide s) const { return aux_info->castling_rights[c][s]; }
    Square en_passent() const { return aux_info->en_passent_target; }
    int material() const { return aux_info->material; }
private:
    AuxilliaryInfo* aux_info;
    Bitboard occupied_bb;
    Bitboard pinned_bb;
    std::array<Bitboard, N_COLOUR> colour_bb;
    std::array<Bitboard, N_PIECE> piece_bb;
    uint _number_checkers;
    std::array<Square, 2> _checkers;
    Colour whos_move = WHITE;
    uint fullmove_counter = 1;
    uint ply_counter = 0;
    std::array<Square, 2> king_square;
    std::array<AuxilliaryInfo, 256> aux_history;
};

constexpr int mating_score = 100100;
bool is_mating(int score);
std::string print_score(int);

constexpr Direction forwards(const Colour us) {
    if (us == WHITE) {
        return Direction::N;
    } else {
        return Direction::S;
    }
}

constexpr Square back_rank(const Colour us) {
    if (us == WHITE) {
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

namespace Zorbist {
    void init();
    long hash(const Board& board);
}