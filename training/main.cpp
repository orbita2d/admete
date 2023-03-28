#include "bitboard.hpp"
#include "board.hpp"
#include "cache.hpp"
#include "evaluate.hpp"
#include "movegen.hpp"
#include "ordering.hpp"
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
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

typedef std::chrono::steady_clock my_clock;
typedef std::pair<DenseBoard, score_t> Position;
typedef std::vector<Position> Dataset;
typedef std::vector<double> result_block;
typedef std::pair<score_t *, std::string> Parameter;
typedef std::vector<Parameter> ParameterArray;
template <typename T> T SquareNumber(T n) { return n * n; }

template <typename T> int sgn(T val) { return (T(0) < val) - (val < T(0)); }

double win_probability(const score_t eval) {
    constexpr double beta = 500;
    const double eta = std::exp(eval / beta);
    return eta / (eta + 1 / eta);
}

void error_on_dataset_kernel(Dataset *dataset, const size_t start, const size_t stride, std::vector<double> *results) {
    Board board;
    Search::SearchOptions options;

    for (size_t i = start; i < dataset->size(); i += stride) {
        board.unpack(dataset->at(i).first);
        score_t score = Evaluation::evaluate_white(board);
        // score *= board.is_white_move() ? 1 : -1;
        score_t truth = dataset->at(i).second;
        results->at(i) = SquareNumber(win_probability(score) - win_probability(truth));
    }
}

double error_on_dataset(Dataset &dataset) {
    constexpr size_t n_thread = 14;
    std::vector<std::thread> threads;
    const size_t n = dataset.size();
    std::vector<double> results(n, -1.);

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

Position quiesce(Board &board, const score_t alpha_start, const score_t beta) {
    // perform quiesence search to evaluate only quiet positions.
    score_t alpha = alpha_start;
    Position qp;
    qp.first = board.pack();
    MoveList moves;

    // Look for checkmate
    if (board.is_check()) {
        // Generates all evasions.
        moves = board.get_moves();
        if (moves.empty()) {
            qp.second = Evaluation::terminal(board);
            return qp;
        }
    }

    // If this is a draw by repetition or insufficient material, return the drawn score.
    if (board.is_draw()) {
        qp.second = Evaluation::drawn_score(board);
        return qp;
    }

    const score_t stand_pat = Evaluation::eval(board);

    alpha = std::max(alpha, stand_pat);

    // Beta cutoff, but don't allow stand-pat in check.
    if (!board.is_check() && stand_pat >= beta) {
        qp.second = stand_pat;
        return qp;
    }

    score_t delta = 900;
    // If pawns are on seventh we could be promoting, delta is higher.
    if (board.pieces(board.who_to_play(), PAWN) & Bitboards::rank(relative_rank(board.who_to_play(), RANK7))) {
        delta += 500;
    }
    // Delta pruning
    if (stand_pat + delta <= alpha) {
        qp.second = stand_pat;
        return qp;
    }

    // Get a list of moves for quiessence. If it's check, it we already have all evasions from the checkmate test.
    // Not in check, we generate quiet checks and all captures.
    if (!board.is_check()) {
        moves = board.get_capture_moves();
    }

    // We already know it's not mate, if there are no captures in a position, return stand pat.
    if (moves.empty()) {
        qp.second = stand_pat;
        return qp;
    }

    // Sort the captures and record SEE.
    Ordering::rank_and_sort_moves(board, moves, NULL_DMOVE);

    for (Move move : moves) {
        // For a capture, the recorded score is the SEE value.
        // It makes sense to not consider losing captures in qsearch.
        if (!board.is_check() && move.is_capture() && !SEE::see(board, move, 0)) {
            continue;
        }
        constexpr score_t see_margin = 100;
        // In qsearch, only consider moves with a decent chance of raising alpha.
        if (!board.is_check() && move.is_capture() && !SEE::see(board, move, alpha - stand_pat - see_margin)) {
            continue;
        }
        board.make_move(move);
        Position sp = quiesce(board, -beta, -alpha);
        const score_t score = -sp.second;
        if (score > alpha) {
            qp.first = sp.first;
        }
        board.unmake_move(move);
        alpha = std::max(alpha, score);
        if (alpha >= beta) {
            break; // beta-cutoff
        }
    }
    qp.second = alpha;
    return qp;
}

void fill_dataset(std::vector<std::string> &files, Dataset &dataset) {
    dataset.clear();
    for (std::string path : files) {
        std::fstream file;
        file.open(path, std::ios::in);
        std::string line;
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
            Position qp = quiesce(board, NEG_INF, POS_INF);
            pair.first = qp.first;
            pair.second = eval;
            dataset.push_back(pair);
        }
    }
}

