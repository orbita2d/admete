#include <cstdint>
#include <ostream>

#pragma once

#define CMASK 0x18
#define PMASK 0x07


enum Colour : bool {
    WHITE,
    BLACK
};

enum PieceEnum {
    PAWN,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING
};

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
    constexpr Piece(Colour c, PieceEnum p) : value( (c == Colour::WHITE ? 0x08 : 0x10) | (p + 1) ) {};

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

    constexpr bool is_colour(Colour colour) const {
        if (colour == Colour::WHITE) {
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

    constexpr bool is_piece(const PieceEnum p) const {
       return (value & PMASK) == p + 1;
    }
    constexpr uint8_t get_value() const {return value; }

    std::string get_algebraic_character() const;
    constexpr operator uint8_t() const {return value; }

private:
    uint8_t value;
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
inline PieceEnum to_enum_piece(const Piece p) {
    return PieceEnum((p.get_value() & PMASK) - 1);
}
std::ostream& operator<<(std::ostream& os, const Piece piece);
