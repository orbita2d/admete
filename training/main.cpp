#include "bitboard.hpp"
#include "board.hpp"
#include "cache.hpp"
#include "evaluate.hpp"
#include "movegen.hpp"
#include "printing.hpp"
#include "zobrist.hpp"
#include <iostream>

int main(int argc, char *argv[]) {
  Bitboards::init();
  Evaluation::init();
  Zobrist::init();
  if (argc < 2) {
    std::cerr << "Need input file" << std::endl;
    return 1;
  }
  const std::string input_file = static_cast<std::string>(argv[1]);
  std::cout << "tests: " << input_file << std::endl;
}