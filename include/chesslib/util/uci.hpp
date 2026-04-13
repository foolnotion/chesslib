#ifndef CHESSLIB_UTIL_UCI_HPP
#define CHESSLIB_UTIL_UCI_HPP

#include <string>
#include <string_view>
#include <tl/expected.hpp>
#include "chesslib/core/types.hpp"

namespace chesslib {
    class board;

namespace uci {

// Format a move in UCI notation: "e2e4", "a7a8q" (promotion lowercase).
auto to_string(move m) -> std::string;

// Parse a UCI string into a legal move on board b.
// Returns an error string if the move string is malformed or not legal in b.
// Takes board& because legal move generation requires make/undo internally.
auto from_string(board const& b, std::string_view s) -> tl::expected<move, std::string>;

} // namespace uci
} // namespace chesslib

#endif
