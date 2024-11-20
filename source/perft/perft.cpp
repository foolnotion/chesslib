#include <chesslib/util/perft.hpp>
#include <cxxopts.hpp>
#include <chrono>

auto run_perft(std::string_view fen, int depth) -> void {
    auto start = std::chrono::steady_clock::now();
    auto count = chesslib::perft(fen, depth);
    auto stop  = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count() / 1e6; // NOLINT
    auto nps = static_cast<double>(count) / elapsed;
    fmt::print("\nNodes searched: {:L}\nNodes per second: {:L}\nElapsed: {} seconds\n", count, nps, elapsed);
}

auto run_perft_debug(std::string_view fen, int depth) -> void {
    auto start = std::chrono::steady_clock::now();
    auto stats = chesslib::perft_debug(fen, depth);
    auto stop  = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count() / 1e6; // NOLINT
    auto nps = static_cast<double>(stats.count) / elapsed;
    // fmt::print("\nCaptures: {:L}, Promotions: {:L}, Enpassant: {:L}, Checks: {:L}, Mates: {:L}\n", stats.capture, stats.promotion, stats.enpassant, stats.check, stats.mate);
    fmt::print("\nCaptures: {:L}, Enpassant: {:L}, Castles: {:L}, Promotions: {:L}, Checks: {:L}, Mates: {:L}\n", stats.capture, stats.enpassant, stats.castle, stats.promotion, stats.check, stats.mate);
    fmt::print("\nNodes searched: {:L}\nNodes per second: {:L}\nElapsed: {} seconds\n", stats.count, nps, elapsed);
}

auto main(int argc, char** argv) -> int {
    constexpr auto* name = "perft";
    constexpr auto* desc = "PERFT, (PERFormance Test, move path enumeration)\n\
A debugging function to walk the move generation tree of strictly\n\
legal moves to count all the leaf nodes of a certain depth\n";
    constexpr auto* init = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    cxxopts::Options options(name, desc);
    options.add_options()
        ("f,fen", "Position as fen string", cxxopts::value<std::string>()->default_value(init))
        ("d,depth", "Search depth in half-moves", cxxopts::value<int>()->default_value("5"))
        ("g,debug", "Provide debug statistics", cxxopts::value<bool>()->default_value("false"))
        ("h,help", "Print usage");

    auto result = options.parse(argc, argv);
    if (result.arguments().empty() || result.count("help")) {
        fmt::print("{}\n", options.help());
    }

    auto const depth = result["depth"].as<int>();
    auto const fen   = result["fen"].as<std::string>();
    auto const debug = result["debug"].as<bool>();

    // set the locale for formatting
    std::locale::global(std::locale("en_US.UTF-8"));
    debug ? run_perft_debug(fen,depth) : run_perft(fen, depth);
    return 0;
}
