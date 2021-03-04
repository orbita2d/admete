#include <iostream>
#include <getopt.h> // For get_opt_long()

#include "piece.hpp"
#include "board.hpp"
#include "search.hpp"
#include "uci.hpp"

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
        board.print_board_extra();
        std::cin >> input;
        if (input == "q") {
            exit(EXIT_SUCCESS);
        } else if (input == "tb") {
            board.unmake_last();
        } else if (input == "moves") {
            print_vector(board.get_pseudolegal_moves());
        } else {
        board.try_move(input);
        }
    }
}

int main(int argc, char* argv[])
{
    static int count_moves_flag = 0;
    static int interactive_flag = 0;
    static int perft_flag = 0;
    static int evaluate_flag = 0;
    static int evaluate_negamax_flag = 0;
    static int divide_flag = 0;
    static int print_flag = 0;
    unsigned int depth = 4;
    std::string board_fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
    int opt;
	while (true) {
		static struct option long_options[] = {
			{"count-moves",	no_argument, &count_moves_flag, 1},
			{"interactive",	no_argument, &interactive_flag, 1},
            {"perft", no_argument, &perft_flag, 1},
            {"eval", no_argument, &evaluate_flag, 1},
            {"eval-negamax", no_argument, &evaluate_negamax_flag, 1},
            {"divide", no_argument, &divide_flag, 1},
			{"depth",	required_argument, 0, 'd'},
			{"board",	required_argument, 0, 'b'},
            {"print",	no_argument, &print_flag, 1},
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
		case '?':
			/* getopt_long already printed an error message. */
			break;
		default:
			exit(EXIT_FAILURE);
		}
	}
    Board board = Board();
    board.fen_decode(board_fen);

    if (print_flag) {
        board.print_board();
    }

    if (count_moves_flag) {
        std::cout << board.fen_encode() << std::endl;
        count_moves(board);
        exit(EXIT_SUCCESS);
    }

    if (interactive_flag) {
        std::cout << board.fen_encode() << std::endl;
        interactive(board);
        exit(EXIT_SUCCESS);
    }

    if (evaluate_flag) {
        std::cout << board.fen_encode() << std::endl;
        std::vector<Move> line;
        line.reserve(depth);
        int score = iterative_deepening(board, depth, line);
        std::cout << print_score(score) << std::endl;
        print_line(line, board);
        exit(EXIT_SUCCESS);
    }
    if (evaluate_negamax_flag) {
        std::cout << board.fen_encode() << std::endl;
        std::vector<Move> line;
        line.reserve(depth);
        int score = iterative_deepening_negamax(board, depth, line);
        std::cout << print_score(score) << std::endl;
        print_line(line, board);
        exit(EXIT_SUCCESS);
    }

    if (perft_flag) {
        std::cout << board.fen_encode() << std::endl;
        for (unsigned int i = 1; i <= depth; i++) {
            std::cout << perft_bulk(i, board) << std::endl;
        }
        exit(EXIT_SUCCESS);
    }

    if (divide_flag) {
        std::cout << board.fen_encode() << std::endl;
        perft_divide(depth, board);
        exit(EXIT_SUCCESS);
    }

    std::vector<Move> line;
    line.reserve(depth);
    find_best_random(board, 6, 10, line);

    // Otherwise, we are probably talking to a computer.

    std::string command;
    std::cin >> command;
    
    if (command == "uci") {
        uci();
    }

    //std::cout << perft(4, board) << std::endl;
}