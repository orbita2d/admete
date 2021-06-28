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
    // Access like castling_rights[WHITE][KINGSIDE]
    std::array<std::array<bool, N_COLOUR>, N_CASTLE> castling_rights;
    uint halfmove_clock = 0;
    Square en_passent_target;
    Bitboard pinned;
    uint number_checkers;
    // Locations of the (up to 2) checkers 
    std::array<Square, 2> checkers;
    bool is_check = false;
    // Squares where, if a particular piece type was placed, it would give check.
    Bitboard check_squares[N_PIECE];

    // Pieces belonging to the player to move, that if moved would give discovered check.
    Bitboard blockers;
    int material;

};

class Board {
public:

    void fen_decode(const std::string& fen);
    std::string fen_encode() const;
    void initialise();
    void initialise_starting_position() { fen_decode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"); }

    Board() {
        aux_info = aux_history.begin();
        initialise_starting_position();
    };
    
    void pretty() const;

    std::string print_move(Move move, MoveList &legal_moves);
    bool is_free(const Square target) const;
    bool is_colour(const Colour c, const Square target) const;
    Square slide_to_edge(const Square origin, const Square direction, const uint to_edge) const;
    MoveList get_evasion_moves() const;
    MoveList get_moves();
    MoveList get_quiessence_moves();
    void sort_moves(MoveList &legal_moves, const DenseMove hash_move, const KillerTableRow killer_move);

    bool is_attacked(const Square square, const Colour us) const;
    void update_checkers();
    void update_check_squares();
    bool gives_check(Move move);
    Bitboard check_squares(const PieceType p) const {return aux_info->check_squares[p];};
    Bitboard blockers() const {return aux_info->blockers;};

    std::array<Square, 2> checkers() const {return aux_info->checkers;}
    Square checkers(int i) const {return aux_info->checkers[i];}
    int number_checkers() const {return aux_info->number_checkers;}
    bool is_check() const{ return aux_info->is_check;};

    long int hash() const { return hash_history[ply()]; }
    Piece pieces(const Square sq) const;
    PieceType piece_type(const Square sq) const;
    int count_pieces(const Colour c, const PieceType p) const{ return piece_counts[c][p]; }
    Bitboard pieces() const {return occupied_bb;}
    Bitboard pieces(const PieceType p) const{return piece_bb[p];}
    Bitboard pieces(const Colour c) const{return colour_bb[c];}
    Bitboard pieces(const Colour c, const PieceType p) const{return colour_bb[c] & piece_bb[p];}
    Bitboard pieces(const Colour c, const PieceType p1, const PieceType p2) const{return colour_bb[c] & (piece_bb[p1] |piece_bb[p2]);}
    Bitboard pinned() const {return aux_info->pinned;}
    Bitboard pawn_controlled(const Colour c) const {return pawn_atk_bb[c];}
    Bitboard passed_pawns(const Colour c) const {return pieces(c, PAWN) & ~Bitboards::forward_block_span(~c, pieces(~c, PAWN)); };
    Bitboard connected_passed_pawns(const Colour c) const {return passed_pawns(c) & Bitboards::full_atk_span(passed_pawns(c));}
    Bitboard open_files() const {return ~Bitboards::vertical_fill(pieces(PAWN)); }
    Bitboard half_open_files(const Colour c) const {return ~Bitboards::vertical_fill(pieces(c, PAWN));}
    Bitboard weak_pawns(const Colour c) const {return pieces(c, PAWN) & ~pawn_controlled(c);}
    Bitboard isolated_pawns(const Colour c) const {return pieces(c, PAWN) & ~Bitboards::full_atk_span(pieces(c, PAWN));}
    Bitboard weak_squares(const Colour c) const {return Bitboards::middle_ranks & ~Bitboards::forward_atk_span(c, pieces(c, PAWN));} // Squares that can never be defended by a pawn
    Bitboard outposts(const Colour c) const {return pawn_controlled(c) & weak_squares(~c);} 

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
    void set_root() { root_node_ply = ply_counter; }
    ply_t get_root() const{ return root_node_ply; }
    ply_t repetitions(const ply_t start) const;
    bool is_draw() const;
    ply_t ply() const { return ply_counter;}
    ply_t height() const { return ply_counter - root_node_ply;}
    bool is_endgame() const;

private:
    AuxilliaryInfo* aux_info;
    Bitboard occupied_bb;
    std::array<Bitboard, N_COLOUR> colour_bb;
    std::array<Bitboard, N_PIECE> piece_bb;
    std::array<Bitboard, N_COLOUR> pawn_atk_bb;
    int piece_counts[N_COLOUR][N_PIECE];
    Colour whos_move = WHITE;
    uint fullmove_counter = 1;
    ply_t ply_counter = 0;
    std::array<Square, 2> king_square;
    std::array<AuxilliaryInfo, MAX_PLY> aux_history;
    std::array<long, MAX_PLY> hash_history;
    ply_t root_node_ply;
};

std::string print_score(int);

inline Move unpack_move(const DenseMove dm, const Board& board) {
    PieceType p = board.piece_type(dm.origin());
    return Move(p, dm.origin(), dm.target(), dm.type());
}

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
    long diff(const Move move, const Colour us, const int last_ep_file, const std::array<std::array<bool, N_COLOUR>, N_CASTLE> castling_rights_change);
}