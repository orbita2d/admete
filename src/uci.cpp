
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <chrono>

#include "uci.hpp"
#include "board.hpp"
#include "search.hpp"

#define ENGINE_NAME "admete"
#define ENGINE_AUTH "orbita"
typedef std::chrono::high_resolution_clock my_clock;

void init_uci() {
    std::cerr << "uci mode" << std::endl;
    std::cout << "id name " << ENGINE_NAME << std::endl;
    std::cout << "id author " << ENGINE_AUTH << std::endl;
    std::cout << "setoption name Nullmove value false" << std::endl;
    std::cout << "uciok" << std::endl;
    std::cerr << "uci okay" << std::endl;
}

void position(Board& board, std::istringstream& is) {
    /*
        position [fen <fenstring> | startpos ]  moves <move1> .... <movei>
        set up the position described in fenstring on the internal board and
        play the moves on the internal chess board.
        if the game was played  from the start position the string "startpos" will be sent
        Note: no "new" command is needed. However, if this position is from a different game than
        the last position sent to the engine, the GUI should have sent a "ucinewgame" inbetween.
    */
    std::string token, fen;
    is >> token;
    // Need to know what the position is.
    if (token == "startpos") {
        fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
        is >> token;
    } else if (token == "fen") {
        while (is >> token && token != "moves") {
            // munch through the command string
            fen += token + " ";
        }
    } else {
        // This is invalid. Just ignore it
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

void bestmove(Move move) {
    /* 
    bestmove <move1> [ ponder <move2> ]
	the engine has stopped searching and found the move <move> best in this position.
	the engine can send the move it likes to ponder on. The engine must not start pondering automatically.
	this command must always be sent if the engine stops searching, also in pondering mode if there is a
	"stop" command, so for every "go" command a "bestmove" command is needed!
	Directly before that the engine should send a final "info" command with the final search information,
	the the GUI has the complete statistics about the last search.
    */
   std::cerr << "bestmove " << move.pretty_print() << std::endl;
   std::cout << "bestmove " << move.pretty_print() << std::endl;
}

void go(Board& board, std::istringstream& is) {
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

    // That's a lot, let's just search to a fixed depth for now.
    int wtime = POS_INF, btime = POS_INF;
    int winc = 0, binc = 0;
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
        }
    }
    const int our_time = board.is_white_move() ? wtime : btime;
    const int our_inc  = board.is_white_move() ? winc : binc;
    const int cutoff_time = our_time / 20 + our_inc*3/5;
    constexpr int max_depth = 8;
    std::cerr << "searching: " << float(cutoff_time) / 1000  << std::endl;
    std::vector<Move> line;
    line.reserve(max_depth);
    my_clock::time_point time_origin = my_clock::now();
    int score = iterative_deepening(board, max_depth, cutoff_time, line);
    std::chrono::duration<double, std::milli> time_span = my_clock::now() - time_origin;
    Move first_move = line.back();
    std::cerr << "found move: " << first_move.pretty_print() << ": " << print_score(score) << "(depth: " << line.size() << ") ";
    std::cerr << "in " << std::setprecision(2) <<time_span.count() / 1000 << "s" << std::endl;
    bestmove(first_move);
    
}
void uci() {
    init_uci();
    std::string command, token;
    bool debug;
    Board board = Board();
    while (true) {
        std::getline(std::cin, command);
        std::istringstream is(command);
        is >> std::ws >> token;
        if (token == "isready") {
            // interface is asking if we can continue, if we are here, we clearly can.
            std::cout << "readyok" << std::endl;
        } else if (token == "ucinewgame") {
            board.initialise_starting_position();
        } else if (token == "position") {
            std::cerr << command << std::endl;
            position(board, is);
        } else if (token == "go") {
            std::cerr << command << std::endl;
            go(board, is);
        } else if (token == "quit") {
            exit(EXIT_SUCCESS);
        } else if (token == "d") {
            board.pretty();
        }
        else {
            std::cerr << "!#" << token << ":"<< command << std::endl;
        }
    }

}