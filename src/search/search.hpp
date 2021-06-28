#pragma once
#include "../game/board.hpp"
#include <chrono>
#include <thread>
#include <atomic>

typedef std::vector<Move> PrincipleLine;

struct SearchOptions {
    SearchOptions() {
        kill_flag.store(false);
        running_flag.store(false);
    };
    SearchOptions(const SearchOptions& so) {
        kill_flag.store(so.kill_flag.load());
        running_flag.store(so.running_flag.load());
    }
    std::atomic<bool> kill_flag; // Flag to use to tell the search to stop as soon as possible
    score_t eval = MIN_SCORE; // Where the eval is set when the object is shared between threads. 
    unsigned long nodes = 0; // How many nodes have been accessed
    std::thread running_thread; // The thread object for the search itself.
    std::atomic<bool> running_flag;  // Flag set when the search is running.
    bool is_running() const {return running_flag.load(); }
};


// perft.cpp
unsigned long perft_comparison(depth_t depth, Board &board);
unsigned long perft(depth_t depth, Board &board);
void perft_divide(depth_t depth, Board &board);

typedef std::chrono::high_resolution_clock my_clock;

// search.cpp
score_t alphabeta(Board& board, depth_t depth, score_t alpha, score_t beta, PrincipleLine& line, my_clock::time_point time_cutoff, bool allow_cutoff, SearchOptions& options);
score_t quiesce(Board& board, score_t alpha, score_t beta, SearchOptions& options);
score_t pv_search(Board& board, depth_t depth, score_t alpha, score_t beta, PrincipleLine& principle, const uint pv_depth, PrincipleLine& line, my_clock::time_point time_cutoff, bool allow_cutoff, SearchOptions& options);
score_t iterative_deepening(Board& board, const depth_t depth, const int max_millis, PrincipleLine& line, SearchOptions& options);
score_t iterative_deepening(Board& board, const depth_t depth, PrincipleLine& line);
