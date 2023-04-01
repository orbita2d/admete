#include "evaluate.hpp"

void Evaluation::constants() {
    pb_passed = {
        Score(0, 0),    Score(0, 0),     Score(0, 0),     Score(0, 0),     Score(0, 0),     Score(0, 0),
        Score(0, 0),    Score(0, 0),     Score(124, 154), Score(120, 164), Score(101, 152), Score(104, 156),
        Score(54, 169), Score(107, 158), Score(97, 165),  Score(108, 162), Score(93, 132),  Score(109, 123),
        Score(54, 100), Score(34, 93),   Score(21, 77),   Score(32, 117),  Score(2, 110),   Score(0, 118),
        Score(64, 100), Score(40, 82),   Score(45, 80),   Score(33, 64),   Score(9, 58),    Score(12, 67),
        Score(-15, 91), Score(25, 92),   Score(27, 67),   Score(-1, 46),   Score(-16, 56),  Score(-13, 34),
        Score(-25, 31), Score(-39, 42),  Score(-22, 62),  Score(24, 66),   Score(14, 8),    Score(-39, 7),
        Score(-25, 9),  Score(-34, 6),   Score(-33, 9),   Score(-58, 10),  Score(-78, 18),  Score(-8, 25),
        Score(-14, 9),  Score(-48, -13), Score(-35, 7),   Score(-50, -8),  Score(-18, -2),  Score(-37, 0),
        Score(-56, -3), Score(-8, 11),   Score(0, 0),     Score(0, 0),     Score(0, 0),     Score(0, 0),
        Score(0, 0),    Score(0, 0),     Score(0, 0),     Score(0, 0),
    };
    for (unsigned int i = 0; i < 2; i++) {
        for (unsigned int j = 0; j < 2; j++) {
            for (unsigned int sq = 0; sq < N_SQUARE; sq++) {
                SPSQT[i][j][PAWN][sq] = Score(100, 100);
                SPSQT[i][j][KNIGHT][sq] = Score(300, 300);
                SPSQT[i][j][BISHOP][sq] = Score(300, 300);
                SPSQT[i][j][ROOK][sq] = Score(500, 500);
                SPSQT[i][j][QUEEN][sq] = Score(900, 900);
            }
        }
    }
}