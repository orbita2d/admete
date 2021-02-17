#include <map>
#include <array>
#include <vector>
#include <string>
#include "piece.hpp"


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

    static uint to_index(uint rank, uint file) {
        return 8 * rank + file;
    }
    static uint str_to_index(const std::string& rf);
    static uint squares_to_north(const uint idx);
    static uint squares_to_south(const uint idx);
    static uint squares_to_east(const uint idx);
    static uint squares_to_west(const uint idx);

    // Unsigned coordinate
    typedef uint coord;
    // Signed coordinate
    typedef signed int s_coord;
    static const s_coord N = -8;
    static const s_coord E =  1;
    static const s_coord S =  8;
    static const s_coord W = -1;
    static const s_coord NW = N + W;
    static const s_coord NE = N + E;
    static const s_coord SE = S + E;
    static const s_coord SW = S + W;
    static const s_coord NNW = N + N + W;
    static const s_coord NNE = N + N + E;
    static const s_coord ENE = E + N + E;
    static const s_coord ESE = E + S + E;
    static const s_coord SSE = S + S + E;
    static const s_coord SSW = S + S + W;
    static const s_coord WSW = W + S + W;
    static const s_coord WNW = W + N + W;

    static std::string idx_to_str(coord idx);
    std::vector<Board::coord> possible_rook_moves(const Board::coord starting_point);
private:
    std::array<Piece, 64> pieces;
    bool whos_move = white_move;
    bool castle_white_kingside = true;
    bool castle_white_queenside = true;
    bool castle_black_kingside = true;
    bool castle_black_queenside = true;
    uint en_passent_target = 0;
    uint halfmove_clock = 0;
    uint fullmove_counter = 1;
};

std::vector<Board::coord> possible_knight_moves(const Board::coord starting_point);
