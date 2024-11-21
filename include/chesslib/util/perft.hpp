#ifndef CHESSLIB_PERFT_HPP
#define CHESSLIB_PERFT_HPP

#include <fmt/core.h>
#include <chesslib/chesslib.hpp>

namespace chesslib {

struct perft_result {
    u64 count{0};
    u64 capture{0};
    u64 enpassant{0};
    u64 castle{0};
    u64 promotion{0};
    u64 check{0};
    u64 mate{0};

    auto operator+=(perft_result const& other) -> perft_result& {
        count += other.count;
        capture += other.capture;
        enpassant += other.enpassant;
        castle += other.castle;
        promotion += other.promotion;
        check += other.check;
        mate += other.mate;
        return *this;
    }

    friend auto operator+(perft_result& a, perft_result const& b) -> perft_result {
        a += b;
        return a;
    }

    friend auto operator==(perft_result const& a, perft_result const& b) {
        return std::tie(a.count, a.capture, a.enpassant, a.castle, a.promotion, a.check, a.mate) ==
               std::tie(b.count, b.capture, b.enpassant, b.castle, b.promotion, b.check, b.mate);
    }
};

auto perft(std::string_view fen, int depth) -> uint64_t;
auto perft_debug(std::string_view fen, int depth) -> perft_result;

} // namespace chesslib

#endif