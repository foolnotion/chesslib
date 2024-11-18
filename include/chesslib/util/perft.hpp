#ifndef CHESSLIB_PERFT_HPP
#define CHESSLIB_PERFT_HPP

#include <fmt/core.h>
#include <chesslib/chesslib.hpp>

namespace chesslib {

struct perft_result {
    uint64_t count{0};
    uint64_t capture{0};
    uint64_t promotion{0};
    uint64_t enpassant{0};
    uint64_t check{0};
    uint64_t mate{0};

    auto operator+=(perft_result const& other) -> perft_result& {
        count += other.count;
        capture += other.capture;
        promotion += other.promotion;
        enpassant += other.enpassant;
        check += other.check;
        mate += other.mate;
        return *this;
    }

    friend auto operator+(perft_result& a, perft_result const& b) -> perft_result {
        a += b;
        return a;
    }
};

auto perft(std::string_view fen, int depth) -> uint64_t;

} // namespace chesslib

#endif