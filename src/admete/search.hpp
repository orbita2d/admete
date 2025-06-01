#pragma once
#include "board.hpp"
#include <atomic>
#include <chrono>
#include <thread>

typedef std::vector<Move> PrincipleLine;

typedef std::chrono::steady_clock my_clock;

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
    ply_t mate_depth = 0;           // Mate in N distance to look for UCI go mate N commands.
    bool tbenable = false;          // Set true if the tablebase is enabled.
    unsigned long tbhits = 0;
    my_clock::time_point origin_time; // Time At start of search.
    bool is_running() const { return running_flag.load(); }
    bool stop() const { return stop_flag.load(); }
    void set_stop() { stop_flag.store(true); }
    void set_origin() { origin_time = my_clock::now(); }
    unsigned get_millis() {
        return 1 + std::chrono::duration_cast<std::chrono::milliseconds>(my_clock::now() - origin_time).count();
    }
    // bool passed_time() { return (get_millis() > hard_cutoff); }
};
score_t scout_search(Board &board, depth_t depth, const score_t alpha, unsigned int time_cutoff,
                     const bool allow_cutoff, const bool allow_null, NodeType node, SearchOptions &options);
score_t pv_search(Board &board, depth_t depth, const score_t alpha, const score_t beta, PrincipleLine &line,
                  unsigned int time_cutoff, const bool allow_cutoff, SearchOptions &options);
score_t quiesce(Board &board, score_t alpha, const score_t beta, SearchOptions &options);
score_t search(Board &board, const depth_t depth, int soft_cutoff, const int hard_cutoff, PrincipleLine &line,
               SearchOptions &options);
score_t search(Board &board, const depth_t depth, PrincipleLine &line);
DenseBoard board_quiesce(Board &board);

unsigned long perft(depth_t depth, Board &board);
unsigned long perft(depth_t depth, Board &board, SearchOptions &options);
unsigned long perft_bulk(depth_t depth, Board &board);
void perft_divide(depth_t depth, Board &board);

// Search parameters
#ifdef WITH_TUNING
#define PARAMETER inline
#else
#define PARAMETER constexpr
#endif

constexpr depth_t efp_max_depth = 1;
constexpr depth_t rfp_max_depth = 3;
PARAMETER std::array<score_t, efp_max_depth+1> extended_futility_margins = {0, 36, };
PARAMETER std::array<score_t,rfp_max_depth+1>  reverse_futility_margins = {0, 325, 550, 800};
PARAMETER depth_t null_move_depth_reduction = 2;
PARAMETER depth_t probcut_depth_reduction = 3;
PARAMETER depth_t probcut_min_depth = 6;
PARAMETER score_t probcut_margin = 173;
PARAMETER int16_t reductions_quiet_di = 40;
PARAMETER int16_t reductions_quiet_d = 0;
PARAMETER int16_t reductions_quiet_i = 0;
PARAMETER int16_t reductions_quiet_c = 100;
PARAMETER int16_t reductions_capture_di = 25;
PARAMETER int16_t reductions_capture_d = 0;
PARAMETER int16_t reductions_capture_i = 0;
PARAMETER int16_t reductions_capture_c = 0;
PARAMETER depth_t history_max_depth = 3;
PARAMETER score_t history_prune_threshold = 15;
PARAMETER score_t see_prune_threshold = 50;
void init();
void reinit();
inline std::array<std::array<std::array<depth_t, MAX_MOVES>, MAX_DEPTH>, 2> reductions_table;
} // namespace Search