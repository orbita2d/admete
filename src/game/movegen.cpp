#include "piece.hpp"
#include "board.hpp"

void add_pawn_promotions(const Move move, std::vector<Move> &moves) {
    // Add all variations of promotions to a move.
    Move my_move = move;
    my_move.make_knight_promotion();
    moves.push_back(my_move);
    my_move.make_bishop_promotion();
    moves.push_back(my_move);
    my_move.make_rook_promotion();
    moves.push_back(my_move);
    my_move.make_queen_promotion();
    moves.push_back(my_move);
}

template<Colour colour>
void get_pawn_moves(const Board &board, const Square origin, std::vector<Move> &moves){
    Square target;
    Move move;
    if (colour == Colour::WHITE) {
        // Moves are North
        // Normal pushes.
        target = origin + (Direction::N);
        if (board.pieces[target].is_piece(Pieces::Blank)) {
            move = Move(origin, target);
            if (origin.rank() == Squares::Rank7) {
                add_pawn_promotions(move, moves);
            } else {
                moves.push_back(move);
            }
        }
        // Normal captures.
        if (origin.to_west() != 0) {
            target = origin + (Direction::NW);
            if (board.pieces[target].is_colour(Colour::BLACK)) {
                move = Move(origin, target);
                move.make_capture();
                if (origin.rank() == Squares::Rank7) {
                    add_pawn_promotions(move, moves);
                } else {
                    moves.push_back(move);
                }
            }
        }

        if (origin.to_east() != 0) {
            target = origin + (Direction::NE);
            if (board.pieces[target].is_colour(Colour::BLACK)) {
                move = Move(origin, target);
                move.make_capture();
                if (origin.rank() == Squares::Rank7) {
                    add_pawn_promotions(move, moves);
                } else {
                    moves.push_back(move);
                }
            }
        }
        // En-passent
        if (origin.rank() == Squares::Rank5) {
            if (origin.to_west() != 0 & board.aux_info.en_passent_target == Square(origin + Direction::NW) | 
                origin.to_east() != 0 & board.aux_info.en_passent_target == Square(origin + Direction::NE) ) {
                move = Move(origin, board.aux_info.en_passent_target); 
                move.make_en_passent();
                moves.push_back(move);
            }
        }

        // Look for double pawn push possibility
        if (origin.rank() == Squares::Rank2) {
            target = origin + (Direction::N + Direction::N);
            if (board.pieces[target].is_piece(Pieces::Blank) & board.pieces[origin + Direction::N].is_piece(Pieces::Blank)) {
                move = Move(origin, target);
                move.make_double_push();
                moves.push_back(move);
            }
        }
    } else
    {
        // Moves are South
        // Normal pushes.
        target = origin + (Direction::S);
        if (board.pieces[target].is_piece(Pieces::Blank)) {
            move = Move(origin, target);
            if (origin.rank() == Squares::Rank2) {
                add_pawn_promotions(move, moves);
            } else {
                moves.push_back(move);
            }
        }
        // Normal captures.
        if (origin.to_west() != 0) {
            target = origin + (Direction::SW);
            if (board.pieces[target].is_colour(Colour::WHITE)) {
                move = Move(origin, target);
                move.make_capture();
                if (origin.rank() == Squares::Rank2) {
                    add_pawn_promotions(move, moves);
                } else {
                    moves.push_back(move);
                }
            }
        }

        if (origin.to_east() != 0) {
            target = origin + (Direction::SE);
            if (board.pieces[target].is_colour(Colour::WHITE)) {
                move = Move(origin, target);
                move.make_capture();
                if (origin.rank() == Squares::Rank2) {
                    add_pawn_promotions(move, moves);
                } else {
                    moves.push_back(move);
                }
            }
        }
        // En-passent
        if (origin.rank() == Squares::Rank4) {
            if (origin.to_west() != 0 & board.aux_info.en_passent_target == Square(origin + Direction::SW) | 
                origin.to_east() != 0 & board.aux_info.en_passent_target == Square(origin + Direction::SE) ) {
                move = Move(origin, board.aux_info.en_passent_target); 
                move.make_en_passent();
                moves.push_back(move);
            }
        }

        // Look for double pawn push possibility
        if (origin.rank() == Squares::Rank7) {
            target = origin + (Direction::S + Direction::S);
            if (board.pieces[target].is_piece(Pieces::Blank) & board.pieces[origin + Direction::S].is_piece(Pieces::Blank)) {
                move = Move(origin, target);
                move.make_double_push();
                moves.push_back(move);
            }
        }
    }
}

