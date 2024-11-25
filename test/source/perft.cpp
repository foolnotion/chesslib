#include <chesslib/chesslib.hpp>
#include <chesslib/util/perft.hpp>
#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <ranges>

TEST_CASE("perft", "[library]") {
    /* perft test cases, see:
     * https://www.chessprogramming.org/Perft_Results
     * https://oeis.org/A048987
    */

    constexpr std::array test_cases = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",                 // initial position
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -",         // position 2
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -",                                    // position 3
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",         // position 4
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",                // position 5
        "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", // position 6
    };

    constexpr std::array test_results = {
        // initial position
        std::array{
            chesslib::perft_result{20, 0, 0, 0, 0, 0, 0},
            chesslib::perft_result{400, 0, 0, 0, 0, 0, 0},
            chesslib::perft_result{8902, 34, 0, 0, 0, 12, 0},
            chesslib::perft_result{197281, 1576, 0, 0, 0, 469, 8},
            chesslib::perft_result{4865609, 82719, 258, 0, 0, 27351, 347},
            chesslib::perft_result{119060324, 2812008, 5248, 0, 0, 809099, 10828}
        },
        // position 2
        std::array{
            chesslib::perft_result{48, 8, 0, 2, 0, 0, 0},
            chesslib::perft_result{2039, 351, 1, 91, 0, 3, 0},
            chesslib::perft_result{97862, 17102, 45, 3162, 0, 993, 1},
            chesslib::perft_result{4085603, 757163, 1929, 128013, 15172, 25523, 43},
            chesslib::perft_result{193690690, 35043416, 73365, 4993637, 8392, 3309887, 30171},
            chesslib::perft_result{8031647685, 1558445089, 3577504, 184513607, 56627920, 92238050, 360003}
        },
        // position 3
        std::array{
            chesslib::perft_result{14, 1, 0, 0, 0, 2, 0},
            chesslib::perft_result{191, 14, 0, 0, 0, 10, 0},
            chesslib::perft_result{2812, 209, 2, 0, 0, 267, 0},
            chesslib::perft_result{43238, 3348, 123, 0, 0, 1680, 17},
            chesslib::perft_result{674624, 52051, 1165, 0, 0, 52950, 0},
            chesslib::perft_result{11030083, 940350, 33325, 0, 7552, 452473, 2733}
        },
        // position 4
        std::array{
            chesslib::perft_result{6, 0, 0, 0, 0, 0, 0},
            chesslib::perft_result{264, 87, 0, 6, 48, 10, 0},
            chesslib::perft_result{9467, 1021, 4, 0, 120, 38, 22},
            chesslib::perft_result{422333, 131393, 0, 7795, 60032, 15492, 5},
            chesslib::perft_result{15833292, 2046173, 6512, 0, 329464, 200568, 50562},
            chesslib::perft_result{706045033, 210369132, 212, 10882006, 81102984, 26973664, 81076}
        },
        // position 5
        std::array{
            chesslib::perft_result{44, 6, 0, 1, 4, 0, 0},
            chesslib::perft_result{1486, 222, 0, 0, 0, 117, 0},
            chesslib::perft_result{62379, 8517, 0, 1081, 5068, 1201, 44},
            chesslib::perft_result{2103487, 296153, 0, 0, 0, 158486, 240},
            chesslib::perft_result{89941194, 12320378, 140, 1240828, 6655216, 3078299, 137306},
            chesslib::perft_result{3048196529, 416546031, 10815, 0, 0, 225699166, 759812}
        },
        // position 6
        std::array{
            chesslib::perft_result{46, 4, 0, 0, 0, 1, 0},
            chesslib::perft_result{2079, 203, 0, 0, 0, 40, 0},
            chesslib::perft_result{89890, 9470, 0 ,0, 0, 1783, 0},
            chesslib::perft_result{3894594, 440388, 0, 0, 0, 68985, 0},
            chesslib::perft_result{164075551, 19528068, 122, 0, 0, 2998608, 228},
            chesslib::perft_result{6923051137, 868499408, 5167, 0, 0, 115530135, 10070}
        },
    };

    auto check = [&](int i) {
        INFO(fmt::format("fen: {}\n", test_cases[i]));
        auto const& result = test_results[i];
        for (auto d = 1; d <= result.size(); ++d) {
            INFO(fmt::format("position {} depth {}\n", i+1, d));
            REQUIRE(chesslib::perft_debug(test_cases[i], d) == result[d-1]);
        }
    };

    SECTION("position 1") { check(0); }
    SECTION("position 2") { check(1); }
    SECTION("position 3") { check(2); }
    SECTION("position 4") { check(3); }
    SECTION("position 5") { check(4); }
    SECTION("position 6") { check(5); }

    SECTION("vajolet positions") {
        // from the test suite of the chess engine vajolet:
        // https://github.com/elcabesa/vajolet/blob/master/tests/perft.txt
        std::ifstream f("./test/testcases/vajolet-perft.txt");
        for (std::string line; std::getline(f, line);) {
            auto i = 0;
            std::string fen;
            for (auto field : std::views::split(line, ',')) {
                std::string sv{field.begin(), field.end()};
                if (i == 0) {
                    fen = sv;
                } else {
                    INFO(fmt::format("perft({}, {}) == {}\n", fen, i, sv));
                    REQUIRE(chesslib::perft(fen, i) == std::stoll(sv));
                }
                ++i;
            }
        }
    }
}