#include <chesslib/util/perft.hpp>

namespace chesslib {

auto move_string(move const m) {
    return fmt::format("{}{}", me::enum_name(static_cast<square>(m.source_square)), me::enum_name(static_cast<square>(m.target_square)));
};

auto generate_moves(board& b) {
    move_list moves;
    moves.reserve(move_list_capacity);
    move_generator{b}.moves(moves);
    return moves;
}

auto perft(std::string_view fen, int depth) -> uint64_t {
    board b{fen};

    auto& s = b.state();
    auto const k = b.white_to_move() ? 0 : 1;

    auto perft = [&](int d, auto&& perft) -> perft_result {
        if (d >= depth) {
            return perft_result{1, 0, 0 ,0, 0, 0};
        }
        s.side = static_cast<side_to_move>((d+k) % 2);
        perft_result p;

        for(auto&& m : generate_moves(b)) {
            move_maker mm{b, m};
            mm.make();
            if (!b.is_king_in_check()) {
                if (d+1 == depth) {
                    p.capture += m.capture != 0;
                    p.promotion += m.promotion != 0;
                    p.enpassant += m.enpassant != 0;
                }
                auto const c = perft(d+1, perft);
                if (d == 0) {
                    fmt::print("{}: {}\n", move_string(m), c.count);
                }
                p += c;
            }
            mm.undo();
        }
        return p;
    };
    return perft(0, perft).count;
}
}