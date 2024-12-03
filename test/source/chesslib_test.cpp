#include <deque>
#include <fstream>
#include <ankerl/unordered_dense.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_all.hpp>
#include <chesslib/chesslib.hpp>
#include <nlohmann/json.hpp>
#include <fmt/ranges.h>

#include <chesslib/core/types.hpp>
#include <chesslib/core/zobrist.hpp>
#include <chesslib/eval/evaluation.hpp>

using namespace chesslib::coord;

namespace chesslib::test {

namespace helpers {
auto print(board const& b, auto&& pred, char const* marker, fmt::color color) {
    auto const& pieces = b.pieces;
    auto const& colors = b.colors;
    auto constexpr n = coord::nrow;
    for (auto i = n - 1; i >= 0; --i) {
        for (auto j = 0; j < n; ++j) {
            auto const s = coord::square_index(i, j);
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
}


struct perft_result {
    u64 count{0};
    u64 capture{0};
    u64 promotion{0};
    u64 enpassant{0};
    u64 check{0};
    u64 mate{0};

    friend auto operator+(perft_result const& a, perft_result const& b) -> perft_result {
        return {
            .count = a.count + b.count,
            .capture = a.capture + b.capture,
            .promotion = a.promotion + b.promotion,
            .enpassant = a.enpassant + b.enpassant,
            .check = a.check + b.check,
            .mate = a.mate + b.mate
        };
    }
};


auto format_moves(board const& board, move_list const& moves) -> std::vector<std::string> {
    // group by target square since this changes the notation => Nec3, Nbc3
    ankerl::unordered_dense::map<std::pair<u8, u8>, std::vector<move>> map;

    std::vector<std::string> out;
    out.reserve(moves.size());

    for (auto const& m : moves) {
        auto [p, c] = board[m.source_square];
        auto [it, ok] = map.insert({ {static_cast<u8>(p), m.target_square}, {} });
        it->second.push_back(m);
    }

    std::string files = "abcdefgh";
    std::string ranks = "12345678";
    std::string promotions = "NBRQ";

    for (auto const& m : moves) {
        auto src = m.source_square;
        auto tgt = m.target_square;

        auto [p, c] = board[src];
        ASSERT(p != piece::none);

        std::string str;

        if (p == piece::pawn) {
            if (m.capture) {
                str.push_back(files[coord::file(src)]);
                str.push_back('x');
            }
            str += me::enum_name(static_cast<square>(tgt));
            if (m.promotion != 0) {
                str.push_back('=');
                str.push_back(piece_letters[0][m.promotion]);
            }
            out.push_back(str);
        } else if (m.castling) {
            if (tgt == square::g1 || tgt == square::g8) {
                str += "O-O";
            } else if (tgt == square::c1 || tgt == square::c8) {
                str += "O-O-O";
            } else {
                throw std::runtime_error("castling flag set but target is inconsistent");
            }
            out.push_back(str);
        } else {
            str.push_back(piece_letters[0][me::enum_integer(p)]);
            auto const& moves_to_tgt = map.find({static_cast<u8>(p), tgt})->second;
            if (moves_to_tgt.size() > 1) {
                if (std::ranges::count_if(moves_to_tgt, [&](auto const& m) {
                    return coord::same_file(m.source_square, src);
                }) > 1) {
                    str.push_back(ranks[coord::rank(m.source_square)]);
                } else {
                    str.push_back(files[coord::file(m.source_square)]);
                }
            }
            if (m.capture) {
                str.push_back('x');
            }
            str += me::enum_name(static_cast<square>(tgt));
            out.push_back(str);
        }
    }

    return out;
}


template<piece P>
static constexpr bool is_sliding_v = ((P == piece::bishop) || (P == piece::rook) || (P == piece::queen));
} // namespace helpers

TEST_CASE("piece values", "[library]")
{
    namespace me = magic_enum;

    me::enum_for_each<piece>([](auto val) {
        constexpr auto p = val();
        fmt::print("{}: {}\t{:08b}\n", me::enum_name(p), me::enum_integer(p), me::enum_integer(p));
    });
}

TEST_CASE("sliding", "[library]")
{
    ASSERT(helpers::is_sliding_v<piece::pawn>   == false);
    ASSERT(helpers::is_sliding_v<piece::knight> == false);
    ASSERT(helpers::is_sliding_v<piece::bishop> == true);
    ASSERT(helpers::is_sliding_v<piece::rook>   == true);
    ASSERT(helpers::is_sliding_v<piece::queen>  == true);
    ASSERT(helpers::is_sliding_v<piece::king>   == false);
}

TEST_CASE("fen export", "[library]")
{
    constexpr std::array test_cases = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w",
        "3rr3/1p1kbQ2/p1p1n3/q2p1Bp1/3P4/2N1P1P1/PP4P1/4RRK1 w",
        "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w",
        "8/7p/8/7k/2K1N3/6Q1/5B2/8 w",
        "8/8/8/8/3K4/8/8/8 w",
        "5rr1/8/8/8/8/8/q3PP2/R3K2R w",
        "5rr1/8/8/8/8/5p2/q3PP2/R3K2R w",
        "8/8/2r3n1/8/4B3/5N2/2R3P1/8 w",
        "8/8/2r3n1/8/4B3/5n2/2q3p1/8 w",
        "8/8/2r3n1/8/4B3/5n2/2q3p1/8 b",
        "5R2/k5r1/P2p4/1K1Np3/1P2P1p1/8/8/1r6 w",
        "5R2/k5r1/P2p4/1K1Np3/1P2P1p1/8/8/1r6 b",
        "8/4k3/8/8/8/8/r6r/4K3 w - - 0 1",
        "8/4k3/8/8/8/8/r6r/4K3 b - - 0 1",
        "8/8/8/8/8/8/8/4K2R",
        "8/8/8/3K4/8/4k3/8/8 w",
        "8/8/8/3K4/8/4k3/8/8 b",
        "8/4k3/8/8/8/8/r6r/R3K2R w KQ - 0 1"
    };
    CHECK(board{}.export_fen().substr(0, std::strlen(test_cases.front())) == test_cases.front());

    auto const* fen = GENERATE_REF(from_range(test_cases));
    CHECK(fen == board{fen}.export_fen().substr(0, std::strlen(fen)));
}

TEST_CASE("move generator", "[library]")
{
    auto check_test_cases = [](auto const& cases, auto piece)
    {
        for (auto fen : cases) {
            chesslib::board board{fen};
            chesslib::move_generator gen{board};
            chesslib::move_list moves;

            gen.moves(moves);
            ankerl::unordered_dense::map<u8, u8> squares;
            for (auto const& m : moves) {
                auto p = board.pieces[m.source_square];
                if (p != piece) { continue; }
                squares.insert({static_cast<u8>(m.target_square), static_cast<u8>(m.source_square)});
            }

            auto pred = [&](auto& b, auto s) {
                return squares.contains(s);
            };

            helpers::print(board, pred, "▢", fmt::color::blue);
            fmt::print("\n");
        }
    };

    SECTION("pawn moves")
    {
        constexpr std::array cases = {
            "8/8/8/8/3pP3/8/8/8 b e3"
        };

        check_test_cases(cases, piece::pawn);
    }

    SECTION("knight moves")
    {
        constexpr std::array cases = {
            "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR"
        };
        check_test_cases(cases, piece::knight);
    }

    SECTION("bishop moves")
    {
        constexpr std::array cases = {
            "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR",
            "8/8/8/3B4/8/8/8/8",
            "P5P1/8/8/3B4/8/5P2/P7/8",
        };
        check_test_cases(cases, piece::bishop);
    }

    SECTION("rook moves")
    {
        constexpr std::array cases = {
            "8/8/8/3R4/8/8/8/8",
            "8/4k3/8/8/8/8/r6r/R3K2R w KQ - 0 1"
        };
        check_test_cases(cases, piece::rook);
    }

    SECTION("queen moves")
    {
        constexpr std::array cases = {
            "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR",
            "8/8/8/3Q4/8/8/8/8",
            "3P4/1p3P2/8/P2Q2p1/8/1p3P2/3p4/8"
        };
        check_test_cases(cases, piece::queen);
    }

    SECTION("king moves")
    {
        constexpr std::array cases = {
            "8/8/8/8/8/8/8/4K2R",
            "8/8/8/3K4/8/4k3/8/8 w",
            "8/8/8/3K4/8/4k3/8/8 b",
            "8/4k3/8/8/8/8/r6r/R3K2R w KQ - 0 1"
        };
        check_test_cases(cases, piece::king);
    }
}

TEST_CASE("attacked squares", "[library]")
{
    auto attacked = [](auto& b, auto s) { return b.is_attacked(s, b.side()); };

    constexpr std::array test_cases = {
        "3rr3/1p1kbQ2/p1p1n3/q2p1Bp1/3P4/2N1P1P1/PP4P1/4RRK1 w",
        "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w",
        "8/7p/8/7k/2K1N3/6Q1/5B2/8 w",
        "8/8/8/8/3K4/8/8/8 w",
        "5rr1/8/8/8/8/8/q3PP2/R3K2R w",
        "5rr1/8/8/8/8/5p2/q3PP2/R3K2R w",
        "8/8/2r3n1/8/4B3/5N2/2R3P1/8 w",
        "8/8/2r3n1/8/4B3/5n2/2q3p1/8 w",
        "8/8/2r3n1/8/4B3/5n2/2q3p1/8 b",
        "5R2/k5r1/P2p4/1K1Np3/1P2P1p1/8/8/1r6 w",
        "5R2/k5r1/P2p4/1K1Np3/1P2P1p1/8/8/1r6 b",
        "8/4k3/8/8/8/8/r6r/4K3 w - - 0 1",
        "8/4k3/8/8/8/8/r6r/4K3 b - - 0 1"
    };

    for (auto const* fen : test_cases) {
        fmt::print("{}\n", fen);
        chesslib::board board{fen};
        helpers::print(board, attacked, "▢", board.side() == side_to_move::white ? fmt::color::red : fmt::color::blue);
        fmt::print("\n");
    }
}

TEST_CASE("parse en-passant", "[library]")
{
    constexpr auto* fen = "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1";
    chesslib::board board{fen};
    board.print();
}

TEST_CASE("king in check", "[library]")
{
    constexpr std::array test_cases = {
        "4k3/8/5P2/p1K4r/Pp5p/6pP/6P1/8 w - - 0 1"
    };

    for (auto const* fen : test_cases) {
        fmt::print("fen: {}\n", fen);
        board b{fen};
        b.print();
        fmt::print("king positions - white: {}, black: {}\n", me::enum_name(b.state().white_king), me::enum_name(b.state().black_king));
        REQUIRE(b.is_king_in_check());
    }
}

TEST_CASE("rampart", "[library]")
{
    using json = nlohmann::json;

    auto check_case = [&](auto t) {
        auto const& s  = t["start"];
        auto const fen = s["fen"].template get<std::string>();
        auto const& e  = t["expected"];
        std::vector<std::string> expected_moves;

        if (!e.empty()) {
            for (auto const& x : e) {
                auto m = x["move"].template get<std::string>();
                if (m.back() == '+' || m.back() == '#') {
                    m.pop_back();
                }
                expected_moves.push_back(m);
            }
        }
        std::ranges::sort(expected_moves);
        board b{fen};
        move_list moves;
        move_generator gen{b};
        gen.moves(moves);

        auto tmp = b;
        auto move_invalid = [&](auto const& m) { return move_maker{b, m}.check(); };
        moves.erase(std::remove_if(moves.begin(), moves.end(), move_invalid), moves.end());
        ASSERT(tmp == b);

        auto computed_moves = helpers::format_moves(b, moves);
        std::ranges::sort(computed_moves);
        auto color = expected_moves == computed_moves ? fmt::color::green : fmt::color::red;
        if (expected_moves != computed_moves) {
            fmt::print("fen: {}\n", fen);
            if (auto it = s.find("description"); it != s.end()) {
                fmt::print("description: {}\n", it->template get<std::string>());
            }
            b.print(); // print the board to see if the fen is parsed correctly
            fmt::print("expected moves: {} {}\n", expected_moves, expected_moves.size());
            fmt::print(fmt::fg(color), "computed moves: {} {}\n\n", computed_moves, computed_moves.size());
            fmt::print("\n");
        }
        REQUIRE(expected_moves == computed_moves);
    };

    // test cases involving castling
    SECTION("castling") {
        std::ifstream f("./test/testcases/castling.json");
        json data = json::parse(f);
        std::ranges::for_each(data["testCases"], check_case);
    }

    SECTION("checkmates") {
        std::ifstream f("./test/testcases/checkmates.json");
        json data = json::parse(f);
        std::ranges::for_each(data["testCases"], check_case);
    }

    SECTION("famous") {
        std::ifstream f("./test/testcases/famous.json");
        json data = json::parse(f);
        std::ranges::for_each(data["testCases"], check_case);
    }

    SECTION("pawns") {
        std::ifstream f("./test/testcases/pawns.json");
        json data = json::parse(f);
        std::ranges::for_each(data["testCases"], check_case);
    }

    SECTION("promotions") {
        std::ifstream f("./test/testcases/promotions.json");
        json data = json::parse(f);
        std::ranges::for_each(data["testCases"], check_case);
    }

    SECTION("stalemates") {
        std::ifstream f("./test/testcases/stalemates.json");
        json data = json::parse(f);
        std::ranges::for_each(data["testCases"], check_case);
    }

    SECTION("standard") {
        std::ifstream f("./test/testcases/standard.json");
        json data = json::parse(f);
        std::ranges::for_each(data["testCases"], check_case);
    }

    SECTION("taxing") {
        std::ifstream f("./test/testcases/taxing.json");
        json data = json::parse(f);
        std::ranges::for_each(data["testCases"], check_case);
    }
}

TEST_CASE("zobrist", "[library]")
{
    auto const* fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w";
    chesslib::board b{fen};
    zobrist::hasher hasher{};

    auto check_move = [&](auto const m) {
        // what is the diff between current board and previous board?
        // how can I use this diff to incrementally update the zobrist hash?
        auto const src = m.source_square;
        auto const tgt = m.target_square;

        auto const [p, c] = b[src];
        auto const i = static_cast<int>(c);
        auto const j = static_cast<int>(p);

        auto h1 = hasher(b);
        h1 ^= zobrist::hasher::ptable(i, j, src); // taking the piece from its source square
        h1 ^= zobrist::hasher::ptable(i, j, tgt); // putting the piece on its destination square

        auto const tmp = b; // make a copy of my board

        move_maker mm{b, m};
        mm.make();

        board::diff(tmp, b);

        INFO("Check intermediate state (move made, incremental hash update)");
        REQUIRE(h1 == hasher(b));

        mm.undo();

        // XOR again to reach previous board hash
        h1 ^= zobrist::hasher::ptable(i, j, src);
        h1 ^= zobrist::hasher::ptable(i, j, tgt);

        INFO("Check end state (move made and undone, 2xincremental hash update");
        REQUIRE(h1 == hasher(b));
    };

    SECTION("a2a4") {
        move m{
            .source_square = square::a2,
            .target_square = square::a4,
            .promotion     = 0,
            .capture       = 0,
            .double_pawn   = 0,
            .enpassant     = 0,
            .castling      = 0
        };
        check_move(m);
    }

    SECTION("a2a3") {
        move m{
            .source_square = square::a2,
            .target_square = square::a3,
            .promotion     = 0,
            .capture       = 0,
            .double_pawn   = 0,
            .enpassant     = 0,
            .castling      = 0
        };
        check_move(m);
    }
}

} // namespace chesslib::test
