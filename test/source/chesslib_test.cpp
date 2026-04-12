#include <deque>
#include <fstream>
#include <ankerl/unordered_dense.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_all.hpp>
#include <chesslib/chesslib.hpp>
#include <nlohmann/json.hpp>
#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <libassert/assert.hpp>

#include <chesslib/core/types.hpp>
#include <chesslib/core/zobrist.hpp>

using namespace chesslib::coord;

namespace chesslib::test {

namespace helpers {

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

template<piece P>
static constexpr bool is_sliding_v = ((P == piece::bishop) || (P == piece::rook) || (P == piece::queen));

namespace {
auto print(board const& b, auto&& pred, char const* marker, fmt::color color) {
    auto constexpr n = coord::nrow;
    for (auto i = n - 1; i >= 0; --i) {
        for (auto j = 0; j < n; ++j) {
            auto const s = coord::square_index(i, j);
            auto const p = me::enum_integer(b.piece_at(s));
            auto const* c = piece_symbols[b.color_at(s) == color::white ? 0 : 1][p];
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

    for (auto const& m : moves) {
        auto src = m.source_square;
        auto tgt = m.target_square;

        auto [p, c] = board[src];
        ASSERT(p != piece::none);

        std::string str;

        if (p == piece::pawn) {
            if (m.capture) {
                str.push_back(files[static_cast<size_t>(coord::file(src))]);
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
                    str.push_back(ranks[static_cast<size_t>(coord::rank(m.source_square))]);
                } else {
                    str.push_back(files[static_cast<size_t>(coord::file(m.source_square))]);
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


} // namespace (anonymous)
} // namespace helpers

TEST_CASE("piece values", "[library]")
{
    namespace me = magic_enum;

    me::enum_for_each<piece>([](auto val) {
        constexpr auto p = val();
        fmt::print("{}: {}\t{:08b}\n", me::enum_name(p), me::enum_integer(p), me::enum_integer(p));
    });

    REQUIRE(me::enum_integer(piece::pawn)   == 0);
    REQUIRE(me::enum_integer(piece::knight) == 1);
    REQUIRE(me::enum_integer(piece::bishop) == 2);
    REQUIRE(me::enum_integer(piece::rook)   == 3);
    REQUIRE(me::enum_integer(piece::queen)  == 4);
    REQUIRE(me::enum_integer(piece::king)   == 5);
    REQUIRE(me::enum_integer(piece::none)   == 6);
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
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "3rr3/1p1kbQ2/p1p1n3/q2p1Bp1/3P4/2N1P1P1/PP4P1/4RRK1 w - - 0 1",
        "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2",
        "8/7p/8/7k/2K1N3/6Q1/5B2/8 w - - 0 1",
        "7k/8/8/8/3K4/8/8/8 w - - 0 1",
        "5rrk/8/8/8/8/8/q3PP2/R3K2R w - - 0 1",
        "5rrk/8/8/8/8/5p2/q3PP2/R3K2R w - - 0 1",
        "7k/8/2r3n1/8/4B3/5N2/2R3P1/K7 w - - 0 1",
        "7k/8/2r3n1/8/4B3/5n2/2q3p1/K7 w - - 0 1",
        "7k/8/2r3n1/8/4B3/5n2/2q3p1/K7 b - - 0 1",
        "5R2/k5r1/P2p4/1K1Np3/1P2P1p1/8/8/1r6 w - - 0 1",
        "5R2/k5r1/P2p4/1K1Np3/1P2P1p1/8/8/1r6 b - - 0 1",
        "8/4k3/8/8/8/8/r6r/4K3 w - - 0 1",
        "8/4k3/8/8/8/8/r6r/4K3 b - - 0 1",
        "k7/8/8/8/8/8/8/4K2R w K - 0 1",
        "8/8/8/3K4/8/4k3/8/8 w - - 0 1",
        "8/8/8/3K4/8/4k3/8/8 b - - 0 1",
        "8/4k3/8/8/8/8/r6r/R3K2R w KQ - 0 1"
    };
    CHECK(board{}.export_fen() == test_cases.front());

    auto const* fen = GENERATE_REF(from_range(test_cases));
    CHECK(fen == board{fen}.export_fen());
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
                auto p = board.piece_at(m.source_square);
                if (p != piece) { continue; }
                squares.insert({static_cast<u8>(m.target_square), static_cast<u8>(m.source_square)});
            }

            auto pred = [&]([[maybe_unused]] auto& b, auto s) {
                return squares.contains(static_cast<u8>(s));
            };

            helpers::print(board, pred, "▢", fmt::color::blue);
            fmt::print("\n");
        }
    };

    SECTION("pawn moves")
    {
        constexpr std::array cases = {
            "7k/8/8/8/3pP3/8/8/K7 b - e3 0 1"
        };

        check_test_cases(cases, piece::pawn);
    }

    SECTION("knight moves")
    {
        constexpr std::array cases = {
            "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2"
        };
        check_test_cases(cases, piece::knight);
    }

    SECTION("bishop moves")
    {
        constexpr std::array cases = {
            "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2",
            "7k/8/8/3B4/8/8/8/K7 w - - 0 1",
            "7k/P5P1/8/3B4/8/5P2/P7/K7 w - - 0 1",
        };
        check_test_cases(cases, piece::bishop);
    }

    SECTION("rook moves")
    {
        constexpr std::array cases = {
            "7k/8/8/3R4/8/8/8/K7 w - - 0 1",
            "8/4k3/8/8/8/8/r6r/R3K2R w KQ - 0 1"
        };
        check_test_cases(cases, piece::rook);
    }

    SECTION("queen moves")
    {
        constexpr std::array cases = {
            "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2",
            "7k/8/8/3Q4/8/8/8/K7 w - - 0 1",
            "7k/3P4/1p3P2/3Q2p1/8/1p3P2/3p4/K7 w - - 0 1"
        };
        check_test_cases(cases, piece::queen);
    }

    SECTION("king moves")
    {
        constexpr std::array cases = {
            "k7/8/8/8/8/8/8/4K2R w K - 0 1",
            "8/8/8/3K4/8/4k3/8/8 w - - 0 1",
            "8/8/8/3K4/8/4k3/8/8 b - - 0 1",
            "8/4k3/8/8/8/8/r6r/R3K2R w KQ - 0 1"
        };
        check_test_cases(cases, piece::king);
    }
}

TEST_CASE("attacked squares", "[library]")
{
    auto attacked = [](auto& b, auto s) { return b.is_attacked(s, b.side()); };

    constexpr std::array test_cases = {
        "3rr3/1p1kbQ2/p1p1n3/q2p1Bp1/3P4/2N1P1P1/PP4P1/4RRK1 w - - 0 1",
        "rnbqkbnr/pp1ppppp/8/2p5/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2",
        "8/7p/8/7k/2K1N3/6Q1/5B2/8 w - - 0 1",
        "7k/8/8/8/3K4/8/8/8 w - - 0 1",
        "5rrk/8/8/8/8/8/q3PP2/R3K2R w - - 0 1",
        "5rrk/8/8/8/8/5p2/q3PP2/R3K2R w - - 0 1",
        "7k/8/2r3n1/8/4B3/5N2/2R3P1/K7 w - - 0 1",
        "7k/8/2r3n1/8/4B3/5n2/2q3p1/K7 w - - 0 1",
        "7k/8/2r3n1/8/4B3/5n2/2q3p1/K7 b - - 0 1",
        "5R2/k5r1/P2p4/1K1Np3/1P2P1p1/8/8/1r6 w - - 0 1",
        "5R2/k5r1/P2p4/1K1Np3/1P2P1p1/8/8/1r6 b - - 0 1",
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
    REQUIRE(board.enpassant() == square::e3);
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
    auto const* fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
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
        h1 ^= zobrist::hasher::piece(i, j, src); // taking the piece from its source square
        h1 ^= zobrist::hasher::piece(i, j, tgt); // putting the piece on its destination square

        // account for en-passant square changes
        auto const ep_before = b.enpassant();
        if (ep_before != square::none) {
            h1 ^= zobrist::hasher::enpassant_file(coord::file(ep_before));
        }

        auto const tmp = b; // make a copy of my board

        move_maker mm{b, m};
        mm.make();

        // make() now toggles side-to-move, which the hasher reflects via the
        // side constant. Account for it in the incremental update.
        h1 ^= zobrist::hasher::side_to_move();

        auto const ep_after = b.enpassant();
        if (ep_after != square::none) {
            h1 ^= zobrist::hasher::enpassant_file(coord::file(ep_after));
        }

        board::diff(tmp, b);

        INFO("Check intermediate state (move made, incremental hash update)");
        REQUIRE(h1 == hasher(b));

        mm.undo();

        // XOR again to reach previous board hash
        h1 ^= zobrist::hasher::piece(i, j, src);
        h1 ^= zobrist::hasher::piece(i, j, tgt);

        // reverse the side toggle
        h1 ^= zobrist::hasher::side_to_move();

        // reverse the en-passant delta
        if (ep_after != square::none) {
            h1 ^= zobrist::hasher::enpassant_file(coord::file(ep_after));
        }
        if (ep_before != square::none) {
            h1 ^= zobrist::hasher::enpassant_file(coord::file(ep_before));
        }

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

    // For all remaining sections: verify that after make+undo the full-recomputed
    // hash is identical to the hash before the move.  This covers all the state
    // that hasher::operator() inspects: piece placement, castling rights, and the
    // en-passant file.  make() toggles side-to-move and undo() restores it, so
    // the hash after undo() matches the hash before make().
    SECTION("hash round-trip") {
        auto check_round_trip = [](std::string_view pos) {
            board b{pos};
            zobrist::hasher h{};
            for (auto const& m : legal_moves(b)) {
                auto const before = h(b);
                move_maker mm{b, m};
                mm.make();
                mm.undo();
                REQUIRE(h(b) == before);
            }
        };

        SECTION("quiet moves - starting position") {
            check_round_trip("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        }
        SECTION("capture") {
            // white e-pawn can take the d5 pawn
            check_round_trip("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2");
        }
        SECTION("en-passant capture") {
            // white d5-pawn can take e.p. on e6; black e5-pawn just double-pushed
            check_round_trip("rnbqkbnr/pppp1ppp/8/3Pp3/8/8/PPP1PPPP/RNBQKBNR w KQkq e6 0 3");
        }
        SECTION("castling") {
            check_round_trip("r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQkq - 0 1");
            check_round_trip("r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R b KQkq - 0 1");
        }
        SECTION("promotion") {
            check_round_trip("8/P6k/8/8/8/8/8/4K3 w - - 0 1");
        }
    }

    SECTION("position uniqueness") {
        // All 20 positions reachable in one move from the starting position must
        // hash distinctly (with make() not toggling side they share the same side
        // flag, so any collision would be a genuine piece-placement collision).
        board start;
        zobrist::hasher h{};
        ankerl::unordered_dense::set<hash> seen;
        seen.insert(h(start));
        for (auto const& m : legal_moves(start)) {
            move_maker mm{start, m};
            mm.make();
            auto const [_, inserted] = seen.insert(h(start));
            REQUIRE(inserted);
            mm.undo();
        }
    }
}

TEST_CASE("legal_moves count", "[library]")
{
    // known perft(1) counts from chessprogramming.org
    constexpr std::array<std::pair<std::string_view, std::size_t>, 6> cases{{
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",      20},
        {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 48},
        {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",                     14},
        {"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 6},
        {"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",     44},
        {"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 46},
    }};

    for (auto const& [fen, expected] : cases) {
        board b{fen};
        REQUIRE(legal_moves(b).size() == expected);
    }
}

TEST_CASE("make/undo round-trip", "[library]")
{
    // For every legal move in a position, make then undo must restore the board exactly.
    auto round_trip = [](std::string_view fen) {
        board b{fen};
        auto moves = legal_moves(b);
        REQUIRE(!moves.empty());
        for (auto const& m : moves) {
            auto orig = b;
            move_maker mm{b, m};
            mm.make();
            mm.undo();
            REQUIRE(b == orig);
        }
    };

    SECTION("quiet moves - starting position") {
        round_trip("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    }
    SECTION("captures") {
        // e4 vs d5: white e-pawn can take d5
        round_trip("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2");
    }
    SECTION("en-passant capture") {
        // white pawn on d5, black just played e7-e5; white can take e.p. on e6
        round_trip("rnbqkbnr/pppp1ppp/8/3Pp3/8/8/PPP1PPPP/RNBQKBNR w KQkq e6 0 3");
    }
    SECTION("castling") {
        round_trip("r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQkq - 0 1");
        round_trip("r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R b KQkq - 0 1");
    }
    SECTION("promotion") {
        // white pawn on a7, can promote (queen, rook, bishop, knight)
        round_trip("8/P6k/8/8/8/8/8/4K3 w - - 0 1");
    }
}

TEST_CASE("fen errors", "[library]")
{
    using namespace chesslib::fen;

    SECTION("invalid active color") {
        auto result = read("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR x KQkq - 0 1");
        REQUIRE(!result);
        REQUIRE(result.error().reason == error::invalid_active_color);
        REQUIRE(result.error().input == "x");
    }

    SECTION("invalid halfmove clock") {
        auto result = read("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - abc 1");
        REQUIRE(!result);
        REQUIRE(result.error().reason == error::invalid_halfmove_clock);
        REQUIRE(result.error().input == "abc");
    }

    SECTION("invalid fullmove number") {
        auto result = read("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 abc");
        REQUIRE(!result);
        REQUIRE(result.error().reason == error::invalid_fullmove_number);
        REQUIRE(result.error().input == "abc");
    }

    SECTION("invalid en-passant target") {
        auto result = read("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq z9 0 1");
        REQUIRE(!result);
        REQUIRE(result.error().reason == error::invalid_enpassant_target);
        REQUIRE(result.error().input == "z9");
    }

    SECTION("invalid castling availability") {
        auto result = read("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KK - 0 1");
        REQUIRE(!result);
        REQUIRE(result.error().reason == error::invalid_castling_availability);
        REQUIRE(result.error().input == "KK");
    }

    SECTION("invalid piece placement character") {
        auto result = read("rnbqkbnr/pppppppp/8/8/8/8/PPPP1PPP/RNBQKBNZ w KQkq - 0 1");
        REQUIRE(!result);
        REQUIRE(result.error().reason == error::invalid_piece_placement);
    }

    SECTION("invalid en-passant rank for side to move") {
        auto result = read("rnbqkbnr/pppp1ppp/8/4p3/8/8/PPPPPPPP/RNBQKBNR w KQkq e3 0 2");
        REQUIRE(!result);
        REQUIRE(result.error().reason == error::invalid_enpassant_target);
        REQUIRE(result.error().input == "e3");
    }

    SECTION("invalid fullmove number zero") {
        auto result = read("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 0");
        REQUIRE(!result);
        REQUIRE(result.error().reason == error::invalid_fullmove_number);
        REQUIRE(result.error().input == "0");
    }

    SECTION("too few fields") {
        auto result = read("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -");
        REQUIRE(!result);
        REQUIRE(result.error().reason == error::too_few_fields);
    }

    SECTION("too many fields") {
        auto result = read("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 extra");
        REQUIRE(!result);
        REQUIRE(result.error().reason == error::too_many_fields);
    }

    SECTION("read_or_throw throws on error") {
        REQUIRE_THROWS_AS(
            read_or_throw("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR x - - 0 1"),
            std::runtime_error
        );
    }
}

TEST_CASE("checkmate and stalemate", "[library]")
{
    SECTION("checkmate - fool's mate") {
        // After 1.f3 e5 2.g4 Qh4# white is checkmated
        board b{"rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3"};
        REQUIRE(is_checkmate(b));
        REQUIRE(!is_stalemate(b));
    }

    SECTION("stalemate") {
        // Black king a8, white queen c7, white king b6 — black to move, stalemate
        board b{"k7/2Q5/1K6/8/8/8/8/8 b - - 0 1"};
        REQUIRE(is_stalemate(b));
        REQUIRE(!is_checkmate(b));
    }

    SECTION("neither - starting position") {
        board b;
        REQUIRE(!is_checkmate(b));
        REQUIRE(!is_stalemate(b));
    }
}

TEST_CASE("uci", "[library]")
{
    SECTION("to_string") {
        move m{.source_square = square::e2, .target_square = square::e4,
               .promotion = 0, .capture = 0, .double_pawn = 1, .enpassant = 0, .castling = 0};
        REQUIRE(uci::to_string(m) == "e2e4");

        // Promotion to queen
        move q{.source_square = square::a7, .target_square = square::a8,
               .promotion = static_cast<u8>(piece::queen),
               .capture = 0, .double_pawn = 0, .enpassant = 0, .castling = 0};
        REQUIRE(uci::to_string(q) == "a7a8q");

        // Knight promotion
        move n{.source_square = square::h7, .target_square = square::h8,
               .promotion = static_cast<u8>(piece::knight),
               .capture = 0, .double_pawn = 0, .enpassant = 0, .castling = 0};
        REQUIRE(uci::to_string(n) == "h7h8n");
    }

    SECTION("from_string - legal moves") {
        board b;
        auto e2e4 = uci::from_string(b, "e2e4");
        REQUIRE(e2e4.has_value());
        REQUIRE(e2e4->double_pawn == 1);
        REQUIRE(e2e4->capture == 0);

        auto g1f3 = uci::from_string(b, "g1f3");
        REQUIRE(g1f3.has_value());
        REQUIRE(g1f3->capture == 0);
    }

    SECTION("from_string - illegal move") {
        board b;
        REQUIRE(!uci::from_string(b, "e2e5").has_value()); // pawn can't jump 3 squares
        REQUIRE(!uci::from_string(b, "e1e2").has_value()); // king blocked
    }

    SECTION("from_string - malformed string") {
        board b;
        REQUIRE(!uci::from_string(b, "xyz").has_value());
        REQUIRE(!uci::from_string(b, "").has_value());
        REQUIRE(!uci::from_string(b, "e2e4e5x").has_value());
    }

    SECTION("round-trip - all starting position moves") {
        board b;
        for (auto const& m : legal_moves(b)) {
            auto const s = uci::to_string(m);
            auto const result = uci::from_string(b, s);
            REQUIRE(result.has_value());
            REQUIRE(result->source_square == m.source_square);
            REQUIRE(result->target_square == m.target_square);
            REQUIRE(result->promotion    == m.promotion);
        }
    }
}

TEST_CASE("san", "[library]")
{
    SECTION("to_string - quiet pawn push") {
        board b;
        auto m = uci::from_string(b, "e2e4").value();
        REQUIRE(san::to_string(b, m) == "e4");
    }

    SECTION("to_string - knight move") {
        board b;
        auto m = uci::from_string(b, "g1f3").value();
        REQUIRE(san::to_string(b, m) == "Nf3");
    }

    SECTION("to_string - pawn capture") {
        // after 1.e4 d5, exd5 is a pawn capture
        board b{"rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2"};
        auto m = uci::from_string(b, "e4d5").value();
        REQUIRE(san::to_string(b, m) == "exd5");
    }

    SECTION("to_string - en passant") {
        // white pawn on e5, black pawn just pushed d7-d5: en passant exd6
        board b{"rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 1"};
        auto m = uci::from_string(b, "e5d6").value();
        REQUIRE(san::to_string(b, m) == "exd6");
    }

    SECTION("to_string - promotion") {
        board b{"8/P6k/8/8/8/8/8/4K3 w - - 0 1"};
        auto m = uci::from_string(b, "a7a8q").value();
        REQUIRE(san::to_string(b, m) == "a8=Q");
    }

    SECTION("to_string - promotion with capture") {
        board b{"1r6/P6k/8/8/8/8/8/4K3 w - - 0 1"};
        auto m = uci::from_string(b, "a7b8q").value();
        REQUIRE(san::to_string(b, m) == "axb8=Q");
    }

    SECTION("to_string - kingside castle") {
        board b{"r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1"};
        auto m = uci::from_string(b, "e1g1").value();
        REQUIRE(san::to_string(b, m) == "O-O");
    }

    SECTION("to_string - queenside castle") {
        board b{"r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1"};
        auto m = uci::from_string(b, "e1c1").value();
        REQUIRE(san::to_string(b, m) == "O-O-O");
    }

    SECTION("to_string - check") {
        // Bxf7+ after 1.e4 e5 2.Bc4: bishop captures on f7, checking the king on e8
        board b{"rnbqkbnr/pppp1ppp/8/4p3/2B1P3/8/PPPP1PPP/RNBQK1NR w KQkq - 0 1"};
        auto m = uci::from_string(b, "c4f7").value();
        REQUIRE(san::to_string(b, m) == "Bxf7+");
    }

    SECTION("to_string - checkmate") {
        // Scholar's mate: Qxf7#
        board b{"r1bqkb1r/pppp1ppp/2n2n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 4 4"};
        auto m = uci::from_string(b, "h5f7").value();
        REQUIRE(san::to_string(b, m) == "Qxf7#");
    }

    SECTION("to_string - disambiguation by file") {
        // King on e4 so both rooks (a1, h1) can reach d1 unobstructed
        board b{"4k3/8/8/8/4K3/8/8/R6R w - - 0 1"};
        auto ra = uci::from_string(b, "a1d1").value();
        auto rh = uci::from_string(b, "h1d1").value();
        REQUIRE(san::to_string(b, ra) == "Rad1");
        REQUIRE(san::to_string(b, rh) == "Rhd1");
    }

    SECTION("to_string - disambiguation by rank") {
        // Two rooks on a-file (a5, a3); target a4 sits between them
        board b{"4k3/8/8/R7/8/R7/8/4K3 w - - 0 1"};
        auto r5 = uci::from_string(b, "a5a4").value();
        auto r3 = uci::from_string(b, "a3a4").value();
        REQUIRE(san::to_string(b, r5) == "R5a4");
        REQUIRE(san::to_string(b, r3) == "R3a4");
    }

    SECTION("from_string - quiet pawn push") {
        board b;
        auto result = san::from_string(b, "e4");
        REQUIRE(result.has_value());
        REQUIRE(result->source_square == square::e2);
        REQUIRE(result->target_square == square::e4);
        REQUIRE(result->double_pawn == 1);
    }

    SECTION("from_string - knight move") {
        board b;
        auto result = san::from_string(b, "Nf3");
        REQUIRE(result.has_value());
        REQUIRE(result->source_square == square::g1);
        REQUIRE(result->target_square == square::f3);
    }

    SECTION("from_string - strips check/mate suffix") {
        board b{"rnbqkbnr/pppp1ppp/8/4p3/2B1P3/8/PPPP1PPP/RNBQK1NR w KQkq - 0 1"};
        REQUIRE(san::from_string(b, "Qh5+").has_value());
        REQUIRE(san::from_string(b, "Qh5").has_value());
    }

    SECTION("from_string - castling") {
        board b{"r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1"};
        auto ks = san::from_string(b, "O-O");
        auto qs = san::from_string(b, "O-O-O");
        REQUIRE(ks.has_value());
        REQUIRE(qs.has_value());
        REQUIRE(ks->target_square == square::g1);
        REQUIRE(qs->target_square == square::c1);
    }

    SECTION("from_string - promotion") {
        board b{"8/P6k/8/8/8/8/8/4K3 w - - 0 1"};
        auto result = san::from_string(b, "a8=Q");
        REQUIRE(result.has_value());
        REQUIRE(result->promotion == static_cast<u8>(piece::queen));
        // also accept without =
        REQUIRE(san::from_string(b, "a8Q").has_value());
    }

    SECTION("from_string - disambiguation") {
        board b{"4k3/8/8/8/4K3/8/8/R6R w - - 0 1"};
        auto ra = san::from_string(b, "Rad1");
        auto rh = san::from_string(b, "Rhd1");
        REQUIRE(ra.has_value());
        REQUIRE(rh.has_value());
        REQUIRE(ra->source_square == square::a1);
        REQUIRE(rh->source_square == square::h1);
    }

    SECTION("from_string - errors") {
        board b;
        REQUIRE(san::from_string(b, "").error()   == san::error::invalid_syntax);
        REQUIRE(san::from_string(b, "e9").error()  == san::error::invalid_syntax);
        REQUIRE(san::from_string(b, "e5").error()  == san::error::no_matching_move);
        REQUIRE(san::from_string(b, "Rd1").error() == san::error::no_matching_move);
    }

    SECTION("round-trip - all legal moves from starting position") {
        board b;
        for (auto const& m : legal_moves(b)) {
            auto const s      = san::to_string(b, m);
            auto const result = san::from_string(b, s);
            REQUIRE(result.has_value());
            REQUIRE(result->source_square == m.source_square);
            REQUIRE(result->target_square == m.target_square);
            REQUIRE(result->promotion     == m.promotion);
        }
    }

    SECTION("round-trip - position with promotions and castling") {
        board b{"r3k2r/pP4pp/8/8/8/8/PP4PP/R3K2R w KQkq - 0 1"};
        for (auto const& m : legal_moves(b)) {
            auto const s      = san::to_string(b, m);
            auto const result = san::from_string(b, s);
            REQUIRE(result.has_value());
            REQUIRE(result->source_square == m.source_square);
            REQUIRE(result->target_square == m.target_square);
            REQUIRE(result->promotion     == m.promotion);
        }
    }
}

TEST_CASE("move codec", "[library]")
{
    using namespace codec;

    auto roundtrip = [](move m) {
        auto const decoded = decode(encode(m));
        REQUIRE(decoded.source_square == m.source_square);
        REQUIRE(decoded.target_square == m.target_square);
        REQUIRE(decoded.promotion     == m.promotion);
        REQUIRE(decoded.capture       == m.capture);
        REQUIRE(decoded.double_pawn   == m.double_pawn);
        REQUIRE(decoded.enpassant     == m.enpassant);
        REQUIRE(decoded.castling      == m.castling);
    };

    SECTION("quiet move") {
        roundtrip(move{.source_square = square::e2, .target_square = square::e3,
                       .promotion = 0, .capture = 0, .double_pawn = 0, .enpassant = 0, .castling = 0});
    }

    SECTION("double pawn push") {
        roundtrip(move{.source_square = square::e2, .target_square = square::e4,
                       .promotion = 0, .capture = 0, .double_pawn = 1, .enpassant = 0, .castling = 0});
    }

    SECTION("capture") {
        roundtrip(move{.source_square = square::d4, .target_square = square::e5,
                       .promotion = 0, .capture = 1, .double_pawn = 0, .enpassant = 0, .castling = 0});
    }

    SECTION("en passant") {
        roundtrip(move{.source_square = square::e5, .target_square = square::d6,
                       .promotion = 0, .capture = 1, .double_pawn = 0, .enpassant = 1, .castling = 0});
    }

    SECTION("kingside castle") {
        roundtrip(move{.source_square = square::e1, .target_square = square::g1,
                       .promotion = 0, .capture = 0, .double_pawn = 0, .enpassant = 0, .castling = 1});
    }

    SECTION("queenside castle") {
        roundtrip(move{.source_square = square::e1, .target_square = square::c1,
                       .promotion = 0, .capture = 0, .double_pawn = 0, .enpassant = 0, .castling = 1});
    }

    SECTION("promotions (quiet)") {
        for (auto p : {piece::knight, piece::bishop, piece::rook, piece::queen}) {
            roundtrip(move{.source_square = square::a7, .target_square = square::a8,
                           .promotion = static_cast<u8>(p),
                           .capture = 0, .double_pawn = 0, .enpassant = 0, .castling = 0});
        }
    }

    SECTION("promotions (capture)") {
        for (auto p : {piece::knight, piece::bishop, piece::rook, piece::queen}) {
            roundtrip(move{.source_square = square::a7, .target_square = square::b8,
                           .promotion = static_cast<u8>(p),
                           .capture = 1, .double_pawn = 0, .enpassant = 0, .castling = 0});
        }
    }

    SECTION("round-trip - all legal moves from starting position") {
        board b;
        for (auto const& m : legal_moves(b)) {
            roundtrip(m);
        }
    }
}

} // namespace chesslib::test
