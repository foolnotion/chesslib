#ifndef CHESSLIB_UTIL_SAN_HPP
#define CHESSLIB_UTIL_SAN_HPP

#include <string>
#include <string_view>

#include <tl/expected.hpp>

#include "chesslib/core/types.hpp"

namespace chesslib
{
class board;

namespace san
{

enum class error : u8
{
    invalid_syntax,  // string cannot be parsed as SAN
    no_matching_move,  // no legal move corresponds to the SAN string
    ambiguous,  // more than one legal move matches (malformed SAN)
};

// Format a move in SAN notation: "e4", "Nf3", "O-O", "exd8=Q#", etc.
// The mutable overload is the real implementation (make/undo required
// internally). The const overload copies the board and delegates — use only in
// non-hot paths.
auto to_string(board& b, move m) -> std::string;
auto to_string(board const& b, move m) -> std::string;

// Parse a SAN string into a legal move on board b.
// Returns an error if the string is malformed, no legal move matches, or the
// match is ambiguous (which should not happen for a valid SAN string).
// The mutable overload is the real implementation; const overload copies.
auto from_string(board& b, std::string_view s) -> tl::expected<move, error>;
auto from_string(board const& b, std::string_view s)
    -> tl::expected<move, error>;

namespace detail
{

auto is_move_legal(board& b, move m, piece p) -> bool;

struct candidate_list
{
    move data[16];
    int count {0};

    auto push_back(move m) -> bool
    {
        if (count >= static_cast<int>(sizeof(data) / sizeof(data[0]))) {
            return false;
        }
        data[count++] = m;
        return true;
    }
    auto begin() const -> move const* { return data; }
    auto end() const -> move const* { return data + count; }
    auto size() const -> int { return count; }
    auto operator[](int i) const -> move const& { return data[i]; }
};

auto enumerate_candidates(board& b,
                          piece p,
                          int tgt_sq,
                          u8 promo,
                          int disambig_file,
                          int disambig_rank) -> candidate_list;

auto parse_castle_move(board const& b, bool kingside)
    -> tl::expected<move, error>;

}  // namespace detail

}  // namespace san
}  // namespace chesslib

#endif
