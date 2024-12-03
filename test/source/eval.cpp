#include <catch2/catch_test_macros.hpp>
#include <chesslib/eval/piece_square_tables.hpp>
#include <chesslib/core/types.hpp>
#include <fmt/core.h>
#include "chesslib/board/board.hpp"
#include "chesslib/eval/evaluation.hpp"

namespace me = magic_enum;

namespace chesslib {

namespace {
auto print_piece_table(u8 p, u8 c) {
    auto cname = me::enum_name<color>(static_cast<color>(c));
    auto pname = me::enum_name<piece>(static_cast<piece>(p));

    fmt::print("{} {}\n", cname, pname);
    for (auto i = 0; i < valid_squares.size(); ++i) {
        fmt::print("{:#4d} ", evaluation::tables::midgame_table[c][p][i]);
        if ((i+1) % 8 == 0) {
            fmt::print("\n");
        }
    }
    fmt::print("\n\n");
}
} // namespace

TEST_CASE("piece square tables") {
    for (auto const c : {0, 1}) {
        for (auto p = me::enum_integer(piece::pawn); p <= me::enum_integer(piece::king); ++p) {
            print_piece_table(p, c);
        }
    }

}

TEST_CASE("board evaluation") {
    SECTION("initial position") {
        chesslib::board b;
        b.print();
        fmt::print("eval: {}\n", evaluate(b));
    }
}

} // namespace chesslib