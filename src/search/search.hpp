#pragma once
#include "../game/board.hpp"
#include <atomic>
#include <chrono>
#include <thread>

typedef std::vector<Move> PrincipleLine;

typedef std::chrono::high_resolution_clock my_clock;

namespace Search {

struct SearchOptions {
    SearchOptions() {
        stop_flag.store(false);
        running_flag.store(false);
    };
    SearchOptions(const SearchOptions &so) {
        stop_flag.store(so.stop_flag.load());
        running_flag.store(so.running_flag.load());
    }
    std::atomic<bool> stop_flag;    // Flag to use to tell the search to stop as soon as possible
    score_t eval = MIN_SCORE;       // Where the eval is set when the object is shared between threads.
    unsigned long nodes = 0;        // How many nodes have been accessed
    std::thread running_thread;     // The thread object for the search itself.
    std::atomic<bool> running_flag; // Flag set when the search is running.
    bool is_running() const { return running_flag.load(); }
    bool stop() const { return stop_flag.load(); }
    void set_stop() { stop_flag.store(true); }
};
score_t scout_search(Board &board, depth_t depth, const score_t alpha, my_clock::time_point time_cutoff,
                     const bool allow_cutoff, const bool allow_null, SearchOptions &options);
score_t pv_search(Board &board, depth_t depth, const score_t alpha, const score_t beta, PrincipleLine &line,
                  my_clock::time_point time_cutoff, const bool allow_cutoff, SearchOptions &options);
score_t quiesce(Board &board, score_t alpha, const score_t beta, SearchOptions &options);
score_t search(Board &board, const depth_t depth, const int max_millis, PrincipleLine &line, SearchOptions &options);
score_t search(Board &board, const depth_t depth, PrincipleLine &line);

unsigned long perft(depth_t depth, Board &board);
unsigned long perft(depth_t depth, Board &board, SearchOptions &options);
unsigned long perft_bulk(depth_t depth, Board &board);
void perft_divide(depth_t depth, Board &board);

} // namespace Search