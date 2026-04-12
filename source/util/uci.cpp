#include <cctype>
#include <string>
#include <string_view>

#include <magic_enum/magic_enum.hpp>
#include <tl/expected.hpp>

#include "chesslib/util/uci.hpp"
#include "chesslib/board/move_generator.hpp"
#include "chesslib/core/types.hpp"

namespace chesslib::uci {

auto to_string(move m) -> std::string {
    // Source and target squares are named by magic_enum (e.g., "e2", "e4").
    std::string s;
    s += me::enum_name(static_cast<square>(m.source_square));
    s += me::enum_name(static_cast<square>(m.target_square));
    // Promotion piece: always lowercase (UCI spec). Encoded as piece enum value.
    if (m.promotion != 0) {
        s += static_cast<char>(
            std::tolower(static_cast<unsigned char>(piece_letters[0][m.promotion])));
    }
    return s;
}

namespace {
// Convert a file char ('a'–'h') and rank char ('1'–'8') to a 0x88 square index.
constexpr auto parse_square(char file, char rank) -> square {
    auto const f = static_cast<u8>(file - 'a'); // 0–7
    auto const r = static_cast<u8>(rank - '1'); // 0–7
    return static_cast<square>((static_cast<unsigned>(r) << 4U) | static_cast<unsigned>(f));
}

auto find_legal_move(board const& b, auto&& predicate) -> tl::expected<move, std::string> {
    auto& mutable_board = const_cast<board&>(b);
    move_list pseudo;
    move_generator{mutable_board}.moves(pseudo);

    for (auto const& m : pseudo) {
        if (!predicate(m)) { continue; }
        move_maker mm{mutable_board, m};
        if (!mm.check()) {
            return m;
        }
    }
    return tl::unexpected{std::string{}};
}
} // namespace

auto from_string(board const& b, std::string_view s) -> tl::expected<move, std::string> {
    if (s.size() < 4 || s.size() > 5) {
        return tl::unexpected{std::string{s}};
    }
    if (s[0] < 'a' || s[0] > 'h' || s[2] < 'a' || s[2] > 'h' ||
        s[1] < '1' || s[1] > '8' || s[3] < '1' || s[3] > '8') {
        return tl::unexpected{std::string{s}};
    }

    auto const src   = parse_square(s[0], s[1]);
    auto const tgt   = parse_square(s[2], s[3]);
    auto const promo = (s.size() == 5) ? char2piece(static_cast<char>(std::tolower(
                                              static_cast<unsigned char>(s[4]))))
                                        : piece::none;

    auto result = find_legal_move(b, [&](move const& m) {
        if (m.source_square != static_cast<u8>(src)) { return false; }
        if (m.target_square != static_cast<u8>(tgt)) { return false; }
        if (promo != piece::none && m.promotion != static_cast<u8>(promo)) { return false; }
        if (promo == piece::none && m.promotion != 0) { return false; }
        return true;
    });
    if (result) {
        return result;
    }
    return tl::unexpected{std::string{s}};
}

} // namespace chesslib::uci
