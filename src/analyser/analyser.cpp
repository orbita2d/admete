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

void count_moves(Board &board) {
    std::vector<Move> moves = board.get_moves();
    std::cout << moves.size() << std::endl;
}

void interactive(Board &board) {
    std::string input;
    while (true) {
        std::cout << std::endl;
        board.pretty();
        std::cin >> input;
        if (input == "q") {
            exit(EXIT_SUCCESS);
        } else if (input == "moves") {
            print_vector(board.get_moves());
        } else {
        board.try_move(input);
        }
    }
}

int main(int argc, char* argv[])
{
    static int count_moves_flag = 0;
    static int count_captures_flag = 0;
    static int interactive_flag = 0;
    static int perft_flag = 0;
    static int perftcomp_flag = 0;
    static int evaluate_flag = 0;
    static int evaluate_random_flag = 0;
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
			{"count-moves",	no_argument, &count_moves_flag, 1},
			{"capture-moves",	no_argument, &count_captures_flag, 1},
			{"interactive",	no_argument, &interactive_flag, 1},
            {"perft", no_argument, &perft_flag, 1},
            {"perft-compare", no_argument, &perftcomp_flag, 1},
            {"eval", no_argument, &evaluate_flag, 1},
            {"eval-random", no_argument, &evaluate_random_flag, 1},
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
    std::cout << "Hello World" << std::endl;
}