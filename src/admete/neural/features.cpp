#include <bitboard.hpp>
#include <board.hpp>
#include <network.hpp>
#include "features.hpp"

namespace Neural {
per_colour<FeatureVector> encode(const Board &board) { 
    per_colour<FeatureVector> features;
    for (Colour c : {WHITE, BLACK}) {
        std::get<0>(features[c]) = Vector<feature_t, N_FEATURES>::zeros();

        for (PieceType p = PAWN; p < KING; p++) {
            Bitboard bb = board.pieces(c, p);
            while (bb) {
                Square sq = pop_lsb(&bb);
                std::get<0>(features[c])[p * 64 + sq.relative(c)] = 1;
            }
        }

        // King is special, we need to know where it is.
        Square king_sq = board.find_king(c);
        std::get<1>(features[c]) = king_sq.relative(c);
    }
    return features;
}

per_colour<FeatureDiff> increment(const Move &move, const Colour us, const bool forward) {
    assert(move != NULL_MOVE);
    const PieceType p = move.moving_piece;
    const Colour them = ~us;
    assert(p < NO_PIECE);
    Square origin = move.origin.relative(us);
    Square target = move.target.relative(us);
    const int inc = forward ? 1 : -1;
    per_colour<FeatureDiff> diffs;
    diffs[WHITE] = FeatureDiff();
    diffs[BLACK] = FeatureDiff();
    // set the king squares as Square::Invalid() to indicate no change
    for (auto c : {WHITE, BLACK}) {
        std::get<1>(diffs[c]) = Square::Invalid();
        std::get<2>(diffs[c]) = Square::Invalid();
    }

    if (move.moving_piece == KING && forward) {
        std::get<2>(diffs[us]) = target;
        std::get<1>(diffs[us]) = origin;
    } else if (move.moving_piece == KING)  {
        // TODO: this is awkward, we could move the check into a later stage?
        std::get<1>(diffs[us]) = target;
        std::get<2>(diffs[us]) = origin;
    }else {
        std::get<0>(diffs[us]).set(p * 64 + origin, -inc);
    }

    if (move.is_promotion()) {
        const PieceType promoted = get_promoted(move);
        assert(promoted < NO_PIECE);
        std::get<0>(diffs[us]).set(promoted * 64 + target, inc);
    } else if (move.moving_piece != KING) {
        std::get<0>(diffs[us]).set(p * 64 + target, inc);
    }

    if (move.is_ep_capture()) {
        const Square captured_square(move.origin.rank(), move.target.file());
        assert(move.captured_piece == PAWN);
        Square captured_sq = captured_square.relative(them);
        std::get<0>(diffs[them]).set(PAWN * 64 + captured_sq, -inc);
    } else if (move.is_capture()) {
        const PieceType captured = move.captured_piece;
        assert(captured < NO_PIECE);
        Square captured_sq = move.target.relative(them);
        std::get<0>(diffs[them]).set(captured * 64 + captured_sq, -inc);
    }

    // Castling needs the rook to be moved.
    if (move.is_castle()) {
        const CastlingSide side = move.get_castleside();
        const Square rook_from =
            RookSquares[WHITE][side]; // Rather than relitivising it, can just use the white squares.
        const Square rook_to = RookCastleSquares[WHITE][side];
        std::get<0>(diffs[us]).set(ROOK * 64 + rook_from, -inc);
        std::get<0>(diffs[us]).set(ROOK * 64 + rook_to, inc); 
    }
    return diffs;
}
void update(per_colour<FeatureVector>& features, const per_colour<FeatureDiff>& diffs) {
    for (Colour c : {WHITE, BLACK}) {
        const auto& diff = diffs[c];
        auto& feature = features[c];
        // Update piece placements
        for (const auto& [index, value] : std::get<0>(diff).data) {
            std::get<0>(feature)[index] += value;
        }
        // Update king square if needed
        Square old_king_sq = std::get<1>(diff);
        Square new_king_sq = std::get<2>(diff);
        if (old_king_sq != new_king_sq) {
            assert(old_king_sq.get_value() == std::get<1>(features[c]).get_value()); // sanity check, the old king square should match
            std::get<1>(feature) = new_king_sq;
        }
    }
}
} // namespace Neural