#include "board.hpp"

// Move Generation

std::array<KnightMoveArray, 64> GenerateKnightMoves(){
    std::array<KnightMoveArray, 64> meta_array;
    KnightMoveArray moves;
    for (unsigned int i = 0; i < 64; i++){
        moves = KnightMoveArray();
        Square origin = Square(i);
        if (origin.to_north() >= 2){
            if(origin.to_west() >= 1) {
                moves.push_back(origin + Direction::NNW);
            }
            if(origin.to_east() >= 1) {
                moves.push_back(origin + Direction::NNE);
            }
        }
        if (origin.to_east() >= 2){
            if(origin.to_north() >= 1) {
                moves.push_back(origin + Direction::ENE);
            }
            if(origin.to_south() >= 1) {
                moves.push_back(origin + Direction::ESE);
            }
        }
        if (origin.to_south() >= 2){
            if(origin.to_east() >= 1) {
                moves.push_back(origin + Direction::SSE);
            }
            if(origin.to_west() >= 1) {
                moves.push_back(origin + Direction::SSW);
            }
        }
        if (origin.to_west() >= 2){
            if(origin.to_south() >= 1) {
                moves.push_back(origin + Direction::WSW);
            }
            if(origin.to_north() >= 1) {
                moves.push_back(origin + Direction::WNW);
            }
        }
        meta_array[i] = moves;
    }
    return meta_array;
}

std::array<KnightMoveArray, 64> knight_meta_array = GenerateKnightMoves();

