#ifndef CHESSLIB_EVALUATION_HPP
#define CHESSLIB_EVALUATION_HPP

#include <cstdlib>

namespace chesslib {
    class board; // forward declaration
    auto evaluate(board const& b) -> int64_t;
};

#endif