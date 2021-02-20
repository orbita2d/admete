#include <cstdint>
#include <ostream>

#pragma once

class Piece
{
public:

    Piece() = default;
    constexpr Piece(int aPiece) : value(uint8_t(aPiece)) { }

    constexpr bool operator==(Piece that) const { return value == that.value; }
    constexpr bool operator!=(Piece that) const { return value != that.value; }
    constexpr Piece operator~() const {
        // Same piece but opposite colour
        const uint8_t colour_mask = 0x30;
        const uint8_t piece_mask = 0x0F;
        return ((~value) & colour_mask) | (value & piece_mask);

    }

    constexpr Piece operator|(Piece that) const {return Piece(value | that.value);}

    char pretty_print() const;

    constexpr Piece get_piece() const{
        const uint8_t mask = 0x0F;
        return Piece(value & mask);
    }
    constexpr Piece get_colour() const{
        const uint8_t mask = 0x30;
        return Piece(value & mask);
    }

    constexpr bool is_colour(Piece colour) const {
        const uint8_t mask = 0x30;
        return uint8_t(get_colour()) == (colour.value & mask);
    }

    constexpr bool is_piece(Piece piece) const {
        const uint8_t mask = 0x0F;
        return uint8_t(get_piece()) == (piece.value & mask);
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
    static constexpr Piece White = Piece(16);
    static constexpr Piece Black = Piece(32);
}

std::ostream& operator<<(std::ostream& os, const Piece piece);

/*
constexpr bool operator==(Piece thing1, Piece::Value thing2) { return uint8_t(thing1) == thing2; }
constexpr bool operator==(Piece::Value thing2, Piece thing1) { return uint8_t(thing1) == thing2; }
*/