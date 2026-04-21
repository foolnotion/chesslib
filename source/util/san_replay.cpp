#include <cctype>
#include <cstring>
#include <string_view>

#include "chesslib/util/san.hpp"

#include <tl/expected.hpp>

#include "chesslib/board/board.hpp"
#include "chesslib/board/encoding.hpp"
#include "chesslib/core/types.hpp"
#include "chesslib/util/san_replay.hpp"

namespace chesslib::san
{

using namespace encoding;

namespace
{

constexpr auto char_to_piece_r(char c) -> piece
{
    switch (std::toupper(static_cast<unsigned char>(c))) {
        case 'N':
            return piece::knight;
        case 'B':
            return piece::bishop;
        case 'R':
            return piece::rook;
        case 'Q':
            return piece::queen;
        case 'K':
            return piece::king;
        default:
            return piece::none;
    }
}

constexpr auto is_promo_letter_r(char c) -> bool
{
    return c == 'Q' || c == 'R' || c == 'B' || c == 'N';
}

constexpr auto is_rank_digit_r(char c) -> bool
{
    return c >= '1' && c <= '8';
}
constexpr auto is_file_letter_r(char c) -> bool
{
    return c >= 'a' && c <= 'h';
}

}  // namespace

replayer::replayer(board& b)
    : b_ {b}
{
}

auto replayer::play(std::string_view s) -> tl::expected<move, error>
{
    if (s.empty()) {
        return tl::unexpected {error::invalid_syntax};
    }

    if (s.back() == '+' || s.back() == '#') {
        s.remove_suffix(1);
    }

    if (s.empty()) {
        return tl::unexpected {error::invalid_syntax};
    }

    if (s == "O-O-O") {
        auto m = detail::parse_castle_move(b_, false);
        if (!m) {
            return m;
        }
        move_maker {b_, *m}.make();
        return m;
    }
    if (s == "O-O") {
        auto m = detail::parse_castle_move(b_, true);
        if (!m) {
            return m;
        }
        move_maker {b_, *m}.make();
        return m;
    }

    u8 promo {0};
    if (s.size() >= 2 && is_promo_letter_r(s.back())) {
        char const penult = s[s.size() - 2];
        if (penult == '=') {
            promo = static_cast<u8>(char_to_piece_r(s.back()));
            s.remove_suffix(2);
        } else if (is_rank_digit_r(penult)) {
            promo = static_cast<u8>(char_to_piece_r(s.back()));
            s.remove_suffix(1);
        }
    }

    if (s.size() < 2) {
        return tl::unexpected {error::invalid_syntax};
    }
    char const tgt_file = s[s.size() - 2];
    char const tgt_rank = s[s.size() - 1];
    if (!is_file_letter_r(tgt_file) || !is_rank_digit_r(tgt_rank)) {
        return tl::unexpected {error::invalid_syntax};
    }
    auto const tgt_sq =
        static_cast<u8>(coord::square_index(tgt_rank - '1', tgt_file - 'a'));
    s.remove_suffix(2);

    if (!s.empty() && s.back() == 'x') {
        s.remove_suffix(1);
    }

    piece p = piece::pawn;
    if (!s.empty() && std::isupper(static_cast<unsigned char>(s.front()))) {
        p = char_to_piece_r(s.front());
        if (p == piece::none) {
            return tl::unexpected {error::invalid_syntax};
        }
        s.remove_prefix(1);
    }

    if (s.size() > 2) {
        return tl::unexpected {error::invalid_syntax};
    }

    int disambig_file = -1;
    int disambig_rank = -1;

    if (s.size() == 2) {
        if (!is_file_letter_r(s[0]) || !is_rank_digit_r(s[1])) {
            return tl::unexpected {error::invalid_syntax};
        }
        disambig_file = s[0] - 'a';
        disambig_rank = s[1] - '1';
    } else if (s.size() == 1) {
        if (is_file_letter_r(s[0])) {
            disambig_file = s[0] - 'a';
        } else if (is_rank_digit_r(s[0])) {
            disambig_rank = s[0] - '1';
        } else {
            return tl::unexpected {error::invalid_syntax};
        }
    }

    auto candidates = detail::enumerate_candidates(
        b_, p, tgt_sq, promo, disambig_file, disambig_rank);

    if (candidates.size() == 0) {
        return tl::unexpected {error::no_matching_move};
    }

    if (candidates.size() == 1) {
        if (!detail::is_move_legal(b_, candidates[0], p)) {
            return tl::unexpected {error::no_matching_move};
        }
        move_maker {b_, candidates[0]}.make();
        return candidates[0];
    }

    move const* found = nullptr;
    for (int i = 0; i < candidates.size(); ++i) {
        if (!detail::is_move_legal(b_, candidates[i], p)) {
            continue;
        }
        if (found != nullptr) {
            return tl::unexpected {error::ambiguous};
        }
        found = &candidates[i];
    }

    if (!found) {
        return tl::unexpected {error::no_matching_move};
    }

    move_maker {b_, *found}.make();
    return *found;
}

}  // namespace chesslib::san
