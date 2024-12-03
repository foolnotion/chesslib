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

using namespace encoding;

struct board_state {
    side_to_move side{side_to_move::white};
    castling_rights castling{0b1111};
    square enpassant{square::none};
    square white_king{square::e1};
    square black_king{square::e8};
    u8 ply{0};
    u8 count{0};

    auto operator==(board_state const& s) const -> bool {
        return std::tie(side, castling, enpassant, white_king, black_king, ply, count) ==
               std::tie(s.side, s.castling, s.enpassant, s.white_king, s.black_king, s.ply, s.count);
    }
};

class board
{
    template<piece P>
    static constexpr bool is_sliding_v = (is<piece::bishop, piece::rook, piece::queen>(P));

    template<piece... Pieces>
    requires (is_sliding_v<Pieces> && ...) || (!is_sliding_v<Pieces> && ...)
    auto attacked_by(auto const& offsets, auto square_idx, auto side) const -> bool {
        if (!coord::valid(square_idx)) { return false; }
        auto const c = static_cast<color>(side);

        for (auto const i : offsets) {
            if constexpr ((is_sliding_v<Pieces> && ...)) { // sliding pieces
                for (auto j = square_idx + i; coord::valid(j); j += i) {
                    auto const p = pieces[j];
                    if (p == piece::none) { continue; }
                    if (!(c == colors[j] && is<Pieces...>(p))) { break; }
                    return true;
                }
            } else { // not sliding
                if (auto j = square_idx + i; coord::valid(j)) {
                    if (!(c == colors[j] && is<Pieces...>(pieces[j]))) { continue; }
                    return true;
                }
            }
        }
        return false;
    }

    // private state
    board_state state_;

    public:
    static constexpr std::array pawn_offsets  {+15, +16, +17, +32};
    static constexpr std::array knight_offsets{-33, -31, -18, -14, +14, +18, +31, +33};
    static constexpr std::array bishop_offsets{-17, -15, +15, +17};
    static constexpr std::array rook_offsets  {-16, -1, +1, +16};
    static constexpr std::array queen_offsets {-17, -16, -15, -1, +1, +15, +16, +17};
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
        for (auto i = 0UL; i < encoding::length; ++i) {
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
        auto const [a, b] = coord::file_rank(state_.white_king);
        auto const [c, d] = coord::file_rank(state_.black_king);
        if (std::abs(a-c) <= 1 && std::abs(b-d) <= 1) {
            return true;
        }
        return white_to_move()
            ? is_attacked(state_.white_king, side_to_move::black)
            : is_attacked(state_.black_king, side_to_move::white);
    }

    auto swap(u8 src, u8 tgt) {
        std::swap(pieces[src], pieces[tgt]);
        std::swap(colors[src], colors[tgt]);
    }

    auto do_move(u8 src, u8 tgt) {
        pieces[tgt] = std::exchange(pieces[src], piece::none);
        colors[tgt] = std::exchange(colors[src], color::none);
    }

    auto operator[](int i) const -> std::tuple<piece, color>
    {
        return {pieces[i], colors[i]};
    }

    auto operator[](square sq) const -> std::tuple<piece, color>
    {
        return (*this)[me::enum_integer(sq)];
    }

    auto import_fen(std::string_view fen) -> void { *this = fen::read(fen); }
    auto export_fen() const -> std::string { return fen::write(*this); }

    auto state() const -> board_state const& { return state_; }
    auto state() -> board_state& { return state_; }

    auto side() const -> side_to_move { return state_.side; }
    auto side() -> side_to_move& { return state_.side; }
    auto set_side(side_to_move si) -> void { state_.side = si; }

    auto enpassant() const -> square { return state_.enpassant; }
    auto set_enpassant(square sq) -> void { state_.enpassant = sq; }

    auto castling() const -> castling_rights { return state_.castling; }
    auto set_castling(castling_rights cs) -> void { state_.castling = cs; }

    auto white_to_move() const -> bool { return state_.side == side_to_move::white; }
    auto black_to_move() const -> bool { return state_.side == side_to_move::black; }

    std::array<piece, encoding::length> pieces {encoding::default_pieces()};
    std::array<color, encoding::length> colors {encoding::default_colors()};

    // printing functions
    auto print() const -> void;

    // debugging functions
    static auto diff(board const& a, board const& b) -> void;
};  // board


struct move_maker {
    explicit move_maker(board& b, move m)
        : board_{b}, state_{board_.state()}, move_{m} {}

    auto make() -> void;
    auto undo() -> void;
    auto check() -> bool;
    auto captured() const -> piece { return std::get<1>(capture_info_); }

    private:
    board& board_; // NOLINT
    board_state state_;
    move move_;
    std::tuple<u8, piece, color> capture_info_{square::none, piece::none, color::none};

    inline auto handle_capture() -> void;
    inline auto update_castling_flags() -> void;
    inline auto handle_promotion() -> void;
};

}  // namespace chesslib

#endif
