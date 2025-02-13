#include <bitboard.hpp>
#include <board.hpp>
#include <network.hpp>

namespace Neural {
per_colour<FeatureVector> cleared_features() {
    per_colour<FeatureVector> features;
    for (Colour c : {WHITE, BLACK}) {
        features[c] = Vector<int16_t, N_FEATURES>::zeros();
    }
    return features;
}

per_colour<FeatureVector> encode(const Board &board) {
    // zero out the feature vectors
    per_colour<FeatureVector> features = cleared_features();

    for (Colour c : {WHITE, BLACK}) {
        for (PieceType p = PAWN; p < N_PIECE; p++) {
            Bitboard bb = board.pieces(c, p);
            while (bb) {
                Square sq = pop_lsb(&bb);
                features[c][p * 64 + sq.relative(c)] = 1;
            }
        }
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
    diffs[us].set(p * 64 + origin, -inc);

    if (move.is_promotion()) {
        const PieceType promoted = get_promoted(move);
        assert(promoted < NO_PIECE);
        diffs[us].set(promoted * 64 + target, inc);
    } else {
        diffs[us].set(p * 64 + target, inc);
    }

    if (move.is_ep_capture()) {
        const Square captured_square(move.origin.rank(), move.target.file());
        assert(move.captured_piece == PAWN);
        Square captured_sq = captured_square.relative(them);
        diffs[them].set(PAWN * 64 + captured_sq, -inc);
    } else if (move.is_capture()) {
        const PieceType captured = move.captured_piece;
        assert(captured < NO_PIECE);
        Square captured_sq = move.target.relative(them);
        diffs[them].set(captured * 64 + captured_sq, -inc);
    }

    // Castling needs the rook to be moved.
    if (move.is_castle()) {
        const CastlingSide side = move.get_castleside();
        const Square rook_from =
            RookSquares[WHITE][side]; // Rather than relitivising it, can just use the white squares.
        const Square rook_to = RookCastleSquares[WHITE][side];
        diffs[us].set(ROOK * 64 + rook_from, -inc);
        diffs[us].set(ROOK * 64 + rook_to, inc);
    }
    return diffs;
}
} // namespace Neural