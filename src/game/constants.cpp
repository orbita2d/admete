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
    psqt_t PSQRT;
    PSQRT[PAWN] = {
        Score(6, 8),     Score(30, 108), Score(-28, 64),  Score(26, 34),    Score(22, -4),   Score(16, -4),
        Score(0, 14),    Score(8, -2),   Score(-2, 160),  Score(-22, 148),  Score(-18, 150), Score(6, 128),
        Score(-26, 100), Score(2, 128),  Score(-42, 124), Score(-150, 138), Score(-17, 63),  Score(-1, 73),
        Score(34, 65),   Score(58, 37),  Score(74, 55),   Score(86, 39),    Score(63, 63),   Score(3, 61),
        Score(-28, 46),  Score(-7, 44),  Score(-12, 32),  Score(9, 24),     Score(29, 24),   Score(16, 30),
        Score(3, 34),    Score(-20, 34), Score(-36, 30),  Score(-26, 38),   Score(-17, 20),  Score(18, 22),
        Score(12, 28),   Score(-7, 26),  Score(-16, 30),  Score(-38, 20),   Score(-40, 30),  Score(-30, 32),
        Score(-24, 24),  Score(-8, 28),  Score(0, 36),    Score(-20, 32),   Score(-6, 26),   Score(-26, 18),
        Score(-30, 32),  Score(-18, 36), Score(-18, 28),  Score(-16, 18),   Score(-10, 58),  Score(-4, 42),
        Score(6, 32),    Score(-26, 22), Score(-2, 0),    Score(-6, -2),    Score(-2, 4),    Score(-12, -8),
        Score(-6, -2),   Score(4, 4),    Score(2, 12),    Score(2, -2),
    };
    PSQRT[KNIGHT] = {
        Score(-114, -30), Score(-34, 16),  Score(-4, 62),  Score(-10, 24), Score(42, 56),  Score(-118, 64),
        Score(14, -2),    Score(-80, -30), Score(48, 10),  Score(22, 38),  Score(158, 16), Score(96, 46),
        Score(144, 52),   Score(164, 28),  Score(68, 24),  Score(82, 18),  Score(40, 36),  Score(58, 40),
        Score(66, 62),    Score(98, 64),   Score(130, 48), Score(184, 46), Score(102, 36), Score(86, 32),
        Score(42, 50),    Score(20, 66),   Score(48, 70),  Score(94, 82),  Score(48, 80),  Score(88, 70),
        Score(24, 74),    Score(68, 48),   Score(2, 50),   Score(6, 52),   Score(34, 72),  Score(24, 82),
        Score(48, 76),    Score(44, 68),   Score(40, 60),  Score(10, 46),  Score(-30, 38), Score(8, 40),
        Score(24, 44),    Score(28, 56),   Score(46, 54),  Score(30, 44),  Score(28, 38),  Score(-22, 58),
        Score(-36, 36),   Score(-24, 48),  Score(6, 22),   Score(30, 40),  Score(26, 40),  Score(28, 34),
        Score(2, 46),     Score(6, 42),    Score(-86, 20), Score(0, 20),   Score(-20, 40), Score(0, 52),
        Score(0, 30),     Score(6, 20),    Score(-6, 32),  Score(-68, -4),
    };
    PSQRT[BISHOP] = {
        Score(22, 20), Score(-26, 20), Score(-48, -8), Score(-116, 24), Score(-36, 6),  Score(-90, 12), Score(-34, 4),
        Score(20, 12), Score(60, 6),   Score(66, -8),  Score(36, 16),   Score(-26, 14), Score(30, -2),  Score(52, -4),
        Score(78, 18), Score(54, -14), Score(40, 8),   Score(68, 6),    Score(76, -10), Score(72, 4),   Score(74, 0),
        Score(96, 12), Score(70, -6),  Score(50, 34),  Score(20, 18),   Score(28, 0),   Score(52, 18),  Score(72, 10),
        Score(70, 6),  Score(62, -2),  Score(26, 26),  Score(22, -4),   Score(12, 2),   Score(24, 4),   Score(24, 18),
        Score(70, 26), Score(56, 2),   Score(24, 26),  Score(30, 4),    Score(32, -16), Score(16, 16),  Score(40, 6),
        Score(36, 34), Score(36, 20),  Score(40, 42),  Score(36, 6),    Score(40, 14),  Score(32, 0),   Score(36, -8),
        Score(44, 12), Score(48, -2),  Score(26, 14),  Score(32, 4),    Score(52, 6),   Score(58, -6),  Score(40, 6),
        Score(6, 20),  Score(40, 0),   Score(24, 12),  Score(8, -8),    Score(10, -8),  Score(12, 12),  Score(2, -10),
        Score(4, 10),
    };
    PSQRT[ROOK] = {
        Score(52, 68),  Score(50, 82),  Score(44, 84),  Score(56, 94),  Score(58, 88),  Score(76, 94),  Score(54, 100),
        Score(84, 90),  Score(26, 92),  Score(20, 106), Score(40, 110), Score(58, 108), Score(58, 96),  Score(94, 108),
        Score(78, 114), Score(104, 92), Score(14, 92),  Score(34, 102), Score(38, 100), Score(58, 94),  Score(54, 98),
        Score(82, 94),  Score(126, 96), Score(78, 80),  Score(-14, 98), Score(-10, 94), Score(8, 104),  Score(14, 96),
        Score(12, 90),  Score(10, 96),  Score(46, 82),  Score(52, 74),  Score(-28, 90), Score(-32, 80), Score(-32, 90),
        Score(-16, 82), Score(-22, 84), Score(-32, 92), Score(-6, 90),  Score(-12, 78), Score(-48, 84), Score(-30, 76),
        Score(-26, 78), Score(-32, 78), Score(-20, 70), Score(-22, 72), Score(16, 64),  Score(-14, 72), Score(-40, 58),
        Score(-38, 68), Score(-18, 78), Score(-18, 76), Score(-6, 74),  Score(4, 70),   Score(10, 50),  Score(-24, 58),
        Score(-22, 78), Score(-12, 74), Score(2, 72),   Score(10, 74),  Score(10, 72),  Score(2, 80),   Score(10, 68),
        Score(-10, 74),
    };
    PSQRT[QUEEN] = {
        Score(24, 184),  Score(46, 162),  Score(74, 198),  Score(-34, 230), Score(74, 210),  Score(148, 180),
        Score(134, 172), Score(114, 172), Score(34, 176),  Score(-10, 190), Score(2, 238),   Score(-4, 262),
        Score(0, 250),   Score(100, 224), Score(46, 236),  Score(122, 188), Score(16, 152),  Score(2, 194),
        Score(46, 164),  Score(32, 190),  Score(72, 218),  Score(98, 216),  Score(110, 230), Score(74, 238),
        Score(4, 156),   Score(-12, 192), Score(8, 186),   Score(4, 214),   Score(16, 226),  Score(26, 240),
        Score(12, 272),  Score(32, 258),  Score(-4, 166),  Score(-12, 176), Score(-14, 208), Score(-4, 194),
        Score(-2, 210),  Score(-8, 220),  Score(18, 196),  Score(10, 206),  Score(-14, 162), Score(2, 172),
        Score(-6, 190),  Score(-4, 182),  Score(-4, 180),  Score(2, 206),   Score(14, 160),  Score(12, 172),
        Score(-16, 140), Score(0, 132),   Score(14, 168),  Score(22, 150),  Score(16, 168),  Score(26, 138),
        Score(24, 124),  Score(34, 106),  Score(-8, 150),  Score(-6, 134),  Score(6, 126),   Score(30, 156),
        Score(18, 142),  Score(-4, 106),  Score(-10, 132), Score(10, 148),
    };
    PSQRT[KING] = {
        Score(-2, -60),  Score(-24, -4),  Score(-18, -6),   Score(-20, -2),  Score(-26, 12),   Score(-10, 4),
        Score(-14, 4),   Score(-16, -68), Score(-12, -10),  Score(-36, 70),  Score(-32, 44),   Score(-12, 50),
        Score(-26, 58),  Score(-24, 70),  Score(-16, 84),   Score(-12, 12),  Score(-16, 0),    Score(-30, 54),
        Score(-52, 52),  Score(-30, 56),  Score(-28, 52),   Score(-22, 56),  Score(-20, 46),   Score(-26, 10),
        Score(-156, 6),  Score(-118, 28), Score(-152, 42),  Score(-214, 50), Score(-194, 50),  Score(-128, 48),
        Score(-86, 30),  Score(-98, -10), Score(-150, -28), Score(-154, 16), Score(-202, 24),  Score(-196, 46),
        Score(-200, 38), Score(-190, 26), Score(-150, 10),  Score(-206, -6), Score(-106, -12), Score(-82, 0),
        Score(-110, 14), Score(-134, 16), Score(-140, 22),  Score(-120, 16), Score(-86, 2),    Score(-96, -10),
        Score(-40, -16), Score(-30, 6),   Score(-26, -4),   Score(-62, 4),   Score(-60, 4),    Score(-36, -2),
        Score(-10, -4),  Score(-26, -22), Score(-34, -22),  Score(38, -28),  Score(36, -10),   Score(-34, -32),
        Score(30, -44),  Score(-36, -26), Score(32, -30),   Score(12, -50),
    };
    for (unsigned int i = 0; i < 2; i++) {
        for (unsigned int j = 0; j < 2; j++) {
            for (unsigned int sq = 0; sq < N_SQUARE; sq++) {
                if (Square(sq).rank() != RANK8 && Square(sq).rank() != RANK1) {
                    SPSQT[i][j][PAWN][sq] = Score(100, 100) + PSQRT[PAWN][sq];
                } else {
                    SPSQT[i][j][PAWN][sq] = Score(0, 0);
                }
                SPSQT[i][j][KNIGHT][sq] = Score(300, 300) + PSQRT[KNIGHT][sq];
                SPSQT[i][j][BISHOP][sq] = Score(330, 350) + PSQRT[BISHOP][sq];
                SPSQT[i][j][ROOK][sq] = Score(500, 500) + PSQRT[ROOK][sq];
                SPSQT[i][j][QUEEN][sq] = Score(900, 900) + PSQRT[QUEEN][sq];
                SPSQT[i][j][KING][sq] = PSQRT[KING][sq];
            }
        }
    }
}