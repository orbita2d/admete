#include "bitboard.hpp"
#include "board.hpp"
#include "cache.hpp"
#include "evaluate.hpp"
#include "movegen.hpp"
#include "printing.hpp"
#include "search.hpp"
#include "zobrist.hpp"
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <random>
#include <sstream>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

typedef std::chrono::steady_clock my_clock;
typedef std::pair<DenseBoard, score_t> Position;
typedef std::vector<Position> Dataset;
typedef std::vector<double> result_block;
typedef std::vector<score_t *> ParameterArray;
template <typename T> T SquareNumber(T n) { return n * n; }

template <typename T> int sgn(T val) { return (T(0) < val) - (val < T(0)); }

constexpr double win_probability(const score_t eval) {
    constexpr double beta = 500;
    const double eta = std::exp(eval / beta);
    return eta / (eta + 1 / eta);
}

void error_on_dataset_kernel(Dataset *dataset, const size_t start, const size_t stride, std::vector<double> *results) {
    Board board;
    Search::SearchOptions options;
    for (size_t i = start; i < dataset->size(); i += stride) {
        board.unpack(dataset->at(i).first);
        score_t score = Search::quiesce(board, NEG_INF, POS_INF, options);
        score *= board.is_white_move() ? 1 : -1;
        score_t truth = dataset->at(i).second;
        results->at(i) = SquareNumber(win_probability(score) - win_probability(truth));
    }
}

double error_on_dataset(Dataset &dataset) {
    constexpr size_t n_thread = 14;
    std::vector<std::thread> threads;
    const size_t n = dataset.size();
    std::vector<double> results(n, 0.);

    for (size_t i = 0; i < n_thread; i++) {
        threads.push_back(std::thread(&error_on_dataset_kernel, &dataset, i, n_thread, &results));
    }

    for (size_t i = 0; i < n_thread; i++) {
        threads.at(i).join();
    }

    double error = 0;
    for (size_t i = 0; i < n; i++) {
        error += results[i];
    }
    error /= n;

    return error;
}

void fill_dataset(std::vector<std::string> &files, Dataset &dataset) {
    for (std::string path : files) {
        std::fstream file;
        file.open(path, std::ios::in);
        std::string line;
        std::cout << path << std::endl;
        while (std::getline(file, line)) {
            std::string token;
            std::istringstream is(line);
            score_t eval;
            std::string fen;
            std::getline(is, fen, ':');
            is >> eval;
            Board board;
            board.fen_decode(fen);
            Position pair;
            pair.first = board.pack();
            pair.second = eval;
            dataset.push_back(pair);
        }
    }
}

void fill_parameters(ParameterArray &parameters) {
    parameters.clear();

    for (int p = KNIGHT; p < KING; p++) {
        parameters.push_back(&Evaluation::mobility[p].opening_score);
        parameters.push_back(&Evaluation::mobility[p].endgame_score);
    }

    for (PieceType p = PAWN; p < N_PIECE; p++) {
        for (int sq = 0; sq < N_SQUARE; sq++) {
            parameters.push_back(&Evaluation::piece_square_tables[p][sq].opening_score);
            parameters.push_back(&Evaluation::piece_square_tables[p][sq].endgame_score);
        }
    }
    for (int sq = 0; sq < N_SQUARE; sq++) {
        parameters.push_back(&Evaluation::pb_passed[sq].opening_score);
        parameters.push_back(&Evaluation::pb_passed[sq].endgame_score);
    }
}

void train_iteration(Dataset &dataset, const ParameterArray &parameters, const size_t idx) {
    // Pick a random parameter to tune
    const size_t n_parameters = parameters.size();

    score_t *working = parameters[idx % n_parameters];
    const score_t starting_value = *working;

    // Check the local conditions projected onto this parameter.
    constexpr score_t step_value = 2;
    const double initial = error_on_dataset(dataset);
    (*working) = starting_value - step_value;
    const double p_less = error_on_dataset(dataset);
    (*working) = starting_value + step_value;
    const double p_more = error_on_dataset(dataset);
    (*working) = starting_value;

    score_t step;
    double p_next;
    double p_now = initial;
    const double dp = p_more - p_less;
    if (std::abs(dp) < 1E-8) {
        std::cout << "flat" << std::endl;
        return;
    }
    // Characterise local conditions.
    if ((initial > p_less) & (initial > p_more)) {
        // Maximum
        std::cout << "max" << std::endl;
        return;
    } else if ((initial < p_less) & (initial < p_more)) {
        // Minimum
        std::cout << "min" << std::endl;
        return;
    } else if (dp > 0) {
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
    while (p_next < p_now) {
        p_now = p_next;
        (*working) += step;
        p_next = error_on_dataset(dataset);
    }
    // Undo the last step
    (*working) -= step;
    std::cout << (*working) << " - ";
    std::cout << std::fixed << std::setprecision(5) << std::sqrt(p_now) << std::endl;
}

int main(int argc, char *argv[]) {
    Bitboards::init();
    Evaluation::init();
    Zobrist::init();
    GameCache::init();
    GameCache::pawn_cache.disable();
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
    std::vector<std::string> files;
    files.reserve(argc - 2);
    for (int i = 2; i < argc; i++) {
        files.push_back(static_cast<std::string>(argv[i]));
    }
    Dataset dataset;
    std::cout << "Filling . . ." << std::endl;
    fill_dataset(files, dataset);
    std::cout << "Done" << std::endl;
    my_clock::time_point origin_time = my_clock::now();
    const double start_error = error_on_dataset(dataset);
    const int iteration_time =
        std::chrono::duration_cast<std::chrono::milliseconds>(my_clock::now() - origin_time).count();
    std::cout << "dataset        : " << std::fixed << std::setprecision(5) << std::sqrt(start_error) << " ";
    std::cout << "( in " << std::setprecision(2) << iteration_time / 1000. << "s )" << std::endl;

    ParameterArray parameters;
    fill_parameters(parameters);
    size_t i = 0;
    while (true) {
        train_iteration(dataset, parameters, i);
        Evaluation::save_tables(tables_path);
        i++;
    }
}