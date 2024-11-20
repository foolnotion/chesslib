#include <chesslib/util/perft.hpp>

namespace chesslib {

auto move_string(move const m) {
    return fmt::format("{}{}", me::enum_name(static_cast<square>(m.source_square)), me::enum_name(static_cast<square>(m.target_square)));
};

auto generate_moves(board& b) {
    move_list moves;
    move_generator{b}.moves(moves);
    return moves;
}

inline auto perft_debug(board& b, int const k, int d, int const depth) -> perft_result {
    b.state().side = static_cast<side_to_move>((d+k) % 2);
    auto flip_side = [](board& b) {
        b.state().side = static_cast<side_to_move>((static_cast<u8>(b.state().side)+1) % 2);
    };
    perft_result p;
    for(auto&& m : generate_moves(b)) {
        move_maker mm{b, m};
        mm.make();

        if (!b.is_king_in_check()) {
            perft_result c;
            if (d+1 >= depth) {
                c = {.count = 1, .capture = m.capture != 0, .enpassant = m.enpassant != 0, .castle = m.castling != 0, .promotion = m.promotion != 0};
            } else {
                c += perft_debug(b, k, d+1, depth);
            }
            flip_side(b);
            if (b.is_king_in_check()) {
                c.check += 1;
                c.mate += std::ranges::all_of(
                    generate_moves(b),
                    [&](auto const n) { return move_maker {b, n}.check(); });
            }
            flip_side(b);
            if (d == 0) {
                fmt::print("{}{}: {}\n", std::string(d, '\t'), move_string(m), c.count);
            }
            p += c;
        }
        mm.undo();
    }
    return p;
}

auto perft_debug(std::string_view fen, int depth) -> perft_result {
    board b{fen};
    auto& s = b.state();
    auto const k = b.white_to_move() ? 0 : 1;
    return perft_debug(b, k, 0, depth);
}

inline auto perft(board& b, int const k, int d, int const depth) -> u64 {
    b.state().side = static_cast<side_to_move>((d+k) % 2);
    auto count{0UL};
    for(auto&& m : generate_moves(b)) {
        move_maker mm{b, m};
        mm.make();
        if (!b.is_king_in_check()) {
            auto const c = (d+1 >= depth) ? 1UL : perft(b, k, d+1, depth);
            if (d == 0) {
                fmt::print("{}: {}\n", move_string(m), c);
            }
            count += c;
        }
        mm.undo();
    }
    return count;
}

auto perft(std::string_view fen, int depth) -> uint64_t {
    board b{fen};
    auto& s = b.state();
    auto const k = b.white_to_move() ? 0 : 1;
    return perft(b, k, 0, depth);
}
} // namespace chesslib
