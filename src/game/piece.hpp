#include <cstdint>
#include <ostream>

#pragma once

#define CMASK 0x18
#define PMASK 0x07

class Piece
{
public:

    Piece() = default;
    constexpr Piece(int aPiece) : value(uint8_t(aPiece)) { };
    constexpr Piece(bool is_white) : value(is_white ? 0x10 : 0x08) {};

    constexpr bool operator==(Piece that) const { return value == that.value; }
    constexpr bool operator!=(Piece that) const { return value != that.value; }
    constexpr Piece operator~() const {
        // Same piece but opposite colour
        return ((~value) & CMASK) | (value & PMASK);

    }

    constexpr Piece operator|(Piece that) const {return Piece(value | that.value);}

    char pretty_print() const;

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

std::ostream& operator<<(std::ostream& os, const Piece piece);

/*
constexpr bool operator==(Piece thing1, Piece::Value thing2) { return uint8_t(thing1) == thing2; }
constexpr bool operator==(Piece::Value thing2, Piece thing1) { return uint8_t(thing1) == thing2; }
*/