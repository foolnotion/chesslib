#ifndef CHESSLIB_BOARD_0X88_HPP
#define CHESSLIB_BOARD_0X88_HPP

#include <array>

#include <fmt/color.h>
#include <fmt/core.h>
#include <libassert/assert.hpp>

#include "encoding.hpp"
#include "chesslib/util/fen.hpp"

namespace me = magic_enum;
using namespace me::bitwise_operators; // NOLINT

namespace chesslib {

using encoding::coord;

struct board_state {
    side_to_move side{side_to_move::white};
    castling_rights castling{0b1111};
    square enpassant{square::none};
    square white_king{square::e1};
    square black_king{square::e8};
    i32 ply{0};
    i32 count{0};

    auto operator==(board_state const& s) const -> bool {
        return std::tie(side, castling, enpassant, white_king, black_king, ply, count) ==
               std::tie(s.side, s.castling, s.enpassant, s.white_king, s.black_king, s.ply, s.count);
    }
};

struct move_maker {
    explicit move_maker(board& b, move m)
        : board_(b), move_(m) {}

    auto make() -> void;
    auto undo() -> void;
    auto check() -> bool;
    auto captured() const -> piece { return std::get<1>(capture_info_); }

    private:
    board& board_; // NOLINT
    move move_;
    board_state state_;
    std::tuple<u8, piece, color> capture_info_{square::none, piece::none, color::none};
};

class board
{
    using coord = encoding::coord;

    template<piece P>
    static constexpr bool is_sliding_v = (is<piece::bishop, piece::rook, piece::queen>(P));

    template<piece... Pieces>
    requires (is_sliding_v<Pieces> && ...) || (!is_sliding_v<Pieces> && ...)
    auto attacked_by(auto const& offsets, auto square_idx, auto side) const -> bool {
        using std::ranges::any_of;

        if (!coord::valid(square_idx)) { return false; }
        auto c = side == side_to_move::white ? color::white : color::black;

        if constexpr ((is_sliding_v<Pieces> && ...)) {
            // if the pieces are all sliding
            return any_of(offsets, [&](auto const a) {
                for (auto j = square_idx + a; coord::valid(j); j += a) {
                    auto const p = pieces[j];
                    if (p == piece::none) { continue; }
                    if (c != colors[j]) { break; }
                    return is<Pieces...>(p);
                }
                return false;
            });
        } else {
            // else if the pieces are not sliding
            return any_of(offsets, [&](auto const a) {
                if (auto j = square_idx + a; coord::valid(j)) {
                    return c == colors[j] && is<Pieces...>(pieces[j]);
                }
                return false;
            });
        }
    }

    // private state
    board_state state_;

    public:
    static constexpr std::array pawn_offsets  {+15, +16, +17, +32};
    static constexpr std::array knight_offsets{+33, +31, +18, +14, -33, -31, -18, -14};
    static constexpr std::array bishop_offsets{-17, -15, +15, +17};
    static constexpr std::array rook_offsets  {-16, -1, +1, +16};
    static constexpr std::array queen_offsets {-17, -15, +15, +17, -16, -1, +1, +16};
    static constexpr std::array king_offsets  {-17, -16, -15, -1, +1, +15, +16, +17};

    board() = default;

    explicit board(std::string_view fen) {
        *this = fen::read(fen);
    }

    auto operator==(board const& b) const -> bool {
        return state_ == b.state_ && pieces == b.pieces && colors == b.colors;
    }

    auto reset() -> void
    {
        pieces = encoding::default_pieces();
        colors = encoding::default_colors();
    }

    auto clear() -> void
    {
        for (auto i = 0UL; i < encoding::sz; ++i) {
            pieces[i] = piece::none;
            colors[i] = color::none;
        }
    }

    auto is_attacked(u8 i, side_to_move s) const -> bool
    {
        // if a piece of the same color is on square i,
        // then return false (cannot attack our own piece)
        using enum piece;

        // pawn attacks
        auto const pawn_capture_offsets = s == side_to_move::white
            ? std::array{coord::sw, coord::se}
            : std::array{coord::nw, coord::ne};

        return attacked_by<pawn>         (pawn_capture_offsets, i, s) ||
               attacked_by<knight>       (knight_offsets, i, s)       ||
               attacked_by<bishop, queen>(bishop_offsets, i, s)       ||
               attacked_by<rook, queen>  (rook_offsets, i, s);
    }

    auto is_attacked(square sq, side_to_move s) const -> bool
    {
        return is_attacked(me::enum_integer(sq), s);
    }

    auto is_king_in_check() const -> bool {
        auto [me, enemy] = white_to_move() ? std::tuple{state_.white_king, state_.black_king}
                                           : std::tuple{state_.black_king, state_.white_king};
        if (std::abs(coord::file(me)-coord::file(enemy)) <= 1 &&
            std::abs(coord::rank(me)-coord::rank(enemy)) <= 1)
        {
            return true;
        }

        auto const s = white_to_move() ? side_to_move::black : side_to_move::white;
        auto const k = white_to_move() ? state_.white_king : state_.black_king;
        return is_attacked(k, s);
    }

    auto swap(u8 src, u8 tgt) {
        std::swap(pieces[src], pieces[tgt]);
        std::swap(colors[src], colors[tgt]);
    }

    inline auto operator[](int i) const -> std::pair<piece, color>
    {
        return {pieces[i], colors[i]};
    }

    inline auto operator[](square sq) const -> std::pair<piece, color>
    {
        return (*this)[me::enum_integer(sq)];
    }

    auto import_fen(std::string_view fen) -> void { *this = fen::read(fen); }
    auto export_fen() const -> std::string { return fen::write(*this); }

    auto state() const -> board_state const& { return state_; }
    auto state() -> board_state& { return state_; }

    auto side() const -> side_to_move { return state_.side; }
    auto side() -> side_to_move& { return state_.side; }
    auto side(side_to_move si) -> void { state_.side = si; }

    auto enpassant() const -> square { return state_.enpassant; }
    auto enpassant(square sq) -> void { state_.enpassant = sq; }

    auto castling() const -> castling_rights { return state_.castling; }
    auto castling(castling_rights cs) -> void { state_.castling = cs; }

    auto white_to_move() const -> bool { return state_.side == side_to_move::white; }
    auto black_to_move() const -> bool { return state_.side == side_to_move::black; }

    std::array<piece, encoding::sz> pieces {encoding::default_pieces()};
    std::array<color, encoding::sz> colors {encoding::default_colors()};

    // printing functions
    auto print() const -> void;
};  // board

}  // namespace chesslib

#endif