template<Colour colour, Direction direction>
void get_sliding_moves(const Board &board, const Square origin, const uint to_edge, std::vector<Move> &moves) {
    Square target = origin;
    Move move;
    for (uint i = 0; i < to_edge; i++) {
        target = target + direction;
        if (board.pieces[target].is_piece(Pieces::Blank)) {
            // Blank Square
            moves.push_back(Move(origin, target));
            continue;
        } else if (board.pieces[target].is_colour(~colour)) {
            // Enemy piece
            move = Move(origin, target);
            move.make_capture();
            moves.push_back(move);
            return;
        } else if (board.pieces[target].is_colour(colour)) {
            // Our piece, no more legal moves.
            return;
        }
    };
}


template<Colour colour>
void get_rook_moves(const Board &board, const Square origin, std::vector<Move> &moves) {
    get_sliding_moves<colour, Direction::N>(board, origin, origin.to_north(), moves);
    get_sliding_moves<colour, Direction::E>(board, origin, origin.to_east(), moves);
    get_sliding_moves<colour, Direction::S>(board, origin, origin.to_south(), moves);
    get_sliding_moves<colour, Direction::W>(board, origin, origin.to_west(), moves);
}


template<Colour colour>
void get_bishop_moves(const Board &board, const Square origin, std::vector<Move> &moves) {
        get_sliding_moves<colour, Direction::NE>(board, origin, origin.to_northeast(), moves);
        get_sliding_moves<colour, Direction::SE>(board, origin, origin.to_southeast(), moves);
        get_sliding_moves<colour, Direction::SW>(board, origin, origin.to_southwest(), moves);
        get_sliding_moves<colour, Direction::NW>(board, origin, origin.to_northwest(), moves);
}

template<Colour colour>
void get_queen_moves(const Board &board, const Square origin, std::vector<Move> &moves) {
    // Queen moves are the union superset of rook and bishop moves
    get_bishop_moves<colour>(board, origin, moves);
    get_rook_moves<colour>(board, origin, moves);
}


template<Colour colour>
void get_castle_moves(const Board &board, std::vector<Move> &moves) {
    Move move;
    if (colour == Colour::WHITE) {
        // You can't castle through check, or while in check
        if (board.is_check(Squares::FileE | Squares::Rank1, Pieces::White)) {return; }
        if (board.aux_info.castle_white_queenside 
            & board.is_free(Squares::FileD | Squares::Rank1) 
            & board.is_free(Squares::FileC | Squares::Rank1)
            & board.is_free(Squares::FileB | Squares::Rank1)
            & !board.is_check(Squares::FileD | Squares::Rank1, Pieces::White)) {
            move = Move(Squares::FileE | Squares::Rank1, Squares::FileC | Squares::Rank1);
            move.make_queen_castle();
            moves.push_back(move);
        }
        if (board.aux_info.castle_white_kingside 
            & board.is_free(Squares::FileF | Squares::Rank1) 
            & board.is_free(Squares::FileG | Squares::Rank1)
            & !board.is_check(Squares::FileF | Squares::Rank1, Pieces::White)
            & !board.is_check(Squares::FileE | Squares::Rank1, Pieces::White)) {
            move = Move(Squares::FileE | Squares::Rank1, Squares::FileG | Squares::Rank1);
            move.make_king_castle();
            moves.push_back(move);
        }
    } else
    {
        if (board.is_check(Squares::FileE | Squares::Rank8, Pieces::Black)) {return; }
        if (board.aux_info.castle_black_queenside 
            & board.is_free(Squares::FileD | Squares::Rank8) 
            & board.is_free(Squares::FileC | Squares::Rank8)
            & board.is_free(Squares::FileB | Squares::Rank8)
            & !board.is_check(Squares::FileD | Squares::Rank8, Pieces::Black)) {
            move = Move(Squares::FileE | Squares::Rank8, Squares::FileC | Squares::Rank8);
            move.make_queen_castle();
            moves.push_back(move);
        }
        if (board.aux_info.castle_black_kingside 
            & board.is_free(Squares::FileF | Squares::Rank8) 
            & board.is_free(Squares::FileG | Squares::Rank8)
            & !board.is_check(Squares::FileF | Squares::Rank8, Pieces::Black)) {
            move = Move(Squares::FileE | Squares::Rank8, Squares::FileG | Squares::Rank8);
            move.make_king_castle();
            moves.push_back(move);
        }
    }
    
}

