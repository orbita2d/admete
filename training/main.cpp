#include "bitboard.hpp"
#include "board.hpp"
#include "cache.hpp"
#include "evaluate.hpp"
#include "movegen.hpp"
#include "printing.hpp"
#include "zobrist.hpp"
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <random>
#include <sstream>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

typedef std::chrono::steady_clock my_clock;
typedef std::pair<std::string, score_t> evalposition;
constexpr size_t blocksize = 4096;
typedef std::array<evalposition, blocksize> evalblock;
typedef std::array<double, blocksize> result_block;
template <typename T> T SquareNumber(T n) { return n * n; }

template <typename T> int sgn(T val) { return (T(0) < val) - (val < T(0)); }

constexpr double win_probability(const score_t eval) {
  constexpr double beta = 500;
  const double eta = std::exp(eval / beta);
  return eta / (eta + 1 / eta);
}

void error_on_block_kernel(evalblock *block, const size_t start,
                           const size_t stride, result_block *results) {
  // Initialise a board model.
  Board board = Board();
  for (size_t i = start; i < blocksize; i += stride) {
    board.fen_decode(block->at(i).first);
    score_t score = Evaluation::evaluate_white(board);
    score_t truth = block->at(i).second;
    results->at(i) =
        SquareNumber(win_probability(score) - win_probability(truth));
  }
}

double error_on_block(evalblock &block) {
  constexpr size_t n_thread = 4;
  std::vector<std::thread> threads;
  result_block results;

  for (size_t i = 0; i < n_thread; i++) {
    threads.push_back(
        std::thread(&error_on_block_kernel, &block, i, n_thread, &results));
  }

  for (size_t i = 0; i < n_thread; i++) {
    threads.at(i).join();
  }

  double error = 0;
  for (size_t i = 0; i < blocksize; i++) {
    error += results[i];
  }
  error /= blocksize;

  return error;
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

  /**
    std::fstream out;
    out.open("error.txt", std::ios_base::app);
    out << error << std::endl;
    */
  return error;
}

void error_on_dataset_kernel(std::vector<std::string> *files,
                             const size_t start, const size_t stride,
                             std::vector<double> *results) {
  for (size_t i = start; i < files->size(); i += stride) {
    results->at(i) = error_on_file(files->at(i));
  }
}

double error_on_dataset(std::vector<std::string> files) {
  constexpr size_t n_thread = 8;
  std::vector<std::thread> threads;
  const size_t n_files = files.size();
  std::vector<double> results(n_files, 0.);

  for (size_t i = 0; i < n_thread; i++) {
    threads.push_back(
        std::thread(&error_on_dataset_kernel, &files, i, n_thread, &results));
  }

  for (size_t i = 0; i < n_thread; i++) {
    threads.at(i).join();
  }

  double error = 0;
  for (size_t i = 0; i < n_files; i++) {
    error += results[i];
  }
  error /= n_files;

  return error;
}

void train_iteration(std::vector<std::string> dataset) {
  // Pick a random parameter to tune
  std::random_device r;
  std::default_random_engine generator(r());
  std::uniform_int_distribution piece_dist((unsigned)PAWN, (unsigned)KING);
  std::uniform_int_distribution colour_dist((unsigned)WHITE, (unsigned)BLACK);
  std::uniform_int_distribution gp_dist((unsigned)OPENING, (unsigned)ENDGAME);
  std::uniform_int_distribution square_dist(0, 63);

  PieceType p = (PieceType)piece_dist(generator);
  Square sq = square_dist(generator);
  GamePhase gp = (GamePhase)gp_dist(generator);
  std::cout << Printing::piece_name(p) << " ";
  std::cout << sq << " ";
  std::cout << gp << std::endl;

  score_t *working = &Evaluation::piece_square_tables[gp][WHITE][p][sq];
  score_t *pair = &Evaluation::piece_square_tables[gp][BLACK][p][(int)sq ^ 56];
  const score_t starting_value = *working;

  // Check the local conditions projected onto this parameter.
  constexpr score_t step_value = 2;
  const double initial = error_on_dataset(dataset);
  (*working) = starting_value - step_value;
  (*pair) = (*working);
  const double p_less = error_on_dataset(dataset);
  (*working) = starting_value + step_value;
  (*pair) = (*working);
  const double p_more = error_on_dataset(dataset);
  (*working) = starting_value;
  (*pair) = (*working);

  score_t step;
  double p_next;
  double p_now = initial;
  // Characterise local conditions.
  if ((initial > p_less) & (initial > p_more)) {
    // Maximum
    std::cout << "max" << std::endl;
    return;
  } else if ((initial < p_less) & (initial < p_more)) {
    // Minimum
    std::cout << "min" << std::endl;
    return;
  } else if (p_less < p_more) {
    // Positive gradient
    step = -step_value;
    p_next = p_less;
  } else {
    // Negative gradient
    step = +step_value;
    p_next = p_more;
  }
  // Loop through while the error is still decreasing (local min search)
  (*working) += step;
  (*pair) = (*working);
  while (p_next < p_now) {
    p_now = p_next;
    (*working) += step;
    (*pair) = (*working);
    p_next = error_on_dataset(dataset);
  }
  // Undo the last step
  (*working) -= step;
  (*pair) = (*working);
  std::cout << "Final: " << (*working) << " - " << std::sqrt(p_now)
            << std::endl;
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
  std::vector<std::string> dataset;
  dataset.reserve(argc - 2);
  for (int i = 2; i < argc; i++) {
    dataset.push_back(static_cast<std::string>(argv[i]));
  }
  std::cout << "evaluation file: " << std::sqrt(error_on_file(error_file))
            << std::endl;
  my_clock::time_point origin_time = my_clock::now();
  const double start_error = error_on_dataset(dataset);
  const int iteration_time = std::chrono::duration_cast<std::chrono::seconds>(
                                 my_clock::now() - origin_time)
                                 .count();
  std::cout << "dataset        : " << std::sqrt(start_error) << " ";
  std::cout << "( in " << iteration_time << "s )" << std::endl;

  while (true) {
    train_iteration(dataset);
    Evaluation::save_tables(tables_path);
    std::cout << std::sqrt(error_on_file(error_file)) << std::endl;
    std::fstream out;
    out.open("error.txt", std::ios_base::app);
    out << std::sqrt(error_on_file(error_file)) << std::endl;
  }
}