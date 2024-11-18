#include <chesslib/util/perft.hpp>
#include <cxxopts.hpp>

auto main(int argc, char** argv) -> int {
    constexpr auto* name = "perft";
    constexpr auto* desc = "PERFT, (PERFormance Test, move path enumeration)\n\
A debugging function to walk the move generation tree of strictly\n\
legal moves to count all the leaf nodes of a certain depth\n";

    cxxopts::Options options(name, desc);
    options.add_options()
        ("f,fen", "Position as fen string", cxxopts::value<std::string>()->default_value("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"))
        ("d,depth", "Search depth in half-moves", cxxopts::value<int>())
        ("h,help", "Print usage");

    auto result = options.parse(argc, argv);
    if (result.arguments().empty() || result.count("help")) {
        fmt::print("{}\n", options.help());
    }

    if (result.count("depth")) {
        auto const depth = result["depth"].as<int>();
        auto const fen   = result["fen"].as<std::string>();
        auto count = chesslib::perft(fen, depth);
        fmt::print("\nNodes searched: {}\n", count);
    }
    return 0;
}
