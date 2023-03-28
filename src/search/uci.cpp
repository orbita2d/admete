
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

#define ENGINE_NAME "admete"
#define ENGINE_VERS "pre2023-03-28"
#define ENGINE_AUTH "orbita"

namespace UCI {
void init_uci() {
    std::cout << "id name " << ENGINE_NAME << " " << ENGINE_VERS << std::endl;
    std::cout << "id author " << ENGINE_AUTH << std::endl;
    std::cout << "option name Hash type spin default " << Cache::hash_default << " min " << Cache::hash_min << " max "
              << Cache::hash_max << std::endl;
    std::cout << "option name TablesPath type string default <empty>" << std::endl;
    std::cout << "option name SyzygyPath type string default <empty>" << std::endl;
    std::cout << "uciok" << std::endl;
}

void set_option(std::istringstream &is, Search::SearchOptions &options) {
    /*
     * setoption name  [value ]
     *        this is sent to the engine when the user wants to change the internal parameters
     *        of the engine. For the "button" type no value is needed.
     *        One string will be sent for each parameter and this will only be sent when the engine is waiting.
     *        The name of the option in  should not be case sensitive and can inludes spaces like also the value.
     *        The substrings "value" and "name" should be avoided in  and  to allow unambiguous parsing,
     *        for example do not use  = "draw value".
     *        Here are some strings for the example below:
     *           "setoption name Nullmove value true\n"
     *      "setoption name Selectivity value 3\n"
     *           "setoption name Style value Risky\n"
     *           "setoption name Clear Hash\n"
     *           "setoption name NalimovPath value c:\chess\tb\4;c:\chess\tb\5\n"
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
    if (option == "Hash") {
        unsigned value = 1;
        if (token == "value") {
            is >> std::ws >> value;
        } else {
            return;
        }
        if (value > Cache::hash_max) {
            std::cerr << "Hash max = " << Cache::hash_max << " MiB" << std::endl;
        } else if (value < Cache::hash_min) {
            std::cerr << "Hash min = " << Cache::hash_min << " MiB" << std::endl;
        }
        value = std::clamp(value, Cache::hash_min, Cache::hash_max);
        Cache::tt_max = (value * (1 << 20)) / sizeof(Cache::TransElement);
        Cache::reinit();
    } else if (option == "SyzygyPath") {
        // Set the path to a file of input paramters
        std::string value;
        if (token == "value") {
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
        } else {
            return;
        }
    } else if (option == "TablesPath") {
        // Set the path to a file of input paramters
        std::string value;
        if (token == "value") {
            while (is >> token) {
                if (value.empty()) {
                    value += token;
                } else {
                    value += " " + token;
                }
            }
        } else {
            return;
        }
        Evaluation::load_tables(value);
    } else {
        std::cout << "Unknown option: \"" << option << "\"" << std::endl;
    }
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

void show_tests(Board &board) {
    // Show a lot of calculated bitboards for the position. This can be helpful in improving the evaluation.
    board.pretty();

    std::cout << "White Passed Pawns:" << std::endl;
    Bitboards::pretty(board.passed_pawns(WHITE));

    std::cout << "Black Passed Pawns:" << std::endl;
    Bitboards::pretty(board.passed_pawns(BLACK));

    std::cout << "White Weak Pawns:" << std::endl;
    Bitboards::pretty(board.weak_pawns(WHITE));

    std::cout << "Black Weak Pawns:" << std::endl;
    Bitboards::pretty(board.weak_pawns(BLACK));

    std::cout << "White Half-Open Files:" << std::endl;
    Bitboards::pretty(board.half_open_files(WHITE));

    std::cout << "Black Half-Open Files:" << std::endl;
    Bitboards::pretty(board.half_open_files(BLACK));

    std::cout << "Open Files:" << std::endl;
    Bitboards::pretty(board.open_files());

    std::cout << "White Isolated Pawns:" << std::endl;
    Bitboards::pretty(board.isolated_pawns(WHITE));

    std::cout << "Black Isolated Pawns:" << std::endl;
    Bitboards::pretty(board.isolated_pawns(BLACK));

    std::cout << "White Connected Passed Pawns:" << std::endl;
    Bitboards::pretty(board.connected_passed_pawns(WHITE));

    std::cout << "Black Connected Passed Pawns:" << std::endl;
    Bitboards::pretty(board.connected_passed_pawns(BLACK));

    std::cout << "King Saftey:" << std::endl;
    Bitboard occ = Bitboards::attacks<QUEEN>(board.pieces(PAWN), board.find_king(WHITE));
    occ &= ~board.pieces(PAWN);
    occ &= ~Bitboards::pawn_attacks(WHITE, board.pieces(WHITE, PAWN));
    Bitboards::pretty(occ);

    std::cout << "White weak squares:" << std::endl;
    Bitboards::pretty(board.weak_squares(WHITE));
    std::cout << "Black weak squares:" << std::endl;
    Bitboards::pretty(board.weak_squares(BLACK));

    std::cout << "White Outposts:" << std::endl;
    Bitboards::pretty(board.outposts(WHITE));
    std::cout << "Black Outposts:" << std::endl;
    Bitboards::pretty(board.outposts(BLACK));
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
    options->running_flag.store(true);
    options->stop_flag = false;
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
        } else if (token == "test") {
            show_tests(board);
        } else if (token == "tables") {
            Evaluation::print_tables();
        } else if (token == "savetables") {
            Evaluation::save_tables("out.txt");
        } else {
            std::cerr << "Unknown command: " << token << std::endl;
        }
    }
}

} // namespace UCI