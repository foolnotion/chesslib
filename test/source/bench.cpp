#include <chrono>
#include <string_view>

#include <catch2/catch_test_macros.hpp>
#include <nanobench.h>
#include <chesslib/chesslib.hpp>
#include <chesslib/util/perft.hpp>

/*
 * Performance regression tests.  Excluded from the normal test run — invoke
 * with:  ctest --test-dir build/dev -R bench  (or pass [.bench] to the binary)
 *
 * Perft NPS tests use raw chrono (100–500 ms per call — throughput metric).
 * Microbenchmarks use nanobench for statistical sampling and warmup.
 */

namespace {
using clock = std::chrono::steady_clock;

auto elapsed_ns(clock::time_point t0) -> double {
    return static_cast<double>((clock::now() - t0).count());
}

auto measure_perft_nps(std::string_view fen, int depth) -> double {
    auto t0    = clock::now();
    auto nodes = chesslib::perft(fen, depth);
    auto dt_ns = elapsed_ns(t0);
    return static_cast<double>(nodes) / (dt_ns / 1e9);
}

auto make_bench() -> ankerl::nanobench::Bench {
    return ankerl::nanobench::Bench()
        .warmup(200)
        .minEpochIterations(500)
        .relative(true);
}

// Extract median ops/second from the last result.
auto ops_per_sec(ankerl::nanobench::Bench const& bench) -> double {
    namespace nb = ankerl::nanobench;
    auto const& r = bench.results().back();
    auto median_s = r.median(nb::Result::Measure::elapsed); // seconds per op
    return median_s > 0.0 ? 1.0 / median_s : 0.0;
}
} // namespace

// ─── Perft throughput ───────────────────────────────────────────────────────
// Exercises the full make/undo + move-generation + legality-filter pipeline.
// Minimum floor: 5 M nodes/s — well below observed ~30 M NPS on modern HW.
static constexpr double min_perft_nps = 5'000'000.0;

TEST_CASE("bench: perft NPS - starting position", "[.bench]")
{
    // depth 5 → 4,865,609 nodes: enough for a stable timing sample (~150 ms)
    auto nps = measure_perft_nps(
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 5);
    INFO("NPS: " << static_cast<long long>(nps));
    REQUIRE(nps >= min_perft_nps);
}

TEST_CASE("bench: perft NPS - position 2 (Kiwipete)", "[.bench]")
{
    // depth 4 → 4,085,603 nodes
    auto nps = measure_perft_nps(
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 4);
    INFO("NPS: " << static_cast<long long>(nps));
    REQUIRE(nps >= min_perft_nps);
}

TEST_CASE("bench: perft NPS - position 5", "[.bench]")
{
    // depth 4 → 2,103,487 nodes
    auto nps = measure_perft_nps(
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 4);
    INFO("NPS: " << static_cast<long long>(nps));
    REQUIRE(nps >= min_perft_nps);
}

// ─── FEN parsing throughput ──────────────────────────────────────────────────
// Relevant for database ingestion: parsing millions of positions from PGN.
// Minimum floor: 500k parses/s.
TEST_CASE("bench: FEN parse throughput", "[.bench]")
{
    namespace nb = ankerl::nanobench;
    static constexpr std::array fens = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
    };

    std::size_t i = 0;
    auto bench = make_bench().run("FEN parse", [&]() {
        auto b = chesslib::fen::read_or_throw(fens[i % fens.size()]);
        nb::doNotOptimizeAway(b);
        ++i;
    });

    auto pps = ops_per_sec(bench);
    INFO("parses/s: " << static_cast<long long>(pps));
    REQUIRE(pps >= 500'000.0);
}

// ─── Legal move generation throughput ────────────────────────────────────────
// Hot path in any search engine.
// Minimum floor: 500k generations/s.
TEST_CASE("bench: legal_moves throughput", "[.bench]")
{
    namespace nb = ankerl::nanobench;
    chesslib::board b{"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"};

    auto bench = make_bench().run("legal_moves", [&]() {
        auto moves = chesslib::legal_moves(b);
        nb::doNotOptimizeAway(moves);
    });

    auto gps = ops_per_sec(bench);
    INFO("generations/s: " << static_cast<long long>(gps));
    REQUIRE(gps >= 500'000.0);
}

// ─── UCI round-trip throughput ───────────────────────────────────────────────
// Relevant for replaying games: parse every move as a UCI string.
// Minimum floor: 200k round-trips/s.
TEST_CASE("bench: UCI parse throughput", "[.bench]")
{
    namespace nb = ankerl::nanobench;
    chesslib::board b;
    auto moves = chesslib::legal_moves(b);

    std::size_t i = 0;
    auto bench = make_bench().run("UCI round-trip", [&]() {
        auto const& m = moves[i % moves.size()];
        auto s        = chesslib::uci::to_string(m);
        auto result   = chesslib::uci::from_string(b, s);
        nb::doNotOptimizeAway(result);
        ++i;
    });

    auto rps = ops_per_sec(bench);
    INFO("round-trips/s: " << static_cast<long long>(rps));
    REQUIRE(rps >= 200'000.0);
}

// ─── Zobrist hash throughput ─────────────────────────────────────────────────
// Full recompute from a complex position.
// Minimum floor: 1M hashes/s.
TEST_CASE("bench: Zobrist hash throughput", "[.bench]")
{
    namespace nb = ankerl::nanobench;
    chesslib::board b{"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"};
    chesslib::zobrist::hasher h{};

    auto bench = make_bench().run("Zobrist hash", [&]() {
        nb::doNotOptimizeAway(h(b));
    });

    auto hps = ops_per_sec(bench);
    INFO("hashes/s: " << static_cast<long long>(hps));
    REQUIRE(hps >= 1'000'000.0);
}