void fill_parameters(ParameterArray &parameters) {
    parameters.clear();
    for (Evaluation::labled_parameter &p : Evaluation::training_parameters) {
        parameters.push_back(Parameter(&p.first->opening_score, p.second + " Op"));
        parameters.push_back(Parameter(&p.first->endgame_score, p.second + " En"));
    }
}

void train_iteration(Dataset &dataset, const ParameterArray &parameters) {
    const size_t n_parameters = parameters.size();
    std::vector<double> grad(n_parameters, 0.0);
    std::vector<double> initial(n_parameters, 0.0);
    const int display_blocks = 16;
    int display = 0;

    double max_grad = 0.0;
    // Calculate GRAD of loss function ERROR_ON_DATASET
    for (unsigned int i = 0; i < n_parameters; i++) {
        // Only minimise over a subspace to reduce iteration time.

        score_t *working = parameters[i].first;
        std::string label = parameters[i].second;
        // std::cout << label << " ";
        const score_t starting_value = *working;
        initial[i] = starting_value;

        // Check the local conditions projected onto this parameter.
        constexpr score_t step_value = 1;
        (*working) = starting_value - step_value;
        const double p_less = error_on_dataset(dataset);
        (*working) = starting_value + step_value;
        const double p_more = error_on_dataset(dataset);
        (*working) = starting_value;
        double dp = p_more - p_less;
        const double g = dp / step_value;
        // std::cout << std::fixed << std::setprecision(2) << training_rate * g << std::endl;
        grad[i] = g;
        max_grad = std::max(std::abs(g), max_grad);
        if (((i * display_blocks) / n_parameters) > display) {
            display = ((i * display_blocks) / n_parameters);
            // std::cout << display << " / " << display_blocks << std::endl;
        }
    }
    std::cout << "Computed grad" << std::endl;

    // Set smallstep to step s.t. largest change in value is 1. (We are working with ints!)
    const double smallstep = 1. / max_grad;
    double step = 0.0;

    double last = 1e10;
    double next = error_on_dataset(dataset);

    while (next < last) {
        step += smallstep;
        // Increment the paramters by our gradient
        // T_n+1 = T_n - GRAD H (T_n)

        for (unsigned int i = 0; i < n_parameters; i++) {
            double g = step * grad[i];
            *parameters[i].first = initial[i] - int(g);
        }
        last = next;
        next = error_on_dataset(dataset);
        std::cout << " - " << std::fixed << std::setprecision(5) << 100 * std::sqrt(last) << std::endl;
    };

    // Undo the last step
    step -= smallstep;
    for (unsigned int i = 0; i < n_parameters; i++) {
        double g = step * grad[i];
        *parameters[i].first = initial[i] - int(g);
    }
}

Dataset build_subset(const Dataset &dataset, const double probability) {
    std::random_device rd;  // Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
    std::uniform_real_distribution<> dis(0., 1.);

    Dataset subset;

    for (size_t i = 0; i < dataset.size(); i++) {
        if (dis(gen) < probability) {
            subset.push_back(dataset[i]);
        }
    }
    return subset;
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

    ParameterArray parameters;
    fill_parameters(parameters);

    while (true) {
        std::cout << "Filling . . ." << std::endl;
        fill_dataset(files, dataset);
        std::cout << "Done" << std::endl;
        my_clock::time_point origin_time = my_clock::now();
        const double start_error = error_on_dataset(dataset);
        const int iteration_time =
            std::chrono::duration_cast<std::chrono::milliseconds>(my_clock::now() - origin_time).count();
        std::cout << "dataset        : " << std::fixed << std::setprecision(5) << 100 * std::sqrt(start_error) << " ";
        std::cout << "( in " << std::setprecision(2) << iteration_time / 1000. << "s )" << std::endl;
        for (int i = 0; i < 32; i++) {
            Dataset subset = build_subset(dataset, 0.15);
            train_iteration(subset, parameters);
            Evaluation::save_tables(tables_path);
            std::cout << std::fixed << std::setprecision(5) << 100 * std::sqrt(error_on_dataset(dataset)) << std::endl;
        }
    }
}