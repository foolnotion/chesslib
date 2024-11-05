#ifndef CHESSLIB_BOARD_0X88_HPP
#define CHESSLIB_BOARD_0X88_HPP

#include <array>

#include <fmt/color.h>
#include <fmt/core.h>
#include <libassert/assert.hpp>

#include "chesslib/core/types.hpp"
#include "chesslib/util/fen.hpp"

namespace me = magic_enum;
using namespace me::bitwise_operators; // NOLINT

namespace chesslib
{

namespace detail_0x88
{
constexpr auto sz {128UL};

constexpr auto default_pieces() -> std::array<piece, sz>
{
    constexpr auto k = piece::king;
    constexpr auto q = piece::queen;
    constexpr auto r = piece::rook;
    constexpr auto b = piece::bishop;
    constexpr auto n = piece::knight;
    constexpr auto p = piece::pawn;
    constexpr auto x = piece::none;

    return {r, n, b, q, k, b, n, r, x, x, x, x, x, x, x, x,
            p, p, p, p, p, p, p, p, x, x, x, x, x, x, x, x,
            x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x,
            x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x,
            x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x,
            x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x,
            p, p, p, p, p, p, p, p, x, x, x, x, x, x, x, x,
            r, n, b, q, k, b, n, r, x, x, x, x, x, x, x, x};
}

constexpr auto default_colors() -> std::array<color, sz>
{
    constexpr auto w = color::white;
    constexpr auto b = color::black;
    constexpr auto x = color::none;

    return {w, w, w, w, w, w, w, w, x, x, x, x, x, x, x, x,
            w, w, w, w, w, w, w, w, x, x, x, x, x, x, x, x,
            x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x,
            x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x,
            x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x,
            x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x,
            b, b, b, b, b, b, b, b, x, x, x, x, x, x, x, x,
            b, b, b, b, b, b, b, b, x, x, x, x, x, x, x, x};
}
}  // namespace detail_0x88

/*
    useful values for moving from square to square.
    consider the following map. in the 0x88 encoding (board size 8x16), the
    following holds:
    ┌─────┬─────┬─────┐     ┌─────┬─────┬─────┐
    │ NW  │  N  │ NE  │     │ +15 │ +16 │ +17 │
    ├─────┼─────┼─────┤     ├─────┼─────┼─────┤
    │  W  │  X  │  E  │ ==> │  -1 │  X  │  +1 │
    ├─────┼─────┼─────┤     ├─────┼─────┼─────┤
    │ SW  │  S  │ SE  │     │ -17 │ -16 │ -15 │
    └─────┴─────┴─────┘     └─────┴─────┴─────┘

    also see: https://www.chessprogramming.org/Direction
*/
struct coord
{
    static constexpr u8 nrow{8};
    static constexpr u8 ncol{16};
    static constexpr u8 mask{0x88};

    // navigation offsets
    static constexpr i8 nw{+15};  // north-west
    static constexpr i8 no{+16};  // north
    static constexpr i8 ne{+17};  // north-east

    static constexpr i8 sw{-17};  // south-west
    static constexpr i8 so{-16};  // south
    static constexpr i8 se{-15};  // south-east

    static constexpr i8 we{-1};  // west
    static constexpr i8 ea{+1};  // east

    static constexpr i8 nn{no + no};  // north+north
    static constexpr i8 ss{so + so};  // south+south
    static constexpr i8 ww{we + we};  // west+west
    static constexpr i8 ee{ea + ea};  // east+east

    // rank identifiers
    static constexpr u8 r1{square::a1 >> 4U};
    static constexpr u8 r2{square::a2 >> 4U};
    static constexpr u8 r3{square::a3 >> 4U};
    static constexpr u8 r4{square::a4 >> 4U};
    static constexpr u8 r5{square::a5 >> 4U};
    static constexpr u8 r6{square::a6 >> 4U};
    static constexpr u8 r7{square::a7 >> 4U};
    static constexpr u8 r8{square::a8 >> 4U};

    // file identifiers
    static constexpr u8 f1{square::a1 & 7U};
    static constexpr u8 f2{square::b1 & 7U};
    static constexpr u8 f3{square::c1 & 7U};
    static constexpr u8 f4{square::d1 & 7U};
    static constexpr u8 f5{square::e1 & 7U};
    static constexpr u8 f6{square::f1 & 7U};
    static constexpr u8 f7{square::g1 & 7U};
    static constexpr u8 f8{square::h1 & 7U};

