#include <chesslib/chesslib.hpp>
#include <chesslib/board/encoding.hpp>
#include <chesslib/board/iterator.hpp>

#include <catch2/catch_test_macros.hpp>

namespace chesslib::test {

TEST_CASE("Name is chesslib", "[library]")
{
    REQUIRE(chesslib::name() == "chesslib");
}

TEST_CASE("Print board", "[library]")
{
    chesslib::board board;
    board.import_fen("rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR");
    board.print();
    board::print_binary();
}

TEST_CASE("Piece values", "[library]")
{
    namespace me = magic_enum;

    me::enum_for_each<piece>([](auto val) {
        constexpr auto p = val();
        fmt::print("{}: {}\n", me::enum_name(p), me::enum_integer(p));
    });
}

} // namespace chesslib::test
