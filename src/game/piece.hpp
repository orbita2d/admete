#pragma once
#include <ostream>
#include "types.hpp"
#include <vector>
#include <inttypes.h>

#define CMASK 0x18
#define PMASK 0x07


constexpr Colour operator~(Colour c) {
  return Colour(c ^ Colour::BLACK); // Toggle color
}

class Piece
{
public:

    Piece() = default;
    constexpr Piece(int aPiece) : value(uint8_t(aPiece)) { };
    constexpr Piece(bool is_black) : value(is_black ? 0x10 : 0x08) {};
    constexpr Piece(Colour c) : value( c == Colour::WHITE ? 0x08 : 0x10) {};
    constexpr Piece(Colour c, PieceType p) : value( (c == Colour::WHITE ? 0x08 : 0x10) | (p + 1) ) {};

    constexpr bool operator==(Piece that) const { return value == that.value; }
    constexpr bool operator!=(Piece that) const { return value != that.value; }
    constexpr Piece operator~() const {
        // Same piece but opposite colour
        return ((~value) & CMASK) | (value & PMASK);

    }

    constexpr Piece operator|(Piece that) const {return Piece(value | that.value);}

    std::string pretty_print() const;
    std::string prettier_print() const;

    constexpr Piece get_piece() const{
        return Piece(value & PMASK);
    }
    constexpr Piece get_colour() const{
        const uint8_t mask = CMASK;
        return Piece(value & mask);
    }

    constexpr bool is_colour(Piece colour) const {
        const uint8_t mask = CMASK;
        return uint8_t(get_colour()) == (colour.value & mask);
    }

    constexpr bool is_colour(Colour us) const {
        if (us == Colour::WHITE) {
            return is_white();
        } else {
            return is_black();
        }
    }

    constexpr bool is_piece(Piece piece) const {
        const uint8_t mask = PMASK;
        return uint8_t(get_piece()) == (piece.value & mask);
    }

    constexpr bool is_white() const {
        return (value & CMASK) == 0x08;
    }

    constexpr bool is_black() const {
        return (value & CMASK) == 0x10;
    }

    constexpr bool is_blank() const {
        return value == 0x00;
    }

    constexpr bool is_pawn() const {
        return (value & PMASK) == 0x01;
    }

    constexpr bool is_knight() const {
        return (value & PMASK) == 0x02;
    }

    constexpr bool is_bishop() const {
        return (value & PMASK) == 0x03;
    }

    constexpr bool is_rook() const {
        return (value & PMASK) == 0x04;
    }

    constexpr bool is_queen() const {
        return (value & PMASK) == 0x05;
    }

    constexpr bool is_king() const {
        return (value & PMASK) == 0x06;
    }

    constexpr bool is_piece(const PieceType p) const {
       return (value & PMASK) == p + 1;
    }
    constexpr int get_value() const {return value; }

    std::string get_algebraic_character() const;
    constexpr operator int() const {return value; }

private:
    int value;
};

namespace Pieces {
    static constexpr Piece Blank = Piece(0);
    static constexpr Piece Pawn = Piece(1);
    static constexpr Piece Knight = Piece(2);
    static constexpr Piece Bishop  = Piece(3);
    static constexpr Piece Rook = Piece(4);
    static constexpr Piece Queen = Piece(5);
    static constexpr Piece King = Piece(6);
    static constexpr Piece White = Piece(8);
    static constexpr Piece Black = Piece(16);
}

inline Colour to_enum_colour(const Piece p) {
    return p.is_white() ? WHITE : BLACK;
}
inline PieceType to_enum_piece(const Piece p) {
    return PieceType((p.get_value() & PMASK) - 1);
}
std::ostream& operator<<(std::ostream& os, const Piece piece);

// Uses four bits to store additional info about moves
enum MoveType {
    QUIETmv = 0,
    CAPTURE = 1,
    PROMOTION = 2,
    SPECIAL1 = 4,
    SPECIAL2 = 8,
    EN_PASSENT = 9,
    DOUBLE_PUSH = 8,
    KING_CASTLE = 4,
    QUEEN_CASTLE = 12
};