    // methods
    static constexpr auto square_index(u8 rank, u8 file) { return (rank << 4U) + file; }
    static constexpr auto valid(u8 sq) { return (sq & mask) == 0; }
    static constexpr auto file(u8 sq) { return sq & 7U; }  // NOLINT
    static constexpr auto rank(u8 sq) { return sq >> 4U; }
    static constexpr auto same_rank(u8 sq1, u8 sq2) { return rank(sq1) == rank(sq2); }
    static constexpr auto same_file(u8 sq1, u8 sq2) { return file(sq1) == file(sq2); }
};

// forward declaration of the fen parser
struct fen_parser;

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

class board
{
    friend struct fen_parser;

    template<piece P>
    static constexpr bool is_sliding_v = ((P == piece::bishop) || (P == piece::rook) || (P == piece::queen));

    template<piece... Pieces>
    requires (is_sliding_v<Pieces> && ...) || (!is_sliding_v<Pieces> && ...)
    auto attacked_by(auto const& offsets, auto square_idx, auto side) const -> bool {
        if (!coord::valid(square_idx)) { return false; }
        auto c = side == side_to_move::white ? color::white : color::black;

        if constexpr ((is_sliding_v<Pieces> && ...)) {
            // if the pieces are all sliding
            return std::ranges::any_of(offsets, [&](auto a) {
                for (auto j = square_idx + a; coord::valid(j); j += a) {
                    auto const [p, d] = (*this)[j];
                    if (((p == Pieces) || ...) && c == d) {
                        return true;
                    }
                    if (p != piece::none) { break; }
                }
                return false;
            });
        } else {
            // else if the pieces are not sliding
            return std::ranges::any_of(offsets, [&](auto a) {
                if (auto j = square_idx + a; coord::valid(j)) {
                    auto const [p, d] = (*this)[j];
                    return ((p == Pieces) || ...) && c == d;
                }
                return false;
            });
        }
    }

    public:
    static constexpr std::array pawn_offsets  {+15, +16, +17, +32};
    static constexpr std::array knight_offsets{33, 31, 18, 14, -33, -31, -18, -14};
    static constexpr std::array bishop_offsets{-17, -15, +15, +17};
    static constexpr std::array rook_offsets  {-16, -1, +1, +16};
    static constexpr std::array queen_offsets {-17, -15, +15, +17, -16, -1, +1, +16};
    static constexpr std::array king_offsets  {-17, -16, -15, -1, +1, +15, +16, +17};

    board() = default;

    explicit board(std::string_view fen) {
        *this = fen_parser::parse(fen);
    }

    auto operator==(board const& b) const -> bool {
        return state_ == b.state_ && pieces == b.pieces && colors == b.colors;
    }

    auto reset() -> void
    {
        pieces = detail_0x88::default_pieces();
        colors = detail_0x88::default_colors();
    }

