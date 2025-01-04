#pragma once
#include "bitboard.hpp"
#include "types.hpp"
#include <array>
#include <map>
#include <string>
#include <vector>
#include <features.hpp>
#include <weights.hpp>

struct DenseBoard {
    per_colour<Bitboard> colour_bb;
    per_piece<Bitboard> piece_bb;
    Colour whos_move;
};

// Information that is game history dependent, that would otherwise need to be encoded in a move.
struct AuxilliaryInfo {
    // Holds the castling rights data, with bit flags set from CastlingRights enum.
    unsigned castling_rights;
    // Half-moves since last pawn move or capture.
    ply_t halfmove_clock = 0;
    // En-passent file, set to NO_FILE if ep is illegal.
    File en_passent_target;
    // Pieces pinned to the king for the side to move.
    Bitboard pinned;
    // Number of checkers in the current position.
    uint number_checkers;
    // Locations of the (up to 2) checkers.
    std::array<Square, 2> checkers;
    // If we are currently in check. Same as number_chekers > 0;
    bool is_check = false;
    // Squares where, if a particular piece type was placed, it would give check.
    per_piece<Bitboard> check_squares;
    // Pieces belonging to the player to move, that if moved would give discovered check.
    Bitboard blockers;
    // Squares that the *other* player is attacking, ignoring pins.
    Bitboard attacked;
    // Move that brought us to this position.
    Move last_move = NULL_MOVE;
};

class Board {
  public:
    void fen_decode(const std::string &fen);
    std::string fen_encode() const;
    void initialise();
    void initialise_starting_position() { fen_decode("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"); }

    DenseBoard pack() const {
        DenseBoard db;
        db.colour_bb = colour_bb;
        db.piece_bb = piece_bb;
        db.whos_move = whos_move;
        return db;
    };

    void unpack(const DenseBoard &db) {
        aux_info = &(*aux_history.begin());
        occupied_bb = db.colour_bb[WHITE] | db.colour_bb[BLACK];
        colour_bb = db.colour_bb;
        piece_bb = db.piece_bb;
        whos_move = db.whos_move;
        initialise();
    }

    Board() {
        aux_info = &(*aux_history.begin());
        initialise_starting_position();
    };

    Board(DenseBoard &db) { unpack(db); };

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

    // Find the locations of the kings.
    void search_kings();
    // Find the checkers and pinned pieces in a position.
    void update_checkers();
    // Find the check sqaures and blocker pieces (discovered checks) in a position.
    void update_check_squares();
    // Find the squares that are being attacked.
    void update_attacks();
    // Calculate the extra pawn structure information used in eval.
    void update_pawns();
    // Returns true if a given move will give check.
    bool gives_check(const Move move) const;
    // Returns the squares where p would give check.
    Bitboard check_squares(const PieceType p) const { return aux_info->check_squares[p]; };
    // Returns the bitboard of blockers.
    Bitboard blockers() const { return aux_info->blockers; };
    // Returns the bitboard of pieces pinned to king.
    Bitboard pinned() const { return aux_info->pinned; }
    // Returns the bitboard of pieces pinned to king.
    Bitboard attacked() const { return aux_info->attacked; }

    // Returns the location of the i'th checker.
    Square checkers(int i) const {
        assert(i >= 0);
        assert(i < 2);
        return aux_info->checkers[i];
    }
    int number_checkers() const { return aux_info->number_checkers; }
    bool is_check() const { return aux_info->is_check; };

    zobrist_t hash() const { return hash_history[ply()]; }
    zobrist_t material_key() const;
    Piece pieces(const Square &sq) const;
    PieceType piece_type(const Square &sq) const;
    int count_pieces(const Colour c, const PieceType p) const { return piece_counts[c][p]; }
    Bitboard pieces() const { return occupied_bb; }
    Bitboard pieces(const PieceType p) const { return piece_bb[p]; }
    Bitboard pieces(const Colour c) const { return colour_bb[c]; }
    Bitboard pieces(const Colour c, const PieceType p) const { return colour_bb[c] & piece_bb[p]; }
    Bitboard pieces(const Colour c, const PieceType p1, const PieceType p2) const {
        return colour_bb[c] & (piece_bb[p1] | piece_bb[p2]);
    }
    Bitboard pawn_controlled(const Colour c) const { return pawn_atk_bb[c]; }
    Bitboard passed_pawns(const Colour c) const { return passed_pawn_bb[c]; };
    Bitboard connected_passed_pawns(const Colour c) const {
        return passed_pawns(c) & Bitboards::full_atk_span(passed_pawns(c));
    }
    Bitboard open_files() const { return ~Bitboards::vertical_fill(pieces(PAWN)); }
    Bitboard half_open_files(const Colour c) const { return ~Bitboards::vertical_fill(pieces(c, PAWN)); }
    Bitboard weak_pawns(const Colour c) const { return pieces(c, PAWN) & ~pawn_controlled(c); }
    Bitboard isolated_pawns(const Colour c) const {
        return pieces(c, PAWN) & ~Bitboards::full_atk_span(pieces(c, PAWN));
    }
    // Squares that can never be defended by c's pawn.
    Bitboard weak_squares(const Colour c) const { return weak_sq_bb[c]; }
    // Squares that can never be defended by their pawn, that are controlled by our pawns.
    Bitboard outposts(const Colour c) const { return pawn_controlled(c) & weak_squares(~c); }