template<Colour colour>
void get_step_moves(const Board &board, const Square origin, const Square target, std::vector<Move> &moves) {
    if (board.pieces[target].is_colour(colour)) {
        // Piece on target is our colour.
        return;
    } else if (board.pieces[target].is_colour(~colour)) {
        //Piece on target is their colour.
        Move move = Move(origin, target);
        move.make_capture();
        moves.push_back(move);
        return;
    } else {
        // Space is blank.
        Move move = Move(origin, target);
        moves.push_back(move);
        return;
    }
}

template<Colour colour>
void get_king_moves(const Board &board, const Square origin, std::vector<Move> &moves) {
    // We should really be careful that we aren't moving into check here.
    // Look to see if we are on an edge.
    if (origin.to_north() != 0) {
        get_step_moves<colour>(board, origin, origin + Direction::N, moves);
    }
    if (origin.to_east() != 0) {
        get_step_moves<colour>(board, origin, origin + Direction::E, moves);
    }
    if (origin.to_south() != 0) {
        get_step_moves<colour>(board, origin, origin + Direction::S, moves);
    }
    if (origin.to_west() != 0) {
        get_step_moves<colour>(board, origin, origin + Direction::W, moves);
    }
    if (origin.to_north() != 0 & origin.to_east() != 0) {
        get_step_moves<colour>(board, origin, origin + Direction::NE, moves);
    }
    if (origin.to_south() != 0 & origin.to_east() != 0) {
        get_step_moves<colour>(board, origin, origin + Direction::SE, moves);
    }
    if (origin.to_south() != 0 & origin.to_west() != 0) {
        get_step_moves<colour>(board, origin, origin + Direction::SW, moves);
    }
    if (origin.to_north() != 0 & origin.to_west() != 0) {
        get_step_moves<colour>(board, origin, origin + Direction::NW, moves);
    }
}

template<Colour colour>
void get_knight_moves(const Board &board, const Square origin, std::vector<Move> &moves) {
    for (Square target : knight_moves(origin)) {
        get_step_moves<colour>(board, origin, target, moves);
    }
};

template<Colour colour>
void generate_pseudolegal_moves(const Board &board, std::vector<Move> &moves) {
    for (Square::square_t i = 0; i < 64; i++) {
        Square square = Square(i);
        Piece piece = board.pieces[square];
        if (! piece.is_colour(colour)) {continue; }
        if (piece.is_knight()) {
            get_knight_moves<colour>(board, square, moves);
        } else if (piece.is_pawn()) {
            get_pawn_moves<colour>(board, square, moves);
        } else if (piece.is_rook()) {
            get_rook_moves<colour>(board, square, moves);
        } else if (piece.is_bishop()) {
            get_bishop_moves<colour>(board, square, moves);
        } else if (piece.is_queen()) {
            get_queen_moves<colour>(board, square, moves);
        } else if (piece.is_king()) {
            get_king_moves<colour>(board, square, moves);
        }
    }
    get_castle_moves<colour>(board, moves);
}


std::vector<Move> Board::get_pseudolegal_moves() const {    
    std::vector<Move> moves;
    moves.reserve(256);
    if (whos_move == white_move) {
        generate_pseudolegal_moves<Colour::WHITE>(*this, moves);
    } else {
        generate_pseudolegal_moves<Colour::BLACK>(*this, moves);
    }
    return moves;
}
