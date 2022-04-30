#include "bitboard.hpp"
#include "board.hpp"
#include "cache.hpp"
#include "evaluate.hpp"
#include "movegen.hpp"
#include "printing.hpp"
#include "zobrist.hpp"
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>

namespace fs = std::filesystem;

typedef std::pair<std::string, score_t> evalposition;
constexpr size_t blocksize = 1024;
typedef std::array<evalposition, blocksize> evalblock;
template <typename T> T SquareNumber(T n) { return n * n; }

template <typename T> int sgn(T val) { return (T(0) < val) - (val < T(0)); }

constexpr double win_probability(const score_t eval) {
  constexpr double beta = 500;
  const double eta = std::exp(eval / beta);
  return eta / (eta + 1 / eta);
}

double error_on_block(evalblock &block) {
  // Initialise a board model.
  Board board = Board();
  double error = 0;
  for (size_t i = 0; i < blocksize; i++) {
    board.fen_decode(block[i].first);
    Score eval = board.get_psqt();
    score_t score = eval.interpolate(board.phase_material());
    score_t truth = block[i].second;
    error += SquareNumber(win_probability(score) - win_probability(truth));
  }
  return error / blocksize;
}

void train_on_block(evalblock &block) {
  // Initialise a board model.
  Board board = Board();
  constexpr double step = 1E-4;
  std::random_device r;
  std::default_random_engine generator(r());
  std::uniform_real_distribution<double> distribution(0.0, 1.0);

  for (size_t i = 0; i < blocksize; i++) {
    board.fen_decode(block[i].first);
    Score eval = board.get_psqt();
    score_t score = eval.interpolate(board.phase_material());
    score_t truth = block[i].second;
    const double error = win_probability(score) - win_probability(truth);
    // error > 0 means white is less likely to win than we think
    double phase_ratio = 0;
    if (board.phase_material() > ENDGAME_MATERIAL) {
      phase_ratio = 1;
    } else if (board.phase_material() > OPENING_MATERIAL) {
      phase_ratio = (board.phase_material() - OPENING_MATERIAL) /
                    (ENDGAME_MATERIAL - OPENING_MATERIAL);
    }
    // Iteration probability in monte-carlo
    const double probability = step * SquareNumber(error);
    const score_t iterant = sgn(error);
    for (PieceType p = KNIGHT; p < KING; p++) {
      // We want to keep the bitboards symmetrical.
      Bitboard occ = board.pieces(WHITE, p);
      while (occ) {
        Square sq = pop_lsb(&occ);
        if (distribution(generator) < (1 - phase_ratio) * probability) {
          Evaluation::piece_square_tables[OPENING][WHITE][p][sq] -= iterant;
          Evaluation::piece_square_tables[OPENING][BLACK][p][56 ^ (int)sq] -=
              iterant;
        }
        if (distribution(generator) < probability) {
          Evaluation::piece_square_tables[ENDGAME][WHITE][p][sq] -= iterant;
          Evaluation::piece_square_tables[ENDGAME][BLACK][p][56 ^ (int)sq] -=
              iterant;
        }
      }

      occ = board.pieces(BLACK, p);
      while (occ) {
        Square sq = pop_lsb(&occ);
        if (distribution(generator) < (1 - phase_ratio) * probability) {
          Evaluation::piece_square_tables[OPENING][WHITE][p][56 ^ (int)sq] +=
              iterant;
          Evaluation::piece_square_tables[OPENING][BLACK][p][sq] += iterant;
        }
        if (distribution(generator) < probability) {
          Evaluation::piece_square_tables[ENDGAME][WHITE][p][56 ^ (int)sq] +=
              iterant;
          Evaluation::piece_square_tables[ENDGAME][BLACK][p][sq] += iterant;
        }
      }
    }

    const score_t after =
        Evaluation::psqt(board).interpolate(board.phase_material());
    const double error_before = SquareNumber(error);
    const double error_after =
        SquareNumber(win_probability(after) - win_probability(truth));
    if (after != score && error_after > error_before) {
      std::cout << "truth: " << truth << std::endl;
      std::cout << "before: " << score << " : " << error_before << std::endl;
      std::cout << "after: " << after << " : " << error_after << std::endl;
      std::cout << std::endl;
    }
  }
}

double error_on_file(std::string path) {
  std::fstream file;
  file.open(path, std::ios::in);
  evalblock block;
  std::string line;
  size_t i = 0;
  size_t blocks = 0;
  double error = 0;
  while (std::getline(file, line)) {
    std::string token;
    std::istringstream is(line);
    score_t eval;
    std::string fen;
    std::getline(is, fen, ':');
    is >> eval;
    block[i].first = fen;
    block[i].second = eval;
    i++;
    if (i >= blocksize) {
      i = 0;
      blocks++;
      error += error_on_block(block);
    }
  }
  error /= blocks;
  std::cout << "error: " << error << std::endl;
  return error;
}

void read_file(std::string path) {
  std::fstream file;
  file.open(path, std::ios::in);
  evalblock block;
  std::string line;
  size_t i = 0;
  while (std::getline(file, line)) {
    std::string token;
    std::istringstream is(line);
    score_t eval;
    std::string fen;
    std::getline(is, fen, ':');
    is >> eval;
    // std::cout << fen << " - " << eval << std::endl;
    block[i].first = fen;
    block[i].second = eval;
    i++;
    if (i >= blocksize) {
      i = 0;
      // do-something
      train_on_block(block);
    }
  }
}

int main(int argc, char *argv[]) {
  Bitboards::init();
  Evaluation::init();
  Zobrist::init();
  if (argc < 3) {
    std::cerr << "Need input file" << std::endl;
    return 1;
  }
  const std::string tables_path = "tables.txt";
  fs::path f{tables_path};
  if (fs::exists(f)) {
    Evaluation::load_tables(tables_path);
  }

  const std::string error_file = static_cast<std::string>(argv[1]);
  const std::string input_file = static_cast<std::string>(argv[2]);
  std::cout << "Using as evaluation data: " << error_file << std::endl;

  error_on_file(error_file);
  for (int i = 2; i < argc; i++) {
    read_file(static_cast<std::string>(argv[i]));
    if (i % 200 == 0) {
      error_on_file(error_file);
    }
  }
  Evaluation::save_tables(tables_path);
  error_on_file(error_file);
}