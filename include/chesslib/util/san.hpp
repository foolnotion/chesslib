#ifndef CHESSLIB_UTIL_SAN_HPP
#define CHESSLIB_UTIL_SAN_HPP

#include <string>
#include <string_view>
#include <tl/expected.hpp>
#include "chesslib/core/types.hpp"

namespace chesslib {
    class board;

namespace san {

enum class error : u8 {
    invalid_syntax,    // string cannot be parsed as SAN
    no_matching_move,  // no legal move corresponds to the SAN string
    ambiguous,         // more than one legal move matches (malformed SAN)
};

// Format a move in SAN notation: "e4", "Nf3", "O-O", "exd8=Q#", etc.
// Takes board& because check/checkmate detection requires make/undo internally.
auto to_string(board& b, move m) -> std::string;

// Parse a SAN string into a legal move on board b.
// Returns an error if the string is malformed, no legal move matches, or the
// match is ambiguous (which should not happen for a valid SAN string).
auto from_string(board& b, std::string_view s) -> tl::expected<move, error>;

} // namespace san
} // namespace chesslib

#endif