KnightMoveArray knight_moves(const Square origin){
    return knight_meta_array[origin];
}

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
        if (board.is_free(target)) {
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
            if (board.pieces(target).is_colour(Colour::BLACK)) {
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
            if (board.pieces(target).is_colour(Colour::BLACK)) {
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
            if (board.is_free(target) & board.is_free(origin + Direction::N)) {
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
        if (board.is_free(target)) {
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
            if (board.pieces(target).is_colour(Colour::WHITE)) {
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
            if (board.pieces(target).is_colour(Colour::WHITE)) {
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
            if (board.is_free(target) & board.is_free(origin + Direction::S)) {
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
        if (board.pieces(target).is_piece(Pieces::Blank)) {
            // Blank Square
            moves.push_back(Move(origin, target));
            continue;
        } else if (board.pieces(target).is_colour(~colour)) {
            // Enemy piece
            move = Move(origin, target);
            move.make_capture();
            moves.push_back(move);
            return;
        } else if (board.pieces(target).is_colour(colour)) {
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
        if (board.aux_info.castle_white_queenside 
            & board.is_free(Squares::FileD | Squares::Rank1) 
            & board.is_free(Squares::FileC | Squares::Rank1)
            & board.is_free(Squares::FileB | Squares::Rank1)
            & !board.is_check(Squares::FileD | Squares::Rank1, Pieces::White)
            & !board.is_check(Squares::FileC | Squares::Rank1, Pieces::White)) {
            move = Move(Squares::FileE | Squares::Rank1, Squares::FileC | Squares::Rank1);
            move.make_queen_castle();
            moves.push_back(move);
        }
        if (board.aux_info.castle_white_kingside 
            & board.is_free(Squares::FileF | Squares::Rank1) 
            & board.is_free(Squares::FileG | Squares::Rank1)
            & !board.is_check(Squares::FileF | Squares::Rank1, Pieces::White)
            & !board.is_check(Squares::FileG | Squares::Rank1, Pieces::White)) {
            move = Move(Squares::FileE | Squares::Rank1, Squares::FileG | Squares::Rank1);
            move.make_king_castle();
            moves.push_back(move);
        }
    } else
    {
        if (board.aux_info.castle_black_queenside 
            & board.is_free(Squares::FileD | Squares::Rank8) 
            & board.is_free(Squares::FileC | Squares::Rank8)
            & board.is_free(Squares::FileB | Squares::Rank8)
            & !board.is_check(Squares::FileD | Squares::Rank8, Pieces::Black)
            & !board.is_check(Squares::FileC | Squares::Rank8, Pieces::Black)) {
            move = Move(Squares::FileE | Squares::Rank8, Squares::FileC | Squares::Rank8);
            move.make_queen_castle();
            moves.push_back(move);
        }
        if (board.aux_info.castle_black_kingside 
            & board.is_free(Squares::FileF | Squares::Rank8) 
            & board.is_free(Squares::FileG | Squares::Rank8)
            & !board.is_check(Squares::FileF | Squares::Rank8, Pieces::Black)
            & !board.is_check(Squares::FileG | Squares::Rank8, Pieces::Black)) {
            move = Move(Squares::FileE | Squares::Rank8, Squares::FileG | Squares::Rank8);
            move.make_king_castle();
            moves.push_back(move);
        }
    }
    
}

template<Colour colour>
void get_step_moves(const Board &board, const Square origin, const Square target, std::vector<Move> &moves) {
    if (board.pieces(target).is_colour(colour)) {
        // Piece on target is our colour.
        return;
    } else if (board.pieces(target).is_colour(~colour)) {
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
    for (Square::square_t sq = 0; sq < 64; sq++) {
        Piece piece = board.pieces(sq);
        if (! piece.is_colour(colour)) {continue; }
        if (piece.is_knight()) {
            get_knight_moves<colour>(board, sq, moves);
        } else if (piece.is_pawn()) {
            get_pawn_moves<colour>(board, sq, moves);
        } else if (piece.is_rook()) {
            get_rook_moves<colour>(board, sq, moves);
        } else if (piece.is_bishop()) {
            get_bishop_moves<colour>(board, sq, moves);
        } else if (piece.is_queen()) {
            get_queen_moves<colour>(board, sq, moves);
        } else if (piece.is_king()) {
            get_king_moves<colour>(board, sq, moves);
        }
    }
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

std::vector<Move> Board::get_moves(){
    Piece colour = whos_move;
    const std::vector<Move> pseudolegal_moves = get_pseudolegal_moves();
    std::vector<Move> legal_moves;
    legal_moves.reserve(256);
    Square king_square = find_king(colour);
    if (aux_info.is_check) {
        for (Move move : pseudolegal_moves) {
            if (move.origin == king_square) {
                // King moves have to be very careful.
                make_move(move);
                if (!is_check(move.target, colour)) {legal_moves.push_back(move); }
                unmake_move(move);
            } else if (number_checkers == 2) {
                // double checks require a king move, which we've just seen this is not.
                continue;
            } else if (is_pinned(move.origin)) {
                continue;
            } else if (move.target == checkers[0]) {
                // this captures the checker, it's legal unless the peice in absolutely pinned.
                legal_moves.push_back(move);
            } else if ( interposes(king_square, checkers[0], move.target)) {
                // this interposes the check it's legal unless the peice in absolutely pinned.
                legal_moves.push_back(move);
            } else if (move.is_ep_capture() & ((move.origin.rank()|move.target.file()) == checkers[0])){
                // If it's an enpassent capture, the captures piece isn't at the target.
                legal_moves.push_back(move);
            } else {
                // All other moves are illegal.
                continue;
            }
        }
        return legal_moves;
    } 
    
    // Otherwise we can be smarter.
    for (Move move : pseudolegal_moves) {
        if (move.origin == king_square) {
            // This is a king move, check where he's gone.
            make_move(move);
            if (!is_check(move.target, colour)) {legal_moves.push_back(move); }
            unmake_move(move);  
        } else if (move.is_ep_capture()) {
            // en_passent's are weird.
            if (is_pinned(move.origin)) {
                // If the pawn was pinned to the diagonal or file, the move is definitely illegal.
                continue;
            } else {
                // This can open a rank. if the king is on that rank it could be a problem.
                if (king_square.rank() == move.origin.rank()) {
                    make_move(move);
                    if (!is_check(king_square, colour)) {legal_moves.push_back(move); }
                    unmake_move(move);
                } else {
                    legal_moves.push_back(move); 
                }
            }
        } else if (is_pinned(move.origin)) {
            // This piece is absoluetly pinned, only legal moves will maintain the pin or capture the pinner.
            if (in_line(king_square, move.origin, move.target)) {legal_moves.push_back(move); }
        } else {
            // Piece isn't pinned, it can do what it wants. 
            legal_moves.push_back(move);
        }
    }

    if (colour.is_white()) {
        get_castle_moves<WHITE>(*this, legal_moves);
    }  else {
        get_castle_moves<BLACK>(*this, legal_moves);
    }
    return legal_moves;
}

std::vector<Move> Board::get_sorted_moves() {
    const std::vector<Move> legal_moves = get_moves();
    std::vector<Move> checks, promotions, captures, quiet_moves, sorted_moves;
    checks.reserve(16);
    promotions.reserve(16);
    captures.reserve(16);
    quiet_moves.reserve(16);
    for (Move move : legal_moves) {
        make_move(move);
        if (aux_info.is_check) {
            checks.push_back(move);
        } else if (move.is_queen_promotion()) {
            promotions.push_back(move);
        } else if (move.is_knight_promotion()) {
            promotions.push_back(move);
        } else if (move.is_capture()) {
            captures.push_back(move);
        } else {
            quiet_moves.push_back(move);
        }
        unmake_move(move);
    }
    sorted_moves.clear();
    sorted_moves.insert(sorted_moves.end(), checks.begin(), checks.end());
    sorted_moves.insert(sorted_moves.end(), promotions.begin(), promotions.end());
    sorted_moves.insert(sorted_moves.end(), captures.begin(), captures.end());
    sorted_moves.insert(sorted_moves.end(), quiet_moves.begin(), quiet_moves.end());
    return sorted_moves;
}