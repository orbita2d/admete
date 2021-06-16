#include <iostream>
#include <chrono>
#include <getopt.h> // For get_opt_long()

#include "piece.hpp"
#include "board.hpp"
#include "search.hpp"
#include "uci.hpp"
#include "evaluate.hpp"
#include "movegen.hpp"
#include "bitboard.hpp"
#include "transposition.hpp"

void print_vector(const std::vector<Square> &moves) {
    for (Square move : moves) {
        std::cout << move << std::endl;
    }
}

void print_vector(const std::vector<Move> &moves) {
    for (Move move : moves) {
        std::cout << move << std::endl;
    }
}

void print_line(PrincipleLine &line, Board& board) {
    for (PrincipleLine::reverse_iterator it = line.rbegin(); it != line.rend(); ++it) {
        std::vector<Move> legal_moves = board.get_moves();
        std::cout << board.print_move(*it, legal_moves) << std::endl;
        board.make_move(*it);
    }
    for (Move move : line) {
        board.unmake_move(move);
    }
}

int main(int argc, char* argv[])
{
    static int perft_flag = 0;
    static int perftcomp_flag = 0;
    static int evaluate_flag = 0;
    static int evaluate_static_flag = 0;
    static int divide_flag = 0;
    static int print_flag = 0;
    static int print_tables_flag = 0;
    unsigned int depth = 4;
    std::string board_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    std::string tuning_table = "";
    static int tuning_table_flag = 0;
    int opt;
	while (true) {
		static struct option long_options[] = {
            {"perft", no_argument, &perft_flag, 1},
            {"perft-compare", no_argument, &perftcomp_flag, 1},
            {"eval", no_argument, &evaluate_flag, 1},
			{"eval-static",	no_argument, &evaluate_static_flag, 1},
            {"divide", no_argument, &divide_flag, 1},
			{"depth",	required_argument, 0, 'd'},
			{"board",	required_argument, 0, 'b'},
            {"print",	no_argument, &print_flag, 1},
            {"print-tables", no_argument, &print_tables_flag, 1},
            {"load-tables", required_argument, 0, 'T'},
			{0, 0, 0, 0}
		};
		int option_index = 0;
		opt = getopt_long(argc, argv, "b:d:", long_options, &option_index);
		if (opt == -1) { break; }
		switch (opt) {
		case 0:
			/* If this option set a flag, do nothing else now. */
			if (long_options[option_index].flag != 0)
				break;
			fprintf(stdout, "option %s", long_options[option_index].name);
			if (optarg)
				fprintf(stdout," with arg %s", optarg);
			fprintf(stdout, "\n");
			break;
		case 'd':
            depth = atoi(optarg);
			break;
		case 'b':
            board_fen = optarg;
			break;
		case 'T':
            tuning_table_flag = 1;
            tuning_table = optarg;
			break;
		case '?':
			/* getopt_long already printed an error message. */
			exit(EXIT_FAILURE);
			break;
		default:
			exit(EXIT_FAILURE);
		}
	}
    Bitboards::init();
    Zorbist::init();
    Evaluation::init();

    if (tuning_table_flag) {
        Evaluation::load_tables(tuning_table);
    }
    
    Board board = Board();
    board.fen_decode(board_fen);
    if (print_flag) {
        board.pretty();
    }

    if (print_tables_flag) {
        print_tables();
        exit(EXIT_SUCCESS);
    }

    if (evaluate_flag) {
        std::cout << board.fen_encode() << std::endl;
        std::vector<Move> line;
        line.reserve(depth);
        long nodes;
        int score = iterative_deepening(board, depth, POS_INF, line, nodes);
        std::cout << print_score(score) << " ( " << nodes << " ) " << std::endl;
        print_line(line, board);
        exit(EXIT_SUCCESS);
    }

    if (evaluate_static_flag) {
        int score = negamax_heuristic(board);
        std::cout << print_score(score) << std::endl;
        exit(EXIT_SUCCESS);
    }

    if (perft_flag) {
        std::cout << board.fen_encode() << std::endl;
        for (unsigned int i = 1; i <= depth; i++) {
            std::cout << std::dec << perft(i, board) << std::endl;
        }
        exit(EXIT_SUCCESS);
    }

    if (perftcomp_flag) {
        std::cout << board.fen_encode() << std::endl;
        std::chrono::high_resolution_clock::time_point time_origin, time_now;
        time_origin = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> time_span;
        long nodes = perft_comparison(depth, board);
        time_now = std::chrono::high_resolution_clock::now();
        time_span = time_now - time_origin;
        std::cout << "nodes: " << nodes << " in " << time_span.count() /1000 << "seconds" << std::endl;
        std::cout <<  int(1000*(nodes / time_span.count())) << " nodes per second" << std::endl;

        exit(EXIT_SUCCESS);
    }

    if (divide_flag) {
        std::cout << board.fen_encode() << std::endl;
        perft_divide(depth, board);
        exit(EXIT_SUCCESS);
    }
    
    // Otherwise, we are probably talking to a computer.

    std::string command;
    std::cin >> command;
    
    if (command == "uci") {
        uci();
    } else if (command == "test") {
        // Testing section
        board.pretty();

        std::cout << "White Passed Pawns:" << std::endl;
        Bitboards::pretty(board.passed_pawns(WHITE)); 

        std::cout << "Black Passed Pawns:" << std::endl;
        Bitboards::pretty(board.passed_pawns(BLACK)); 

        std::cout << "White Weak Pawns:" << std::endl;
        Bitboards::pretty(board.weak_pawns(WHITE)); 

        std::cout << "Black Weak Pawns:" << std::endl;
        Bitboards::pretty(board.weak_pawns(BLACK)); 

        std::cout << "White Half-Open Files:" << std::endl;
        Bitboards::pretty(board.half_open_files(WHITE)); 

        std::cout << "Black Half-Open Files:" << std::endl;
        Bitboards::pretty(board.half_open_files(BLACK)); 

        std::cout << "Open Files:" << std::endl;
        Bitboards::pretty(board.open_files()); 

        std::cout << "White Isolated Pawns:" << std::endl;
        Bitboards::pretty(board.isolated_pawns(WHITE)); 

        std::cout << "Black Isolated Pawns:" << std::endl;
        Bitboards::pretty(board.isolated_pawns(BLACK)); 

        std::cout << "White Connected Passed Pawns:" << std::endl;
        Bitboards::pretty(board.connected_passed_pawns(WHITE)); 

        std::cout << "Black Connected Passed Pawns:" << std::endl;
        Bitboards::pretty(board.connected_passed_pawns(BLACK)); 

        std::cout << "King Saftey:" << std::endl;
        Bitboard occ = Bitboards::attacks<QUEEN>(board.pieces(PAWN), board.find_king(WHITE));
        occ &= ~board.pieces(PAWN);
        occ &= ~Bitboards::pawn_attacks(WHITE, board.pieces(WHITE, PAWN));
        Bitboards::pretty(occ);

        std::cout << "White weak squares:" << std::endl;
        Bitboards::pretty(board.weak_squares(WHITE));
        std::cout << "Black weak squares:" << std::endl;
        Bitboards::pretty(board.weak_squares(BLACK));

        std::cout << "White Outposts:" << std::endl;
        Bitboards::pretty(board.outposts(WHITE));
        std::cout << "Black Outposts:" << std::endl;
        Bitboards::pretty(board.outposts(BLACK));

    }
}