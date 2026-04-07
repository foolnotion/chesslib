#include <algorithm>
#include <cstdint>
#include <string_view>

#include <fmt/base.h>
#include <fmt/format.h>
#include <magic_enum/magic_enum.hpp>

#include <chesslib/util/perft.hpp>
#include <chesslib/board/board.hpp>
#include <chesslib/board/move_generator.hpp>
#include <chesslib/core/types.hpp>

namespace chesslib {

namespace {
auto move_string(move const m) {
    return fmt::format("{}{}", me::enum_name(static_cast<square>(m.source_square)), me::enum_name(static_cast<square>(m.target_square)));
}

auto generate_moves(board& b) {
    move_list moves;
    move_generator{b}.moves(moves);
    return moves;
}

// depth = remaining half-moves to search (1 = leaf).
// print_root = whether to print per-move counts at this level (true only at root).
inline auto perft_debug(board& b, int depth, bool print_root) -> perft_result {
    perft_result p;
    for (auto&& m : generate_moves(b)) {
        auto const side = b.side();  // side that is about to move
        move_maker mm{b, m};
        mm.make();

        // Reject moves that leave the moving side's king in check.
        // After make() the side has been toggled, so use the saved side.
        if (!b.is_king_in_check(side)) {
            perft_result c;
            if (depth == 1) {
                c = {.count = 1, .capture = m.capture != 0, .enpassant = m.enpassant != 0,
                     .castle = m.castling != 0, .promotion = m.promotion != 0};
            } else {
                c += perft_debug(b, depth - 1, /*print_root=*/false);
            }
            // After make() the side is the opponent's — is_king_in_check() therefore
            // checks whether the opponent (the side that was just moved against) is in check.
            if (b.is_king_in_check()) {
                c.check += 1;
                c.mate += std::ranges::all_of(
                    generate_moves(b),
                    [&](auto const n) -> bool { return move_maker{b, n}.check(); });
            }
            if (print_root) {
                fmt::print("{}: {}\n", move_string(m), c.count);
            }
            p += c;
        }
        mm.undo();
    }
    return p;
}

inline auto perft(board& b, int depth, bool print_root) -> u64 {
    auto count{0UL};
    for (auto&& m : generate_moves(b)) {
        auto const side = b.side();
        move_maker mm{b, m};
        mm.make();
        if (!b.is_king_in_check(side)) {
            auto const c = (depth == 1) ? 1UL : perft(b, depth - 1, /*print_root=*/false);
            if (print_root) {
                fmt::print("{}: {}\n", move_string(m), c);
            }
            count += c;
        }
        mm.undo();
    }
    return count;
}
} // namespace

auto perft_debug(std::string_view fen, int depth) -> perft_result {
    board b{fen};
    return perft_debug(b, depth, /*print_root=*/true);
}

auto perft(std::string_view fen, int depth) -> uint64_t {
    board b{fen};
    return perft(b, depth, /*print_root=*/false);
}
} // namespace chesslib
