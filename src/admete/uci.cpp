#include "uci.hpp"
#include "board.hpp"
#include "evaluate.hpp"
#include "search.hpp"
#include "tablebase.hpp"
#include "transposition.hpp"
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include "ordering.hpp"
#include "weights.hpp"

#define ENGINE_NAME "admete"
#define ENGINE_VERS "v1.7.0pre"
#define ENGINE_AUTH "Kylie MacFarquharson (née orbita)"

namespace UCI {

class UciOption {
public:
    virtual std::string print() const = 0;
    virtual void set(std::string value) = 0;
    std::string name;
};

template<typename T>
class UciOptionSpin : public UciOption {
static_assert(std::is_integral<T>::value, "T must be an integral type");
public:
    UciOptionSpin(T min, T max, T def, std::string name, T *value) : min(min), max(max), def(def), value(value) { this->name = name; }
    std::string print() const override {
        return "option name " + name + " type spin default " + std::to_string(def) + " min " + std::to_string(min) +
               " max " + std::to_string(max);
    }

    void set(std::string value) override {
        // if the string is not a number, stoi will throw an exception. Do some basic error checking.
        if (value.empty()) {
            std::cerr << "Empty value" << std::endl;
            return;
        }
        try {
            T val = static_cast<T>(std::stoi(value));
            if (val < min ) {
                std::cerr << "Value too low: " << val << " < " << min << std::endl;
            } else if (val > max) {
                std::cerr << "Value too high: " << val << " > " << max << std::endl;
            } else {
                *(this->value ) = val; 
            }
        } catch (std::invalid_argument &e) {
            std::cerr << "Invalid argument: " << value << std::endl;
        }
    }
private:
    T min, max, def;
    T *value;
};

class UciOptionString : public UciOption {
public:
    UciOptionString(std::string name, std::string def, std::string *value) :def(def), value(value) { this->name = name; }
    std::string print() const override {
        return "option name " + name + " type string default " + def;
    }
    void set(std::string value) override { *(this->value) = value; }
private:
    std::string def;
    std::string *value;
};

inline std::vector<UciOption*> uci_options;    
void init_uci() {
    // set up the UCI options
    uci_options.clear();
    // efp margins
    for (size_t i = 1; i < Search::extended_futility_margins.size(); i++) {
        uci_options.push_back(new UciOptionSpin<score_t>(0, TBWIN_MIN-1, Search::extended_futility_margins[i], "efp_margin_" + std::to_string(i), &Search::extended_futility_margins[i]));
    }
    // rfp margins
    for (size_t i = 1; i < Search::reverse_futility_margins.size(); i++) {
        uci_options.push_back(new UciOptionSpin<score_t>(0, TBWIN_MIN-1, Search::reverse_futility_margins[i], "rfp_margin_" + std::to_string(i), &Search::reverse_futility_margins[i]));
    }
    uci_options.push_back(new UciOptionSpin<int16_t>(-1000, 1000, Search::reductions_quiet_di, "reductions_quiet_di", &Search::reductions_quiet_di));
    uci_options.push_back(new UciOptionSpin<int16_t>(-1000, 1000, Search::reductions_quiet_d, "reductions_quiet_d", &Search::reductions_quiet_d));
    uci_options.push_back(new UciOptionSpin<int16_t>(-1000, 1000, Search::reductions_quiet_i, "reductions_quiet_i", &Search::reductions_quiet_i));
    uci_options.push_back(new UciOptionSpin<int16_t>(-1000, 1000, Search::reductions_quiet_c, "reductions_quiet_c", &Search::reductions_quiet_c));
    uci_options.push_back(new UciOptionSpin<int16_t>(-1000, 1000, Search::reductions_capture_di, "reductions_capture_di", &Search::reductions_capture_di));
    uci_options.push_back(new UciOptionSpin<int16_t>(-1000, 1000, Search::reductions_capture_d, "reductions_capture_d", &Search::reductions_capture_d));
    uci_options.push_back(new UciOptionSpin<int16_t>(-1000, 1000, Search::reductions_capture_i, "reductions_capture_i", &Search::reductions_capture_i));
    uci_options.push_back(new UciOptionSpin<int16_t>(-1000, 1000, Search::reductions_capture_c, "reductions_capture_c", &Search::reductions_capture_c));
    uci_options.push_back(new UciOptionSpin<score_t>(0, 1000, Search::probcut_margin, "probcut_margin", &Search::probcut_margin));
    uci_options.push_back(new UciOptionSpin<depth_t>(0, 12, Search::probcut_min_depth, "probcut_min_depth", &Search::probcut_min_depth));
    uci_options.push_back(new UciOptionSpin<depth_t>(0, 12, Search::probcut_depth_reduction, "probcut_depth_reduction", &Search::probcut_depth_reduction));
    uci_options.push_back(new UciOptionSpin<score_t>(0, 1000, Evaluation::contempt, "contempt", &Evaluation::contempt));
    uci_options.push_back(new UciOptionSpin<depth_t>(0, 12, Search::null_move_depth_reduction, "null_move_depth_reduction", &Search::null_move_depth_reduction));
    uci_options.push_back(new UciOptionSpin<depth_t>(0, 12, Search::history_max_depth, "history_max_depth", &Search::history_max_depth));
    uci_options.push_back(new UciOptionSpin<score_t>(0, 1000, Search::history_prune_threshold, "history_prune_threshold", &Search::history_prune_threshold));
    uci_options.push_back(new UciOptionSpin<score_t>(0, 1000, Search::see_prune_threshold, "see_prune_threshold", &Search::see_prune_threshold));
    

    std::cout << "id name " << ENGINE_NAME << " " << ENGINE_VERS << std::endl;
    std::cout << "id author " << ENGINE_AUTH << std::endl;
    std::cout << "option name Hash type spin default " << Cache::hash_default << " min " << Cache::hash_min << " max "
              << Cache::hash_max << std::endl;
    std::cout << "option name SyzygyPath type string default <empty>" << std::endl;

    for (const auto &option : uci_options) {
        std::cout << option->print() << std::endl;
    }

    std::cout << "uciok" << std::endl;
}

void set_option(std::istringstream &is, Search::SearchOptions &options) {
    /*
     * setoption name  [value ]
     *        this is sent to the engine when the user wants to change the internal parameters
     *        of the engine. For the "button" type no value is needed.
     *        One string will be sent for each parameter and this will only be sent when the engine is waiting.
     *        The name of the option in  should not be case sensitive and can inludes spaces like also the value.
     *        The substrings "value" and "name" should be avoided in and to allow unambiguous parsing,
     *        for example do not use  = "draw value".
     *        Here are some strings for the example below:
     *          "setoption name Nullmove value true\n"
     *          "setoption name Selectivity value 3\n"
     *          "setoption name Style value Risky\n"
     *          "setoption name Clear Hash\n"
     *          "setoption name NalimovPath value c:\chess\tb\4;c:\chess\tb\5\n"
     */
    std::string token, option;
    is >> std::ws >> token;
    if (token == "name") {
        while (is >> token && token != "value") {
            if (option.empty()) {
                option += token;
            } else {
                option += " " + token;
            }
        }
    } else {
        return;
    }
    if (token != "value") {
        return;
    }
    if (option == "Hash") {
        unsigned value = 1;
        is >> std::ws >> value;
        
        if (value > Cache::hash_max) {
            std::cerr << "Hash max = " << Cache::hash_max << " MiB" << std::endl;
        } else if (value < Cache::hash_min) {
            std::cerr << "Hash min = " << Cache::hash_min << " MiB" << std::endl;
        }
        value = std::clamp(value, Cache::hash_min, Cache::hash_max);
        Cache::tt_max = (value * (1 << 20)) / sizeof(Cache::TransElement);
        Cache::reinit();
        return;
    } 
    
    if (option == "SyzygyPath") {
        // Set the path to a file of input paramters
        std::string value;
        while (is >> token) {
            if (value.empty()) {
                value += token;
            } else {
                value += " " + token;
            }
        }
        options.tbenable = true;
        const bool success = Tablebase::init(value);
        if (success) {
            std::cerr << "Load Syzygy EGTB successful." << std::endl;
        } else {
            std::cerr << "Load Syzygy EGTB unsuccessful." << std::endl;
        }
        
        return;
    }

    std::string value;
    while (is >> token) {
        if (value.empty()) {
            value += token;
        } else {
            value += " " + token;
        }
    }

    for (const auto &opt : uci_options) {
        if (option == opt->name) {
            opt->set(value);
            Search::reinit();
            return;
        }
    }

    std::cout << "Unknown option: \"" << option << "\"" << std::endl;
}
void position(Board &board, std::istringstream &is) {
    /*
        position [fen <fenstring> | startpos ]  moves <move1> .... <movei>
        set up the position described in fenstring on the internal board and
        play the moves on the internal chess board.
        if the game was played  from the start position the string "startpos" will be sent
        Note: no "new" command is needed. However, if this position is from a different game than
        the last position sent to the engine, the GUI should have sent a "ucinewgame" inbetween.
    */
    std::string token, fen;
    is >> std::ws >> token;
    // Need to know what the position is.
    if (token == "startpos") {
        fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        is >> token;
    } else if (token == "fen") {
        while (is >> token && token != "moves") {
            // munch through the command string
            if (token == "\"") {
                continue;
            }
            fen += token + " ";
        }
    } else {
        // This is invalid. Just ignore it
        std::cerr << "Invalid position string: \"" << token << "\"" << std::endl;
        return;
    }
    board.fen_decode(fen);
    if (token == "moves") {
        while (is >> token) {
            // munch through the command string
            board.try_uci_move(token);
        }
    }
}

void bestmove(Board &board, const Move move) {
    /*
    bestmove <move1> [ ponder <move2> ]
        the engine has stopped searching and found the move <move> best in this position.
        the engine can send the move it likes to ponder on. The engine must not start pondering automatically.
        this command must always be sent if the engine stops searching, also in pondering mode if there is a
        "stop" command, so for every "go" command a "bestmove" command is needed!
        Directly before that the engine should send a final "info" command with the final search information,
        the the GUI has the complete statistics about the last search.
    */
    MoveList legal_moves = board.get_moves();
    if (is_legal(move, legal_moves)) {
        std::cout << "bestmove " << move.pretty() << std::endl;
    } else {
        std::cerr << "illegal move!: " << board.fen_encode() << std::endl;
        std::cout << "bestmove " << legal_moves[0].pretty() << std::endl;
    }
}

void do_search(Board *board, depth_t max_depth, const int soft_cutoff, const int hard_cutoff,
               Search::SearchOptions *options) {
    PrincipleLine line;
    line.reserve(max_depth);
    options->stop_flag.store(false);
    options->running_flag.store(true);
    options->nodes = 0;
    int score = Search::search(*board, max_depth, soft_cutoff, hard_cutoff, line, *options);
    options->eval = score;
    Move first_move = line.back();
    bestmove(*board, first_move);
    // Set this so that the thread can be joined.
    options->running_flag.store(false);
}

void cleanup_thread(Search::SearchOptions &options) {
    if (!options.is_running() && options.running_thread.joinable()) {
        options.running_thread.join();
    }
}

void go(Board &board, std::istringstream &is, Search::SearchOptions &options) {
    /*
    * go
    start calculating on the current position set up with the "position" command.
    There are a number of commands that can follow this command, all will be sent in the same string.
    If one command is not sent its value should be interpreted as it would not influence the search.
    * searchmoves <move1> .... <movei>
        restrict search to this moves only
        Example: After "position startpos" and "go infinite searchmoves e2e4 d2d4"
        the engine should only search the two moves e2e4 and d2d4 in the initial position.
    * ponder
        start searching in pondering mode.
        Do not exit the search in ponder mode, even if it's mate!
        This means that the last move sent in in the position string is the ponder move.
        The engine can do what it wants to do, but after a "ponderhit" command
        it should execute the suggested move to ponder on. This means that the ponder move sent by
        the GUI can be interpreted as a recommendation about which move to ponder. However, if the
        engine decides to ponder on a different move, it should not display any mainlines as they are
        likely to be misinterpreted by the GUI because the GUI expects the engine to ponder
        on the suggested move.
    * wtime <x>
        white has x msec left on the clock
    * btime <x>
        black has x msec left on the clock
    * winc <x>
        white increment per move in mseconds if x > 0
    * binc <x>
        black increment per move in mseconds if x > 0
    * movestogo <x>
        there are x moves to the next time control,
        this will only be sent if x > 0,
        if you don't get this and get the wtime and btime it's sudden death
    * depth <x>
        search x plies only.
    * nodes <x>
        search x nodes only,
    * mate <x>
        search for a mate in x moves
    * movetime <x>
        search exactly x mseconds
    * infinite
        search until the "stop" command. Do not exit the search without being told so in this mode!
    */

    int wtime = POS_INF, btime = POS_INF; // Time left in this time control.
    int winc = 0, binc = 0;               // Increments
    int move_time = POS_INF;              // Absolute max time to spend on this move.
    uint max_depth = 20;                  // Absolute max depth to calculate to.
    uint movestogo = 0;                   // How many moves till the next time control.
    std::string token;
    while (is >> token) {
        // munch through the command string
        if (token == "wtime") {
            is >> wtime;
        } else if (token == "btime") {
            is >> btime;
        } else if (token == "winc") {
            is >> winc;
        } else if (token == "binc") {
            is >> binc;
        } else if (token == "depth") {
            is >> max_depth;
        } else if (token == "movetime") {
            is >> move_time;
        } else if (token == "mate") {
            is >> options.mate_depth;
        } else if (token == "movestogo") {
            is >> movestogo;
        }
    }
    const int our_time = board.is_white_move() ? wtime : btime;
    const int our_inc = board.is_white_move() ? winc : binc;
    int soft_cutoff; // How much time to spend on this move.
    int hard_cutoff;
    if (our_time == POS_INF) {
        // Infinite analysis
        soft_cutoff = POS_INF;
        hard_cutoff = POS_INF;
    } else if (movestogo == 0) {
        // Sudden death time control, try fit sd_N_move moves in the rest of the game
        constexpr int sd_n_move = 20;
        soft_cutoff = (our_time / sd_n_move + our_inc);
        hard_cutoff = soft_cutoff * 3;
    } else {
        // Classical type time control. Try fit however many moves till the next time control plus one in the game.
        soft_cutoff = (our_time / (.5 * movestogo + 1) + our_inc);
        hard_cutoff = soft_cutoff * 2.5;
    }
    // Time passed after which we kill the search, (to keep up with movetime resitriction, to not lose on time, to not
    // waste time if bf explodes).
    hard_cutoff = std::min(std::min(move_time, (int)(our_time * 0.8)), hard_cutoff);
    options.running_thread = std::thread(&do_search, &board, (depth_t)max_depth, soft_cutoff, hard_cutoff, &options);
}

void do_perft(Board *board, const depth_t depth, Search::SearchOptions *options) {
    // Mark the search thread as running.
    options->running_flag.store(true);
    options->stop_flag = false;
    options->nodes = 0;

    my_clock::time_point time_origin = my_clock::now();

    options->nodes = Search::perft(depth, *board, *options);

    std::chrono::duration<double, std::milli> time_span = my_clock::now() - time_origin;
    unsigned long nps = int(1000 * (options->nodes / time_span.count()));

    if (!options->stop()) {
        uci_info(depth, options->nodes, nps, time_span.count());
    }

    // Mark the search thread as finished (and ready to join)
    options->running_flag.store(false);
}

void perft(Board &board, std::istringstream &is, Search::SearchOptions &options) {
    // perft <depth>

    // This command ignores the <fen> part (and relies on position being set already).
    // Returns the perft score for that depth.

    // std::istringstream >> uint8_t doesn't do what you think.
    int depth;
    is >> depth;

    options.running_thread = std::thread(&do_perft, &board, (depth_t)depth, &options);
}

void divide(Board &board, std::istringstream &is, Search::SearchOptions) {
    /* perft <depth>
     */

    // This command ignores the <fen> part (and relies on position being set already).
    // Returns the perft score for that depth.

    // std::istringstream >> uint8_t doesn't do what you think.
    int depth = 1;
    is >> depth;
    Search::perft_divide((depth_t)depth, board);
}

void stop(Search::SearchOptions &options) {
    /*
    stop calculating as soon as possible,
    don't forget the "bestmove" and possibly the "ponder" token when finishing the search
    */
    if (options.is_running()) {
        options.set_stop();
        options.running_thread.join();
    }
}

void uci_info(depth_t depth, score_t eval, unsigned long nodes, unsigned long tbhits, unsigned long nps,
              PrincipleLine principle, unsigned int time, ply_t root_ply) {
    if (!uci_enabled) {
        return;
    }
    std::cout << std::dec;
    std::cout << "info";
    std::cout << " depth " << (uint)depth;

    if (is_mating(eval)) {
        // Mate for white. Score is (MATING_SCORE - mate_ply)
        score_t n = (MATING_SCORE - eval - root_ply + 1) / 2;
        std::cout << " score mate " << (int)n;
    } else if (is_mating(-eval)) {
        // Mate for black. Score is (mate_ply - MATING_SCORE)
        score_t n = (eval + MATING_SCORE - root_ply) / 2;
        std::cout << " score mate " << -(int)n;
    } else {
        std::cout << " score cp " << (int)eval;
    }
    if (nodes > 0) {
        std::cout << " nodes " << nodes;
    }
    if (tbhits > 0) {
        std::cout << " tbhits " << tbhits;
    }
    if (nps > 0) {
        std::cout << " nps " << nps;
    }
    std::cout << " pv ";
    for (PrincipleLine::reverse_iterator it = principle.rbegin(); it != principle.rend(); ++it) {
        std::cout << it->pretty() << " ";
    }
    std::cout << " time " << time;
    std::cout << std::endl;
}

void uci_info(depth_t depth, unsigned long nodes, unsigned long nps, unsigned int time) {
    // This one only used for perft.
    if (!uci_enabled) {
        return;
    }
    std::cout << std::dec;
    std::cout << "info";
    std::cout << " depth " << (uint)depth;

    if (nodes > 0) {
        std::cout << " nodes " << nodes;
    }
    if (nps > 0) {
        std::cout << " nps " << nps;
    }
    std::cout << " time " << time;
    std::cout << std::endl;
}

void uci_info_nodes(unsigned long nodes, unsigned long nps) {
    if (!uci_enabled) {
        return;
    }
    if (nodes > 0) {
        std::cout << "nodes " << nodes;
    }
    if (nps > 0) {
        std::cout << " nps " << nps;
    }
    std::cout << std::endl;
}

// TODO: Tidy this up
typedef std::pair<DenseBoard, score_t> Position;
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
        Position sp = UCI::quiesce(board, -beta, -alpha);
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


void print_features(Board &board, std::istringstream &is) {
    // Print the feature vector from the neural network.
    // Used to build the training set.

    // if the second token is "quiesce", quiesce the board before encoding.
    std::string token;
    is >> token;
    per_colour<Neural::Feature2Vector> features;
    if (token == "quiesce") {
        auto starting_pos = board.pack();
        Position pos = UCI::quiesce(board, MIN_SCORE, MAX_SCORE);
        board.unpack(pos.first);
        features = Neural::encode2(board);
        board.unpack(starting_pos);
    } else {
        features = Neural::encode2(board);
    }

    Colour player = board.who_to_play();
    const size_t feature_len = Neural::N_FEATURES2;
    for (size_t i = 0; i < feature_len; i++) {
        std::cout << std::get<0>(features[player])[i];
    }
    std::cout << ";" << Square::square_t(std::get<1>(features[player])) << ";";
    for (size_t i = 0; i < feature_len; i++) {
        std::cout << std::get<0>(features[~player])[i];
    }
    std::cout << ";" << Square::square_t(std::get<1>(features[~player])) << std::endl;
}

void uci() {
    uci_enabled = true;
    std::string command, token;
    Board board = Board();
    Search::SearchOptions options = Search::SearchOptions();
    std::cout << ENGINE_NAME << " " << ENGINE_VERS << " by " << ENGINE_AUTH << std::endl;
    while (true) {
        std::getline(std::cin, command);
        cleanup_thread(options);
        std::istringstream is(command);
        is >> std::ws >> token;
        if (token == "uci") {
            init_uci();
        } else if (token == "isready") {
            // Interface is asking if we can continue, if we are here, we clearly can.
            stop(options);
            std::cout << "readyok" << std::endl;
        } else if (token == "ucinewgame") {
            board.initialise_starting_position();
        } else if (token == "setoption") {
            stop(options);
            set_option(is, options);
        } else if (token == "position") {
            stop(options);
            position(board, is);
        } else if (token == "go") {
            stop(options);
            go(board, is, options);
        } else if (token == "perft") {
            stop(options);
            perft(board, is, options);
        } else if (token == "divide") {
            stop(options);
            divide(board, is, options);
        } else if (token == "stop") {
            stop(options);
        } else if (token == "quit") {
            exit(EXIT_SUCCESS);
        } else if (token == "d") {
            board.pretty();
        } else if (token == "h") {
            score_t v = Evaluation::evaluate_white(board);
            std::cout << std::dec << (int)v << std::endl;
        } else if (token == "features") {
            print_features(board, is);
        }
        else {
            std::cerr << "Unknown command: " << token << std::endl;
        }
    }
}

} // namespace UCI