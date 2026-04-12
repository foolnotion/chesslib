#include <ankerl/unordered_dense.h>
#include <catch2/catch_test_macros.hpp>
#include <chesslib/chesslib.hpp>
#include <nlohmann/json.hpp>
#include <fmt/ranges.h>
#include <libassert/assert.hpp>

namespace chesslib::test {
    TEST_CASE("fen parser", "[library]")
    {
        SECTION("8/4k3/8/8/8/8/r6r/R3K2R w - - 0 1") {
            board b{"8/4k3/8/8/8/8/r6r/R3K2R w - - 0 1"};
            ASSERT(me::enum_integer(b.castling()) == 0);
        }

        SECTION("strict parser requires both kings") {
            auto result = fen::read("8/8/8/8/3K4/8/8/8 w - - 0 1");
            REQUIRE(!result);
            REQUIRE(result.error().reason == fen::error::invalid_piece_placement);
        }
    }
} // namespace chesslib::test