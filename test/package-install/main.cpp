#include <chesslib/chesslib.hpp>

#include <cstdlib>

int main() {
    chesslib::board board{};
    auto const moves = chesslib::legal_moves(board);

    if (moves.size() != 20) {
        return EXIT_FAILURE;
    }

    auto const move = chesslib::uci::from_string(board, "e2e4");
    if (!move.has_value()) {
        return EXIT_FAILURE;
    }

    return chesslib::san::to_string(board, *move) == "e4" ? EXIT_SUCCESS : EXIT_FAILURE;
}