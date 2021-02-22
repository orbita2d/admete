#include <gtest/gtest.h>
#include "testing.hpp"
#include "board.hpp"


TEST(FenEncoding, initial) {
    std::string board_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 ";
    Board board = Board();
    board.fen_decode(board_fen);
    EXPECT_EQ(board.fen_encode(), board_fen);
}

TEST(PerftTest, initial1) {
    std::string board_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 ";
    Board board = Board();
    board.fen_decode(board_fen);
    EXPECT_EQ(perft_bulk(1, board),  20);
}

TEST(PerftTest, initial2) {
    std::string board_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 ";
    Board board = Board();
    board.fen_decode(board_fen);
    EXPECT_EQ(perft_bulk(2, board),  400);
}

TEST(PerftTest, initial3) {
    std::string board_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 ";
    Board board = Board();
    board.fen_decode(board_fen);
    EXPECT_EQ(perft_bulk(3, board),  8902);
}

TEST(PerftTest, initial4) {
    std::string board_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 ";
    Board board = Board();
    board.fen_decode(board_fen);
    EXPECT_EQ(perft_bulk(4, board),  197281);
}


TEST(PerftTest, initial5) {
    std::string board_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 ";
    Board board = Board();
    board.fen_decode(board_fen);
    EXPECT_EQ(perft_bulk(5, board),  4865609);
}

TEST(PerftTest, kiwipete1) {
    std::string board_fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
    Board board = Board();
    board.fen_decode(board_fen);
    EXPECT_EQ(perft_bulk(1, board),  48);
}

TEST(PerftTest, kiwipete2) {
    std::string board_fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
    Board board = Board();
    board.fen_decode(board_fen);
    EXPECT_EQ(perft_bulk(2, board),   2039);
}

TEST(PerftTest, kiwipete3) {
    std::string board_fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
    Board board = Board();
    board.fen_decode(board_fen);
    EXPECT_EQ(perft_bulk(3, board),  97862);
}

TEST(PerftTest, kiwipete4) {
    std::string board_fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
    Board board = Board();
    board.fen_decode(board_fen);
    EXPECT_EQ(perft_bulk(4, board),  4085603);
}
/*
TEST(PerftTest, kiwipete5) {
    std::string board_fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
    Board board = Board();
    board.fen_decode(board_fen);
    EXPECT_EQ(perft_bulk(5, board),  193690690);
}
*/

TEST(PerftTest, wikithree1) {
    std::string board_fen = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -";
    Board board = Board();
    board.fen_decode(board_fen);
    EXPECT_EQ(perft_bulk(1, board),  14);
}

TEST(PerftTest, wikithree2) {
    std::string board_fen = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -";
    Board board = Board();
    board.fen_decode(board_fen);
    EXPECT_EQ(perft_bulk(2, board),  191);
}

TEST(PerftTest, wikithree3) {
    std::string board_fen = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -";
    Board board = Board();
    board.fen_decode(board_fen);
    EXPECT_EQ(perft_bulk(3, board),  2812);
}

TEST(PerftTest, wikithree4) {
    std::string board_fen = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -";
    Board board = Board();
    board.fen_decode(board_fen);
    EXPECT_EQ(perft_bulk(4, board),  43238);
}

TEST(PerftTest, wikithree5) {
    std::string board_fen = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -";
    Board board = Board();
    board.fen_decode(board_fen);
    EXPECT_EQ(perft_bulk(5, board),  674624);
}


TEST(PerftTest, wikifoura1) {
    std::string board_fen = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
    Board board = Board();
    board.fen_decode(board_fen);
    EXPECT_EQ(perft_bulk(1, board),  6);
}

TEST(PerftTest, wikifoura2) {
    std::string board_fen = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
    Board board = Board();
    board.fen_decode(board_fen);
    EXPECT_EQ(perft_bulk(2, board),  264);
}

TEST(PerftTest, wikifoura3) {
    std::string board_fen = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
    Board board = Board();
    board.fen_decode(board_fen);
    EXPECT_EQ(perft_bulk(3, board),  9467);
}

TEST(PerftTest, wikifoura4) {
    std::string board_fen = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
    Board board = Board();
    board.fen_decode(board_fen);
    EXPECT_EQ(perft_bulk(4, board),  422333);
}


TEST(PerftTest, wikifourb1) {
    std::string board_fen = "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1";
    Board board = Board();
    board.fen_decode(board_fen);
    EXPECT_EQ(perft_bulk(1, board),  6);
}

TEST(PerftTest, wikifourb2) {
    std::string board_fen = "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1";
    Board board = Board();
    board.fen_decode(board_fen);
    EXPECT_EQ(perft_bulk(2, board),  264);
}

TEST(PerftTest, wikifourb3) {
    std::string board_fen = "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1";
    Board board = Board();
    board.fen_decode(board_fen);
    EXPECT_EQ(perft_bulk(3, board),  9467);
}

TEST(PerftTest, wikifourb4) {
    std::string board_fen = "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1";
    Board board = Board();
    board.fen_decode(board_fen);
    EXPECT_EQ(perft_bulk(4, board),  422333);
}


TEST(PerftTest, wikifive1) {
    std::string board_fen = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8  ";
    Board board = Board();
    board.fen_decode(board_fen);
    EXPECT_EQ(perft_bulk(1, board),  44);
}

TEST(PerftTest, wikifive2) {
    std::string board_fen = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8  ";
    Board board = Board();
    board.fen_decode(board_fen);
    EXPECT_EQ(perft_bulk(2, board),   1486);
}

TEST(PerftTest, wikifive3) {
    std::string board_fen = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8  ";
    Board board = Board();
    board.fen_decode(board_fen);
    EXPECT_EQ(perft_bulk(3, board),   62379);
}

TEST(PerftTest, wikifive4) {
    std::string board_fen = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8  ";
    Board board = Board();
    board.fen_decode(board_fen);
    EXPECT_EQ(perft_bulk(4, board),  2103487);
}