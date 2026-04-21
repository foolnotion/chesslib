#ifndef CHESSLIB_UTIL_SAN_REPLAY_HPP
#define CHESSLIB_UTIL_SAN_REPLAY_HPP

#include <string_view>
#include <tl/expected.hpp>
#include "chesslib/core/types.hpp"
#include "chesslib/util/san.hpp"

namespace chesslib {
class board;

namespace san {

class replayer {
public:
    explicit replayer(board& b);

    auto play(std::string_view s) -> tl::expected<move, error>;

private:
    board& b_;
};

} // namespace san
} // namespace chesslib

#endif
