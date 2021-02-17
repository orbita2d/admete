#include <cstdint>

#pragma once

class Piece
{
public:
    enum Value : uint8_t
    {
        Blank,
        Pawn,
        Knight,
        Bishop,
        Rook,
        Queen,
        King,
        White = 16,
        Black = 32
    };

    Piece() = default;
    constexpr Piece(Value aPiece) : value(aPiece) { }
    constexpr Piece(int aPiece) : value(uint8_t(aPiece)) { }
    constexpr bool operator==(Piece that) const { return value == that.value; }
    constexpr bool operator!=(Piece that) const { return value != that.value; }
    char pretty_print();

    const uint8_t get_piece(){
        uint8_t mask = 0x0F;
        return value & mask;
    }
    const uint8_t get_colour(){
        uint8_t mask = 0xF0;
        return value & mask;
    }
private:
    uint8_t value;
};
