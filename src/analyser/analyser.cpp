#include <iostream>
#include <getopt.h> // For get_opt_long()
#include <string>
#include <fstream>
#include <istream>
#include <sstream>
#include <thread>

#include "board.hpp"
#include "search.hpp"
#include "uci.hpp"
#include "evaluate.hpp"
#include "movegen.hpp"
#include "bitboard.hpp"
#include "transposition.hpp"

constexpr size_t max_block = 1<<12;
size_t number_threads = 1;

void analyse_block(std::vector<std::string> *fen_list, std::vector<int> *results, size_t start, size_t stride, size_t end) {
	Board board = Board();

	for (size_t i = start; i < end; i += stride) {
		std::string fen = fen_list->at(i);
		board.fen_decode(fen);
		PrincipleLine pl;
		int score = iterative_deepening(board, 2, pl);
		score = board.is_white_move() ? score : -score;
		results->at(i) = score;
	}
}

unsigned long process_block(std::vector<std::string> fen_list, std::vector<int> reference_list) {
	size_t block_size = fen_list.size();
	std::vector<int> results(block_size);
	std::vector<std::thread> threads;

	// Multithread in the standard striding way.
	for (size_t i = 0; i < ::number_threads; i++) {
		threads.push_back(std::thread(&analyse_block, &fen_list, &results, i, number_threads, block_size));
	}

	// Join the threads
	for (size_t i = 0; i < ::number_threads; i++) {
		threads.at(i).join();
	}

	unsigned long totals = 0;
	for (size_t i = 0; i < block_size; i++) {
		int diff = std::abs(reference_list[i] - results[i]);
		totals += std::min(800, diff);
	}
	return totals;
}

void summary(std::string filename) {
    size_t array_length = 0;
    std::vector<std::string> fen_list;
    std::vector<int> reference_list;
	std::fstream file;
	file.open(filename, std::ios::in);
	if (!file) {
		std::cerr << "No such file: " << filename << std::endl;
		exit(EXIT_FAILURE);
	}
    std::string line;
	unsigned long totals = 0;
	size_t counter = 0;

    while (getline(file, line)) {
    	std::string fen;
		int reference;
		size_t pos = line.find(":");
		if (pos == std::string::npos) {
			exit(EXIT_FAILURE);
		}
        fen = line.substr(0, pos);
        line.erase(0, pos + 1);
		std::stringstream ss(line);
		ss >> reference;
		fen_list.push_back(fen);
		reference_list.push_back(reference);
		array_length++;
		if (array_length >= max_block) {
			totals += process_block(fen_list, reference_list);
			counter += array_length;
			fen_list.clear();
			reference_list.clear();
			array_length = 0;
		}
    }
	// Finish up the last bits that didn't fill the array
	totals += process_block(fen_list, reference_list);
	counter += array_length;

	std::cout << "Summary: " << totals << " / " << counter << std::endl;
}

int main(int argc, char* argv[])
{
    unsigned int depth = 4;
    std::string tuning_table = "";
    std::string input_file = "";
    static int tuning_table_flag = 0;
    static int summary_flag = 0;
    int opt;
	while (true) {
		static struct option long_options[] = {
            {"summary", no_argument, &summary_flag, 1},
            {"load-tables", required_argument, 0, 'T'},
            {"input", required_argument, 0, 'i'},
			{0, 0, 0, 0}
		};
		int option_index = 0;
		opt = getopt_long(argc, argv, "b:d:i:T:j:", long_options, &option_index);
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
		case 'T':
            tuning_table_flag = 1;
            tuning_table = optarg;
			break;
		case 'i':
            input_file = optarg;
			break;
		case 'j':
            ::number_threads = atoi(optarg);
			break;
		case '?':
			/* getopt_long already printed an error message. */
			exit(EXIT_FAILURE);
			break;
		default:
			exit(EXIT_FAILURE);
		}
	}
    if (input_file == "") {
		std::cerr << "Include input file" << std::endl;
        exit(EXIT_FAILURE);
    }
    Bitboards::init();
    Zorbist::init();
    Evaluation::init();
	Cache::init();
	Cache::transposition_table.disable();
	Cache::killer_table.disable();

    if (tuning_table_flag) {
        Evaluation::load_tables(tuning_table);
    }

    summary(input_file);
}