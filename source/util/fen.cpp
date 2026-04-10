#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstddef>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include <fmt/format.h>
#include <libassert/assert.hpp>
#include <magic_enum/magic_enum.hpp>
#include <tl/expected.hpp>

#include "chesslib/util/fen.hpp"
#include "chesslib/board/board.hpp"
#include "chesslib/board/encoding.hpp"
#include "chesslib/core/types.hpp"

using namespace chesslib::encoding;
using namespace magic_enum::bitwise_operators;

namespace chesslib::fen {

    auto read(std::string_view fen) -> tl::expected<board, parse_error> {
        board b;
        std::ranges::fill(b.pieces_, piece::none);
        std::ranges::fill(b.colors_, color::none);
        auto& state = b.state_;

        auto n = 0;
        for (auto field : std::ranges::views::split(fen, ' ')) {
            switch(n++) {
                case fen_record::piece_placement: {
                    auto make_err = [&](auto const& f) {
                        return tl::unexpected(parse_error{
                            .reason = error::invalid_piece_placement,
                            .input  = std::string{f.begin(), f.end()}
                        });
                    };
                    auto row = static_cast<i32>(square::a8);
                    auto rank_count = 0;
                    auto white_kings = 0;
                    auto black_kings = 0;
                    for (auto line : std::ranges::views::split(field, '/')) {
                        rank_count++;
                        auto sq = row - 1;
                        auto file_count = 0;
                        for (auto c : line) {
                            if (std::isdigit(c) != 0) {
                                auto skip = c - '0';
                                if (skip < 1 || skip > 8) { return make_err(line); }
                                sq += skip;
                                file_count += skip;
                            }
                            else if (std::isalpha(c) != 0) {
                                sq += 1;
                                file_count += 1;
                                if (!coord::valid(sq)) { return make_err(line); }
                                auto col     = std::isupper(c) != 0 ? color::white : color::black;
                                auto pic     = char2piece(static_cast<char>(std::tolower(c)));
                                b.pieces_[static_cast<size_t>(sq)] = pic;
                                b.colors_[static_cast<size_t>(sq)] = col;
                                if (pic == piece::king) {
                                    auto& k   = col == color::white ? state.white_king : state.black_king;
                                    auto& cnt = col == color::white ? white_kings : black_kings;
                                    cnt++;
                                    if (cnt > 1) { return make_err(line); }
                                    k = static_cast<square>(sq);
                                }
                            }
                        }
                        if (file_count != 8) { return make_err(line); }
                        row -= coord::ncol;
                    }
                    if (rank_count != 8) { return make_err(field); }
                    if (white_kings != 1 || black_kings != 1) { return make_err(field); }
                    break;
                }
                case fen_record::active_color: {
                    if (std::ranges::empty(field)) {
                        return tl::unexpected(parse_error{
                            .reason = error::invalid_active_color,
                            .input  = {}
                        });
                    }
                    auto c = field.front();
                    if (c != 'w' && c != 'b') {
                        return tl::unexpected(parse_error{
                            .reason = error::invalid_active_color,
                            .input  = std::string{field.begin(), field.end()}
                        });
                    }
                    b.state_.side = c == 'w' ? side_to_move::white : side_to_move::black;
                    break;
                }
                case fen_record::castling_availability: {
                    u8 castling = 0;
                    for (auto c : field) {
                        switch(c) {
                            case 'K': castling |= me::enum_integer(castling_rights::wk); break;
                            case 'Q': castling |= me::enum_integer(castling_rights::wq); break;
                            case 'k': castling |= me::enum_integer(castling_rights::bk); break;
                            case 'q': castling |= me::enum_integer(castling_rights::bq); break;
                            default:  break;
                        }
                    }
                    state.castling = static_cast<castling_rights>(castling);
                    break;
                }
                case fen_record::en_passant_target: {
                    auto const sv = std::string_view{field.begin(), field.end()};
                    if (sv != "-") {
                        auto res = me::enum_cast<square>(sv);
                        if (!res) {
                            return tl::unexpected(parse_error{
                                .reason = error::invalid_enpassant_target,
                                .input  = std::string{sv}
                            });
                        }
                        state.enpassant = res.value();
                    }
                    break;
                }
                case fen_record::halfmove_clock: {
                    auto const* end = std::to_address(field.end());
                    auto [ptr, ec] = std::from_chars(std::to_address(field.begin()), end, state.halfmove_clock);
                    if (ec != std::errc{} || ptr != end) {
                        return tl::unexpected(parse_error{
                            .reason = error::invalid_halfmove_clock,
                            .input  = std::string{field.begin(), field.end()}
                        });
                    }
                    break;
                }
                case fen_record::fullmove_number: {
                    auto const* end = std::to_address(field.end());
                    auto [ptr, ec] = std::from_chars(std::to_address(field.begin()), end, state.fullmove_number);
                    if (ec != std::errc{} || ptr != end) {
                        return tl::unexpected(parse_error{
                            .reason = error::invalid_fullmove_number,
                            .input  = std::string{field.begin(), field.end()}
                        });
                    }
                    break;
                }
                default: {
                    return tl::unexpected(parse_error{
                        .reason = error::too_many_fields,
                        .input  = std::string{fen}
                    });
                }
            }
        }
        if (n < fen_record::num_fields) {
            return tl::unexpected(parse_error{
                .reason = error::too_few_fields,
                .input  = std::string{fen}
            });
        }
        return b;
    }

    auto read_or_throw(std::string_view fen) -> board {
        auto result = read(fen);
        if (!result) {
            auto const& e = result.error();
            throw std::runtime_error(fmt::format(
                "FEN parse error ({}): \"{}\"",
                me::enum_name(e.reason), e.input));
        }
        return std::move(result).value();
    }

    auto write(board const& b) -> std::string {
        auto const& state  = b.state_;

        std::string fen;
        for (i32 i = me::enum_integer(square::a8); i >= 0; i -= coord::ncol) {
            for (auto sq = i; coord::valid(sq); ) {
                auto k = sq;
                while(coord::valid(sq) && b.piece_at(sq) == piece::none) { ++sq; }
                if (sq > k) {
                    ASSERT(sq-k <= 8);
                    fen.push_back(static_cast<char>(sq - k + '0'));
                }
                while(coord::valid(sq) && b.piece_at(sq) != piece::none) {
                    auto [p, c] = b[sq++];
                    fen.push_back(piece_letters[c == color::white ? 0 : 1][me::enum_integer(p)]);
                }
            }
            if (std::cmp_greater_equal(i, coord::ncol)) {
                fen.push_back('/');
            }
        }

        fen.push_back(' ');
        fen.push_back(b.white_to_move() ? 'w' : 'b');

        fen.push_back(' ');
        auto castling = state.castling;
        if ((castling & castling_rights::wk) == castling_rights::wk) { fen.push_back('K'); }
        if ((castling & castling_rights::wq) == castling_rights::wq) { fen.push_back('Q'); }
        if ((castling & castling_rights::bk) == castling_rights::bk) { fen.push_back('k'); }
        if ((castling & castling_rights::bq) == castling_rights::bq) { fen.push_back('q'); }
        if (me::enum_integer(castling) == 0) { fen.push_back('-'); }

        fen.push_back(' ');
        fen += state.enpassant == square::none ? "-" : me::enum_name(state.enpassant);

        fen.push_back(' ');
        fen += fmt::format("{} {}", state.halfmove_clock, state.fullmove_number);
        return fen;
    }
}  // namespace chesslib::fen
