#include "tablebase.hpp"
#include "evaluate.hpp"
#include "tbprobe.h"
#include <iostream>

namespace Tablebase {
bool init(const std ::string filename) { return tb_init(filename.c_str()); }
bool probe_root(Board &board, MoveList &moves) {
    unsigned castling_rights = board.can_castle(WHITE) | board.can_castle(BLACK);
    unsigned ep_sq = 0;
    if (!(board.en_passent() == Square(0))) {
        ep_sq = board.en_passent().get_value();
    }
    unsigned results[256];
    unsigned tbresult =
        tb_probe_root(board.pieces(WHITE), board.pieces(BLACK), board.pieces(KING), board.pieces(QUEEN),
                      board.pieces(ROOK), board.pieces(BISHOP), board.pieces(KNIGHT), board.pieces(PAWN),
                      board.halfmove_clock(), castling_rights, ep_sq, board.is_white_move(), results);
    if (tbresult == TB_RESULT_FAILED) {
        return false;
    }
    const Square tb_from = Square(TB_GET_FROM(tbresult));
    const Square tb_to = Square(TB_GET_TO(tbresult));
    PieceType promotes = NO_PIECE;
    if (TB_GET_PROMOTES(tbresult) == TB_PROMOTES_KNIGHT) {
        promotes = KNIGHT;
    } else if (TB_GET_PROMOTES(tbresult) == TB_PROMOTES_BISHOP) {
        promotes = BISHOP;
    } else if (TB_GET_PROMOTES(tbresult) == TB_PROMOTES_ROOK) {
        promotes = ROOK;
    } else if (TB_GET_PROMOTES(tbresult) == TB_PROMOTES_QUEEN) {
        promotes = QUEEN;
    }
    MoveList winning_moves;
    for (Move &move : moves) {
        if (move.origin == tb_from && move.target == tb_to && get_promoted(move) == promotes) {
            unsigned wdl = TB_GET_WDL(tbresult);
            unsigned dtm = TB_GET_DTZ(tbresult);

            if (wdl == TB_WIN) {
                move.score = TBWIN - dtm;
            } else if (wdl == TB_CURSED_WIN) {
                move.score = Evaluation::drawn_score(board) + 2;
            } else if (wdl == TB_DRAW) {
                move.score = Evaluation::drawn_score(board);
            } else if (wdl == TB_BLESSED_LOSS) {
                move.score = Evaluation::drawn_score(board) - 2;
            } else if (wdl == TB_LOSS) {
                move.score = -TBWIN + dtm;
            }
            winning_moves.push_back(move);
        }
    }
    if (winning_moves.empty()) {
        return false;
    } else {
        moves = winning_moves;
        return true;
    }
}

bool probe_wdl(Board &board, score_t &result, Bounds &bounds) {

    // Don't probe unless the 50 move rule counter was just reset.
    if (board.halfmove_clock() != 0) {
        return false;
    }

    unsigned castling_rights = board.can_castle(WHITE) | board.can_castle(BLACK);

    // Don't probe if there are castling rights.
    if (castling_rights) {
        return false;
    }

    // Don't probe if there are more pieces on the board than the tablebase supports.
    if (count_bits(board.pieces()) > 6) {
        return false;
    }

    unsigned ep_sq = 0;
    if (!(board.en_passent() == Square(0))) {
        ep_sq = board.en_passent().get_value();
    }
    unsigned tbresult = tb_probe_wdl(board.pieces(WHITE), board.pieces(BLACK), board.pieces(KING), board.pieces(QUEEN),
                                     board.pieces(ROOK), board.pieces(BISHOP), board.pieces(KNIGHT), board.pieces(PAWN),
                                     board.halfmove_clock(), castling_rights, ep_sq, board.is_white_move());
    if (tbresult == TB_RESULT_FAILED) {
        return false;
    } else if (tbresult == TB_WIN) {
        result = TBWIN;
        // If the result is a TBWIN, we could still have a mate in N
        bounds = LOWER;
        return true;
    } else if (tbresult == TB_CURSED_WIN) {
        result = Evaluation::drawn_score(board) + 2;
        bounds = EXACT;
        return true;
    } else if (tbresult == TB_DRAW) {
        result = Evaluation::drawn_score(board);
        bounds = EXACT;
        return true;
    } else if (tbresult == TB_BLESSED_LOSS) {
        result = Evaluation::drawn_score(board) - 2;
        bounds = EXACT;
        return true;
    } else if (tbresult == TB_LOSS) {
        result = -TBWIN;
        // If the result is a TBLOSS, we could still have a mate in -N
        bounds = UPPER;
        return true;
    } else {
        return false;
    }
}
} // namespace Tablebase