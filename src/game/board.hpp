#pragma once
#include "bitboard.hpp"
#include "types.hpp"
#include <array>
#include <map>
#include <string>
#include <vector>

struct AuxilliaryInfo {
    // Information that is game history dependent, that would otherwise need to be encoded in a move.
    // Access like castling_rights[WHITE][KINGSIDE]
    std::array<std::array<bool, N_COLOUR>, N_CASTLE> castling_rights;
    ply_t halfmove_clock = 0;
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

    // Move that brought us to this position.
    Move last_move = NULL_MOVE;
};

class Board {
  public:
    void fen_decode(const std::string &fen);
    std::string fen_encode() const;
    void initialise();
    void initialise_starting_position() { fen_decode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"); }

    Board() {
        aux_info = aux_history.begin();
        initialise_starting_position();
    };

    void pretty() const;

    bool is_free(const Square target) const;
    bool is_colour(const Colour c, const Square target) const;
    Square slide_to_edge(const Square origin, const Square direction, const uint to_edge) const;

    MoveList get_moves() const;
    MoveList get_capture_moves() const;
    MoveList get_evasion_moves() const;
    MoveList get_quiessence_moves() const;

    void get_moves(MoveList &) const;
    void get_capture_moves(MoveList &) const;
    void get_evasion_moves(MoveList &) const;
    void get_quiessence_moves(MoveList &) const;

    // Return true if there are no legal moves in the position.
    bool is_terminal() const { return !get_moves().empty(); }

    bool is_attacked(const Square square, const Colour us) const;
    void update_checkers();
    void update_check_squares();
    bool gives_check(const Move move);
    Bitboard check_squares(const PieceType p) const { return aux_info->check_squares[p]; };
    Bitboard blockers() const { return aux_info->blockers; };

    std::array<Square, 2> checkers() const { return aux_info->checkers; }
    Square checkers(int i) const { return aux_info->checkers[i]; }
    int number_checkers() const { return aux_info->number_checkers; }
    bool is_check() const { return aux_info->is_check; };

    zobrist_t hash() const { return hash_history[ply()]; }
    zobrist_t material_key() const;
    Piece pieces(const Square sq) const;
    PieceType piece_type(const Square sq) const;
    int count_pieces(const Colour c, const PieceType p) const { return piece_counts[c][p]; }
    Bitboard pieces() const { return occupied_bb; }
    Bitboard pieces(const PieceType p) const { return piece_bb[p]; }
    Bitboard pieces(const Colour c) const { return colour_bb[c]; }
    Bitboard pieces(const Colour c, const PieceType p) const { return colour_bb[c] & piece_bb[p]; }
    Bitboard pieces(const Colour c, const PieceType p1, const PieceType p2) const {
        return colour_bb[c] & (piece_bb[p1] | piece_bb[p2]);
    }
    Bitboard pinned() const { return aux_info->pinned; }
    Bitboard pawn_controlled(const Colour c) const { return pawn_atk_bb[c]; }
    Bitboard passed_pawns(const Colour c) const {
        return pieces(c, PAWN) & ~Bitboards::forward_block_span(~c, pieces(~c, PAWN));
    };
    Bitboard connected_passed_pawns(const Colour c) const {
        return passed_pawns(c) & Bitboards::full_atk_span(passed_pawns(c));
    }
    Bitboard open_files() const { return ~Bitboards::vertical_fill(pieces(PAWN)); }
    Bitboard half_open_files(const Colour c) const { return ~Bitboards::vertical_fill(pieces(c, PAWN)); }
    Bitboard weak_pawns(const Colour c) const { return pieces(c, PAWN) & ~pawn_controlled(c); }
    Bitboard isolated_pawns(const Colour c) const {
        return pieces(c, PAWN) & ~Bitboards::full_atk_span(pieces(c, PAWN));
    }
    Bitboard weak_squares(const Colour c) const {
        // Squares that can never be defended by a pawn
        return weak_sq_bb[c];
    }
    Bitboard outposts(const Colour c) const { return pawn_controlled(c) & weak_squares(~c); }

    void make_move(Move &move);
    void unmake_move(const Move move);

    void make_nullmove();
    void unmake_nullmove();

    Move fetch_move(const std::string move_sting);
    bool try_uci_move(const std::string move_sting);

    Square find_king(const Colour us) const;
    void search_kings();
    bool is_pinned(const Square origin) const;
    void build_occupied_bb();
    Colour who_to_play() const { return whos_move; }
    bool is_black_move() const { return whos_move == BLACK; }
    bool is_white_move() const { return whos_move == WHITE; }
    bool can_castle(const Colour c) const {
        return aux_info->castling_rights[c][KINGSIDE] | aux_info->castling_rights[c][QUEENSIDE];
    }
    bool can_castle(const Colour c, const CastlingSide s) const { return aux_info->castling_rights[c][s]; }
    Square en_passent() const { return aux_info->en_passent_target; }
    int material() const { return aux_info->material; }
    ply_t repetitions(const ply_t start) const;
    ply_t repetitions(const ply_t start, const ply_t query) const;
    bool is_draw() const;
    ply_t ply() const { return ply_counter; }
    ply_t height() const { return ply_counter - root_node_ply; }
    void set_root() { root_node_ply = ply_counter; }
    ply_t get_root() const { return root_node_ply; }
    bool is_root() const { return ply_counter == root_node_ply; }
    bool is_endgame() const;
    ply_t halfmove_clock() const { return aux_info->halfmove_clock; }
    void flip();
    Move last_move() const { return aux_info->last_move; }

  private:
    AuxilliaryInfo *aux_info;
    Bitboard occupied_bb;
    std::array<Bitboard, N_COLOUR> colour_bb;
    std::array<Bitboard, N_PIECE> piece_bb;
    std::array<Bitboard, N_COLOUR> pawn_atk_bb;
    std::array<Bitboard, N_COLOUR> weak_sq_bb;
    int piece_counts[N_COLOUR][N_PIECE];
    Colour whos_move = WHITE;
    uint fullmove_counter = 1;
    ply_t ply_counter = 0;
    std::array<Square, 2> king_square;
    std::array<AuxilliaryInfo, MAX_PLY> aux_history;
    std::array<zobrist_t, MAX_PLY> hash_history;
    ply_t root_node_ply;
};

inline Move unpack_move(const DenseMove dm, const Board &board) {
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

constexpr Square relative_rank(const Colour c, const Square sq) {
    // Square from black's perspective perspective;
    if (c == Colour::WHITE) {
        return sq;
    } else {
        return Square(56 - sq.rank()) | sq.file();
    }
}

// zorbist.cpp
namespace Zobrist {
void init();
zobrist_t hash(const Board &board);
zobrist_t material(const Board &board);
zobrist_t diff(const Move move, const Colour us, const int last_ep_file,
               const std::array<std::array<bool, N_COLOUR>, N_CASTLE> castling_rights_change);
zobrist_t nulldiff(const Colour us, const int last_ep_file);

inline zobrist_t zobrist_table[N_COLOUR][N_PIECE][N_SQUARE];
inline zobrist_t zobrist_table_cr[N_COLOUR][2];
inline zobrist_t zobrist_table_move[N_COLOUR];
inline zobrist_t zobrist_table_ep[8];

} // namespace Zobrist