    void make_move(Move &move);
    void unmake_move(const Move move);

    void make_nullmove();
    void unmake_nullmove();

    Move fetch_move(const std::string move_sting);
    bool try_uci_move(const std::string move_sting);

    // Get the square the king is on.
    Square find_king(const Colour us) const;
    // Who's turn is it.
    Colour who_to_play() const { return whos_move; }
    // Returns true if it's black's turn.
    bool is_black_move() const { return whos_move == BLACK; }
    // Returns true if it's white's turn.
    bool is_white_move() const { return whos_move == WHITE; }
    // True if colour c can castle at all.
    bool can_castle(const Colour c) const { return aux_info->castling_rights & get_rights(c); }
    // True if colour c can castle on side s.
    bool can_castle(const Colour c, const CastlingSide s) const { return aux_info->castling_rights & get_rights(c, s); }
    // Returns en_passent file.
    File en_passent() const { return aux_info->en_passent_target; }
    // How much material in total on the board, used to determine game phase.
    score_t phase_material() const { return _phase_material; }
    // How many times has the current position occured since ply 'start'.
    ply_t repetitions(const ply_t start) const;
    // How many times has the position at ply 'query' occured since ply 'start'.
    ply_t repetitions(const ply_t start, const ply_t query) const;
    // Returns true if draw by repetition, rule of 50 or insufficent material.
    bool is_draw() const;
    // What is our ply since the startpos.
    ply_t ply() const { return ply_counter; }
    // How many ply we are from the root node.
    ply_t height() const { return ply_counter - root_node_ply; }
    // Set the current node as the root node.
    void set_root() { root_node_ply = ply_counter; }
    // Returns the ply of the root node.
    ply_t get_root() const { return root_node_ply; }
    // True if the current node is the root node.
    bool is_root() const { return ply_counter == root_node_ply; }
    // True if the amount of material on the board is below the endgame threshold.
    bool is_endgame() const;
    // How many ply since the last pawn move or capture.
    ply_t halfmove_clock() const { return aux_info->halfmove_clock; }
    // Reverse the board, turns white into black and visa-versa.
    void flip();
    // Returns the move that got us to this node.
    Move last_move() const { return aux_info->last_move; }

    const Neural::accumulator_t &accumulator() const { return _accumulator; }

  private:
    AuxilliaryInfo *aux_info;
    Bitboard occupied_bb;
    per_colour<Bitboard> colour_bb;
    per_piece<Bitboard> piece_bb;
    per_colour<Bitboard> pawn_atk_bb;
    per_colour<Bitboard> weak_sq_bb;
    per_colour<Bitboard> passed_pawn_bb;
    int piece_counts[N_COLOUR][N_PIECE];
    Colour whos_move = WHITE;
    uint fullmove_counter = 1;
    ply_t ply_counter = 0;
    std::array<Square, 2> king_square;
    std::array<AuxilliaryInfo, MAX_PLY> aux_history;
    std::array<zobrist_t, MAX_PLY> hash_history;
    ply_t root_node_ply;
    score_t _phase_material;

    Neural::accumulator_t _accumulator = Neural::get_accumulator();
};

inline Move unpack_move(const DenseMove dm, const Board &board) {
    PieceType p = board.piece_type(dm.origin());
    return Move(p, dm.origin(), dm.target(), dm.type());
}

bool interposes(const Square origin, const Square target, const Square query);
bool in_line(const Square, const Square, const Square);
bool in_line(const Square, const Square);