#include <chesslib/chesslib.hpp>
#include <catch2/catch_test_macros.hpp>
#include <ankerl/unordered_dense.h>

namespace chesslib::test {
namespace helpers {
auto print(board const& b, auto&& pred, char const* marker, fmt::color color) {
        auto const& pieces = b.pieces;
        auto const& colors = b.colors;
        auto constexpr n = coord::nrow;
        for (auto i = n - 1; i >= 0; --i) {
            for (auto j = 0; j < n; ++j) {
                auto const s = coord::square(i, j);
                auto const p = me::enum_integer(pieces[s]);
                auto const* c = piece_symbols[colors[s] == color::white ? 0 : 1][p];
                auto fg = fmt::color::white;
                if (pred(b, s)) {
                    if (c == std::string{" "}) { c = marker; }
                    fg = color;
                }

                fmt::print(fmt::bg((coord::file(s) + coord::rank(s)) % 2 == 0
                                        ? fmt::color::dim_gray
                                        : fmt::color::gray) | fmt::fg(fg), "{} ", c);
            }
            fmt::print("\n");
        }
    };
} // namespace helpers

TEST_CASE("print board", "[library]")
{
    chesslib::board board;
    board.import_fen("rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR");
    board.print();
    board::print_binary();
}

TEST_CASE("piece values", "[library]")
{
    namespace me = magic_enum;

    me::enum_for_each<piece>([](auto val) {
        constexpr auto p = val();
        fmt::print("{}: {}\n", me::enum_name(p), me::enum_integer(p));
    });
}

TEST_CASE("pawn moves", "[library]")
{
    SECTION("rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR") {
        chesslib::board board{"rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR"};
        chesslib::move_generator gen{board};
        chesslib::move_list moves;

        gen.moves(moves);
        ankerl::unordered_dense::map<u8, u8> squares;
        for (auto const& m : moves) {
            auto p = board.pieces[m.source_square];
            if (p != piece::pawn) { continue; }
            squares.insert({static_cast<u8>(m.target_square), static_cast<u8>(m.source_square)});
        }

        auto pred = [&](auto& b, auto s) {
            return squares.contains(s);
        };

        helpers::print(board, pred, "▢", fmt::color::blue);
        fmt::print("\n");
    }
}

TEST_CASE("knight moves", "[library]")
{
    SECTION("rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR") {
        chesslib::board board{"rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR"};
        chesslib::move_generator gen{board};
        chesslib::move_list moves;

        gen.moves(moves);
        ankerl::unordered_dense::map<u8, u8> squares;
        for (auto const& m : moves) {
            auto p = board.pieces[m.source_square];
            if (p != piece::knight) { continue; }
            squares.insert({static_cast<u8>(m.target_square), static_cast<u8>(m.source_square)});
        }

        auto pred = [&](auto& b, auto s) {
            return squares.contains(s);
        };

        helpers::print(board, pred, "▢", fmt::color::blue);
        fmt::print("\n");
    }
}

TEST_CASE("bishop moves", "[library]")
{
    SECTION("rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR") {
        chesslib::board board{"rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR"};
        chesslib::move_generator gen{board};
        chesslib::move_list moves;

        gen.moves(moves);
        ankerl::unordered_dense::map<u8, u8> squares;
        for (auto const& m : moves) {
            auto p = board.pieces[m.source_square];
            if (p != piece::bishop) { continue; }
            squares.insert({static_cast<u8>(m.target_square), static_cast<u8>(m.source_square)});
        }

        auto pred = [&](auto& b, auto s) {
            return squares.contains(s);
        };

        helpers::print(board, pred, "▢", fmt::color::blue);
        fmt::print("\n");
    }

    SECTION("8/8/8/3B4/8/8/8/8") {
        chesslib::board board{"8/8/8/3B4/8/8/8/8"};
        chesslib::move_generator gen{board};
        chesslib::move_list moves;

        gen.moves(moves);
        ankerl::unordered_dense::map<u8, u8> squares;
        for (auto const& m : moves) {
            auto p = board.pieces[m.source_square];
            if (p != piece::bishop) { continue; }
            squares.insert({static_cast<u8>(m.target_square), static_cast<u8>(m.source_square)});
        }

        auto pred = [&](auto& b, auto s) {
            return squares.contains(s);
        };

        helpers::print(board, pred, "▢", fmt::color::blue);
        fmt::print("\n");
    }

    SECTION("P5P1/8/8/3B4/8/5P2/P7/8") {
        chesslib::board board{"P5P1/8/8/3B4/8/5P2/P7/8"};
        chesslib::move_generator gen{board};
        chesslib::move_list moves;

        gen.moves(moves);
        ankerl::unordered_dense::map<u8, u8> squares;
        for (auto const& m : moves) {
            auto p = board.pieces[m.source_square];
            if (p != piece::bishop) { continue; }
            squares.insert({static_cast<u8>(m.target_square), static_cast<u8>(m.source_square)});
        }

        auto pred = [&](auto& b, auto s) {
            return squares.contains(s);
        };

        helpers::print(board, pred, "▢", fmt::color::blue);
        fmt::print("\n");
    }
}

TEST_CASE("rook moves", "[library]")
{
    SECTION("8/8/8/3R4/8/8/8/8") {
        chesslib::board board{"8/8/8/3R4/8/8/8/8"};
        chesslib::move_generator gen{board};
        chesslib::move_list moves;

        gen.moves(moves);
        ankerl::unordered_dense::map<u8, u8> squares;
        for (auto const& m : moves) {
            auto p = board.pieces[m.source_square];
            if (p != piece::rook) { continue; }
            squares.insert({static_cast<u8>(m.target_square), static_cast<u8>(m.source_square)});
        }

        auto pred = [&](auto& b, auto s) {
            return squares.contains(s);
        };

        helpers::print(board, pred, "▢", fmt::color::blue);
        fmt::print("\n");
    }
}

TEST_CASE("queen moves", "[library]")
{
    SECTION("rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR") {
        chesslib::board board{"rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR"};
        chesslib::move_generator gen{board};
        chesslib::move_list moves;

        gen.moves(moves);
        ankerl::unordered_dense::map<u8, u8> squares;
        for (auto const& m : moves) {
            auto p = board.pieces[m.source_square];
            if (p != piece::queen) { continue; }
            squares.insert({static_cast<u8>(m.target_square), static_cast<u8>(m.source_square)});
        }

        auto pred = [&](auto& b, auto s) {
            return squares.contains(s);
        };

        helpers::print(board, pred, "▢", fmt::color::blue);
        fmt::print("\n");
    }

    SECTION("8/8/8/3Q4/8/8/8/8") {
        chesslib::board board{"8/8/8/3Q4/8/8/8/8"};
        chesslib::move_generator gen{board};
        chesslib::move_list moves;

        gen.moves(moves);
        ankerl::unordered_dense::map<u8, u8> squares;
        for (auto const& m : moves) {
            auto p = board.pieces[m.source_square];
            if (p != piece::queen) { continue; }
            squares.insert({static_cast<u8>(m.target_square), static_cast<u8>(m.source_square)});
        }

        auto pred = [&](auto& b, auto s) {
            return squares.contains(s);
        };

        helpers::print(board, pred, "▢", fmt::color::blue);
        fmt::print("\n");
    }

    SECTION("3P4/1P3P2/8/P2Q2P1/8/1P3P2/3P4/8") {
        chesslib::board board{"3P4/1p3P2/8/P2Q2p1/8/1p3P2/3p4/8"};
        chesslib::move_generator gen{board};
        chesslib::move_list moves;

        gen.moves(moves);
        ankerl::unordered_dense::map<u8, u8> squares;
        for (auto const& m : moves) {
            auto p = board.pieces[m.source_square];
            if (p != piece::queen) { continue; }
            squares.insert({static_cast<u8>(m.target_square), static_cast<u8>(m.source_square)});
        }

        auto pred = [&](auto& b, auto s) {
            return squares.contains(s);
        };

        helpers::print(board, pred, "▢", fmt::color::blue);
        fmt::print("\n");
    }
}

TEST_CASE("king moves", "[library]")
{
    SECTION("8/8/8/8/8/8/8/4K2R") {
        chesslib::board board{"8/8/8/8/8/8/8/4K2R"};
        chesslib::move_generator gen{board};
        chesslib::move_list moves;

        gen.moves(moves);
        ankerl::unordered_dense::map<u8, u8> squares;
        for (auto const& m : moves) {
            auto p = board.pieces[m.source_square];
            if (p != piece::king) { continue; }
            squares.insert({static_cast<u8>(m.target_square), static_cast<u8>(m.source_square)});
        }

        auto pred = [&](auto& b, auto s) {
            return squares.contains(s);
        };

        helpers::print(board, pred, "▢", fmt::color::blue);
        fmt::print("\n");
    }

    SECTION("8/8/8/3K4/8/4K3/8/8") {
        chesslib::move_list moves;

        chesslib::board board{"8/8/8/3K4/8/4k3/8/8"};
        board.side() = side::white;
        chesslib::move_generator gen{board};
        fmt::print("side to move: {}\n", me::enum_name(board.side()));
        gen.moves(moves);

        ankerl::unordered_dense::map<u8, u8> squares;
        auto pred = [&](auto& b, auto s) {
            return squares.contains(s);
        };

        for (auto const& m : moves) {
            auto p = board.pieces[m.source_square];
            if (p != piece::king) { continue; }
            squares.insert({static_cast<u8>(m.target_square), static_cast<u8>(m.source_square)});
        }
        helpers::print(board, pred, "▢", fmt::color::blue);
        fmt::print("\n");

        board.side() = side::black;
        fmt::print("side to move: {}\n", me::enum_name(board.side()));
        gen.moves(moves);
        squares.clear();
        for (auto const& m : moves) {
            auto p = board.pieces[m.source_square];
            if (p != piece::king) { continue; }
            squares.insert({static_cast<u8>(m.target_square), static_cast<u8>(m.source_square)});
        }
        helpers::print(board, pred, "▢", fmt::color::blue);
        fmt::print("\n");
    }
}

TEST_CASE("attacked squares", "[library]")
{
    auto pred = [](auto& b, auto s) { return b.is_attacked(s, side::white); };

    SECTION("3rr3/1p1kbQ2/p1p1n3/q2p1Bp1/3P4/2N1P1P1/PP4P1/4RRK1") {
        chesslib::board board{"3rr3/1p1kbQ2/p1p1n3/q2p1Bp1/3P4/2N1P1P1/PP4P1/4RRK1"};
        helpers::print(board, pred, "▢", fmt::color::red);
        fmt::print("\n");
    }

    SECTION("rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR") {
        chesslib::board board{"rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR"};
        helpers::print(board, pred, "▢", fmt::color::red);
        fmt::print("\n");
    }

    SECTION("8/7p/8/7k/2K1N3/6Q1/5B2/8") {
        chesslib::board board{"8/7p/8/7k/2K1N3/6Q1/5B2/8"};
        helpers::print(board, pred, "▢", fmt::color::red);
        fmt::print("\n");
    }

    SECTION("8/8/8/8/3K4/8/8/8") {
        chesslib::board board{"8/8/8/8/3K4/8/8/8"};
        helpers::print(board, pred, "▢", fmt::color::red);
        fmt::print("\n");
    }
}
} // namespace chesslib::test