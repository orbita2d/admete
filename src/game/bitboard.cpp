#include <iostream>

#include "bitboard.hpp"

void Bitboards::pretty(const Bitboard bb) {
    for (uint rank = 0; rank< 8; rank++) {
        for (uint file = 0; file< 8; file++) {
            uint idx = 8*rank +file;
            std::cout << (((bb >> idx) & 1) ? "X" : ".") << ' ';
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;

}

void Bitboards::init() {
    // Zero-initialise
    
    for (int sq = 0; sq < N_SQUARE; sq++) {
        PawnAttacks[WHITE][sq] = 0;
        PawnAttacks[BLACK][sq] = 0;
    }
    for (int p = 0; p < N_PIECE; p++) {
        for (int sq = 0; sq < N_SQUARE; sq++) {
            PseudolegalAttacks[p][sq] = 0;
        }
    }
    for (int i = 0; i < N_SQUARE; i++) {
        Square origin = Square(i);
        Bitboard origin_bb = sq_to_bb(origin);
        // Knight attacks
        if (origin.to_north() >= 2){
            if(origin.to_west() >= 1) {
                PseudolegalAttacks[KNIGHT][origin] |= Square(origin + Direction::NNW);
            }
            if(origin.to_east() >= 1) {
               PseudolegalAttacks[KNIGHT][origin] |= Square(origin + Direction::NNE);
            }
        }
        if (origin.to_east() >= 2){
            if(origin.to_north() >= 1) {
                PseudolegalAttacks[KNIGHT][origin] |= Square(origin + Direction::ENE);
            }
            if(origin.to_south() >= 1) {
                PseudolegalAttacks[KNIGHT][origin] |= Square(origin + Direction::ESE);
            }
        }
        if (origin.to_south() >= 2){
            if(origin.to_east() >= 1) {
                PseudolegalAttacks[KNIGHT][origin] |= Square(origin + Direction::SSE);
            }
            if(origin.to_west() >= 1) {
                PseudolegalAttacks[KNIGHT][origin] |= Square(origin + Direction::SSW);
            }
        }
        if (origin.to_west() >= 2){
            if(origin.to_south() >= 1) {
                PseudolegalAttacks[KNIGHT][origin] |= Square(origin + Direction::WSW);
            }
            if(origin.to_north() >= 1) {
                PseudolegalAttacks[KNIGHT][origin] |= Square(origin + Direction::WNW);
            }
        }
        // King attacks
        if (origin.to_north() >= 1){
            PseudolegalAttacks[KING][origin] |= Square(origin + Direction::N);
            if(origin.to_west() >= 1) {
                PseudolegalAttacks[KING][origin] |= Square(origin + Direction::NW);
            }
            if(origin.to_east() >= 1) {
               PseudolegalAttacks[KING][origin] |= Square(origin + Direction::NE);
            }
        }
        if (origin.to_west() >= 1){
               PseudolegalAttacks[KING][origin] |= Square(origin + Direction::W);
        }
        if (origin.to_east() >= 1){
               PseudolegalAttacks[KING][origin] |= Square(origin + Direction::E);
        }
        if (origin.to_south() >= 1){
            PseudolegalAttacks[KING][origin] |= Square(origin + Direction::S);
            if(origin.to_east() >= 1) {
                PseudolegalAttacks[KING][origin] |= Square(origin + Direction::SE);
            }
            if(origin.to_west() >= 1) {
                PseudolegalAttacks[KING][origin] |= Square(origin + Direction::SW);
            }
        }
        PawnAttacks[WHITE][origin] = shift<Direction::NW>(origin_bb) | shift<Direction::NE>(origin_bb);
        PawnAttacks[BLACK][origin] = shift<Direction::SW>(origin_bb) | shift<Direction::SE>(origin_bb);
    }
}