struct DenseMove {
    constexpr DenseMove() = default;
    constexpr DenseMove(const Square o, const Square t) : v((o.get_value() << 6) | t.get_value()) {};
    constexpr DenseMove(const Square o, const Square t, const MoveType m) : v((m << 12) | (o.get_value() << 6) | t.get_value()) {};
    Square target() const {return v & 0x003f;}
    Square origin() const {return (v >> 6) & 0x003f;}
    MoveType type() const {return (MoveType)((v >> 12) & 0x000f);}
    int16_t v = 0;
};

struct Move {
public:
    Move(const PieceType p, const Square o, const Square t) : origin(o), target(t), moving_piece(p) {};
    Move(const PieceType p, const Square o, const Square t, const MoveType m) : origin(o), target(t), moving_piece(p), type(m) {};
    constexpr Move() {};

    Square origin;
    Square target;
    int score = 0;

    std::string pretty() const{
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
                type == that.type;
    }

    void make_quiet() {
        type = QUIETmv;
    }
    void make_double_push() {
        type = DOUBLE_PUSH;
    }
    void make_king_castle() {
        type = KING_CASTLE;
    }
    void make_queen_castle() {
        type = QUEEN_CASTLE;
    }
    void make_capture() {
        type = CAPTURE;
    }
    void make_en_passent() {
        type = EN_PASSENT;
    }

    void make_bishop_promotion() {
        type = (MoveType)((type & CAPTURE) | (PROMOTION)) ;
    }
    void make_knight_promotion() {
        type = (MoveType)((type & CAPTURE) | (PROMOTION | SPECIAL2)) ;
    }
    void make_rook_promotion() {
        type = (MoveType)((type & CAPTURE) | (PROMOTION | SPECIAL1)) ;
    }
    void make_queen_promotion() {
        type = (MoveType)((type & CAPTURE) | (PROMOTION | SPECIAL1 | SPECIAL2)) ;
    }
    constexpr bool is_capture() const {
        return type & CAPTURE;
    }
    constexpr bool is_ep_capture() const {
        return type == EN_PASSENT;
    }
    constexpr bool is_double_push() const {
        return type == DOUBLE_PUSH;
    }
    constexpr bool is_king_castle() const {
        return type == KING_CASTLE;
    }
    constexpr bool is_queen_castle() const {
        return type == QUEEN_CASTLE;
    }
    constexpr bool is_knight_promotion() const {
        return (type & ~CAPTURE) == (PROMOTION);
    }
    constexpr bool is_bishop_promotion() const {
        return (type & ~CAPTURE) == (PROMOTION | SPECIAL2);
    }
    constexpr bool is_rook_promotion() const {
        return (type & ~CAPTURE) == (PROMOTION | SPECIAL1 );
    }
    constexpr bool is_queen_promotion() const {
        return (type & ~CAPTURE) == (PROMOTION | SPECIAL1 | SPECIAL2);
    }
    constexpr bool is_promotion() const {
        return (type & PROMOTION);
    }
    PieceType captured_piece = NO_PIECE;
    PieceType moving_piece = NO_PIECE;
    MoveType type = QUIETmv;
};

constexpr DenseMove NULL_DMOVE = DenseMove(0, 0);
constexpr Move NULL_MOVE = Move();

constexpr PieceType get_promoted(const Move m) {
    if (m.is_knight_promotion()) {
        return KNIGHT;
    } else if (m.is_bishop_promotion()) {
        return BISHOP;
    } else if (m.is_rook_promotion()) {
        return ROOK;
    } else if (m.is_queen_promotion()) {
        return QUEEN;
    }
    return NO_PIECE;
}
std::ostream& operator<<(std::ostream& os, const Move move);

inline bool operator==(const Move m, const DenseMove dm) {
    return (dm.origin() == m.origin && dm.target() == m.target && dm.type() == m.type);
}
inline bool operator==(const DenseMove dm, const Move m) {
    return m == dm;
}

typedef std::vector<Move> MoveList;

inline DenseMove pack_move(const Move m) {
    return DenseMove(m.origin, m.target, m.type);
}

inline Move unpack_move(const DenseMove dm, const MoveList& moves) {
    for (auto move: moves) {
        if (move == dm) { return move;}
    }
    return NULL_MOVE;
}