    auto clear() -> void
    {
        for (auto i = 0UL; i < detail_0x88::sz; ++i) {
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
               attacked_by<rook, queen>  (rook_offsets, i, s)         ||
               attacked_by<king>         (king_offsets, i, s);
    }

    auto is_attacked(square sq, side_to_move s) const -> bool
    {
        return is_attacked(me::enum_integer(sq), s);
    }

    auto is_king_in_check() const -> bool {
        auto const s = white_to_move() ? side_to_move::black : side_to_move::white;
        auto const k = white_to_move() ? state_.white_king : state_.black_king;
        return is_attacked(k, s);
    }

    auto swap(u8 src, u8 tgt) {
        std::swap(pieces[src], pieces[tgt]);
        std::swap(colors[src], colors[tgt]);
    }

    // this function will return the previous board state
    inline auto make_move(/*move to make*/move const& m) -> board_state {
        auto src = m.source_square;
        auto tgt = m.target_square;

        // get the piece that is currently moving
        auto [p, c] = (*this)[src];
        if (p == piece::none) {
            fmt::print("src: {}, tgt: {}\n", me::enum_name(static_cast<square>(src)), me::enum_name(static_cast<square>(tgt)));
            print();
            ASSERT(p != piece::none);
        }

        // swap target and destination squares & colors
        swap(src, tgt);

        if (m.capture) {
            pieces[src] = piece::none;
            colors[src] = color::none;
        }

        if (m.promotion) {
            pieces[tgt] = static_cast<piece>(m.promotion);
        }

        // make a copy of the old state
        auto const state = state_;

        auto& castling  = state_.castling;
        auto& enpassant = state_.enpassant;
        auto const side = state_.side;

        if (m.castling) {
            switch(tgt) {
                case square::g1: {
                    // rook on h1 goes to f1
                    swap(square::h1, square::f1);
                    castling &= ~(castling_rights::wk | castling_rights::wq);
                    break;
                }
                case square::c1: {
                    // rook on a1 goes to d1
                    swap(square::a1, square::d1);
                    castling &= ~(castling_rights::wk | castling_rights::wq);
                    break;
                }
                case square::g8: {
                    // rook on h8 goes to f8
                    swap(square::h8, square::f8);
                    castling &= ~(castling_rights::bk | castling_rights::bq);
                    break;
                }
                case square::c8: {
                    // rook on a8 goes to d8
                    swap(square::a8, square::d8);
                    castling &= ~(castling_rights::bk | castling_rights::bq);
                    break;
                }
                default: {
                    break;
                }
            }
        }

        // set the castle flags if rook or king move
        switch (p) {
            case piece::pawn: {
                // check if enpassant is possible and set the flag
                auto offsets = side == side_to_move::white
                    ? std::array{coord::nn + coord::we, coord::nn + coord::ea}
                    : std::array{coord::ss + coord::we, coord::ss + coord::ea};

                if (std::abs(src - tgt) == coord::nn && std::ranges::any_of(offsets, [&](auto o) {
                    auto j = src + o;
                    if (!coord::valid(j)) { return false; }
                    auto [q, d] = (*this)[j];
                    return q == piece::pawn && c != d;
                })) {
                    auto const offset = (side == side_to_move::white) ? coord::no : coord::so;
                    enpassant = static_cast<square>(src + offset);
                }
                break;
            }
            case piece::rook: {
                switch(src) {
                    case square::h1: castling &= ~(castling_rights::wk); break;
                    case square::a1: castling &= ~(castling_rights::wq); break;
                    case square::h8: castling &= ~(castling_rights::bk); break;
                    case square::a8: castling &= ~(castling_rights::bq); break;
                    default: break;
                }
                break;
            }
            case piece::king: {
                switch(src) {
                    case square::e1: castling &= ~(castling_rights::wk | castling_rights::wq);
                    case square::a8: castling &= ~(castling_rights::bk | castling_rights::bq);
                    default: break;
                }
                // update king position
                auto& k = side == side_to_move::white ? state_.white_king : state_.black_king;
                k = static_cast<square>(tgt);
                break;
            }
            default: {
                break;
            }
        }

        return state;
    }

    inline auto unmake_move(/*move to unmake:*/move const& m, /*previous state:*/ board_state const& s) {
        auto src = m.source_square;
        auto tgt = m.target_square;

        swap(src, tgt);

        if(m.promotion) {
            pieces[src] = piece::pawn;
        }

        if (m.castling) {
            // move the rook back
            switch(tgt) {
                case square::g1: swap(square::h1, square::f1); break;
                case square::c1: swap(square::a1, square::d1); break;
                case square::g8: swap(square::h8, square::f8); break;
                case square::c8: swap(square::a8, square::d8); break;
                default: break;
            }
        }

        // restore previous state
        state_ = s;
    }

    inline auto operator[](int i) const -> std::pair<piece, color>
    {
        return {pieces[i], colors[i]};
    }

    inline auto operator[](square sq) const -> std::pair<piece, color>
    {
        auto i = me::enum_integer(sq);
        return (*this)[i];
    }

    // show the bit patterns in the 0x88 encoding
    static auto print_binary() -> void
    {
        auto k = 1;
        for (u8 i = 0; i < detail_0x88::sz; ++i) {
            fmt::print("{:08b} ", i);

            if (k++ == coord::ncol) {
                k = 1;
                fmt::print("\n");
            }
        }
    }

    auto import_fen(std::string_view fen) -> void
    {
        *this = fen_parser::parse(fen);
    }

    auto export_fen() const -> std::string
    {
        return {};
    }

    auto print() const -> void
    {
        auto constexpr n = coord::nrow;
        for (auto i = n - 1; i >= 0; --i) {
            for (auto j = 0; j < n; ++j) {
                auto const s = coord::square_index(i, j);
                auto const p = me::enum_integer(pieces[s]);
                const auto* c = piece_symbols[colors[s] == color::white ? 0 : 1][p];
                fmt::print(fmt::bg((coord::file(s) + coord::rank(s)) % 2 == 0
                                       ? fmt::color::dim_gray
                                       : fmt::color::gray), "{} ", c);
            }
            fmt::print("\n");
        }
    }

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

    std::array<piece, detail_0x88::sz> pieces {detail_0x88::default_pieces()};
    std::array<color, detail_0x88::sz> colors {detail_0x88::default_colors()};

    private:
    board_state state_;
};  // board

}  // namespace chesslib

#endif
