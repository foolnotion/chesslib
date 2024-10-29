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
    static constexpr auto square(u8 rank, u8 file) { return (rank << 4U) + file; }
    static constexpr auto valid(u8 sq) { return (sq & mask) == 0; }
    static constexpr auto file(u8 sq) { return sq & 7U; }  // NOLINT
    static constexpr auto rank(u8 sq) { return sq >> 4U; }
    static constexpr auto same_rank(u8 sq1, u8 sq2) { return rank(sq1) == rank(sq2); }
    static constexpr auto same_file(u8 sq1, u8 sq2) { return file(sq1) == file(sq2); }
};

// forward declaration of the fen parser
struct fen_parser;

class board
{
    friend struct fen_parser;

    template<bool Sliding = false, piece... Pieces>
    struct attack_checker {
        board const& board;

        auto operator()(auto const& offsets, auto square_idx, auto side) {
            if (!coord::valid(square_idx)) { return false; }
            auto c = side == side::white ? color::white : color::black;
            return std::ranges::any_of(offsets, [&](auto a) {
                if (auto j = square_idx + a; coord::valid(j)) {
                    auto const [p, d] = board[j];
                    return ((p == Pieces) || ...) && c == d;
                }
                return false;
            });
        }
    };

    template<piece... Pieces>
    struct attack_checker<true, Pieces...> {
        board const& board;

        auto operator()(auto const& offsets, auto square_idx, auto side) {
            if (!coord::valid(square_idx)) { return false; }
            auto c = side == side::white ? color::white : color::black;
            return std::ranges::any_of(offsets, [&](auto a) {
                for (auto j = square_idx + a; coord::valid(j); j += a) {
                    auto const [p, d] = board[j];
                    if (((p == Pieces) || ...) && c == d) {
                        return true;
                    }
                    if (p != piece::none) { break; }
                }
                return false;
            });
        }
    };

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

    auto is_attacked(u8 i, side s) const -> bool
    {
        // if a piece of the same color is on square i,
        // then return false (cannot attack our own piece)
        auto c = s == side::white ? color::white : color::black;
        using enum piece;

        // pawn attacks
        auto pawn_capture_offsets = s == side::white
            ? std::array{coord::sw, coord::se}
            : std::array{coord::nw, coord::ne};

        return attack_checker</*sliding=*/false, pawn>{*this}(pawn_capture_offsets, i, s) ||
               attack_checker</*sliding=*/false, knight>{*this}(knight_offsets, i, s) ||
               attack_checker</*sliding=*/true , bishop, queen>{*this}(bishop_offsets, i, s) ||
               attack_checker</*sliding=*/true , rook, queen>{*this}(rook_offsets, i, s) ||
               attack_checker</*sliding=*/false, king>{*this}(king_offsets, i, s);
    }

    auto is_attacked(square sq, side s) const -> bool
    {
        return is_attacked(me::enum_integer(sq), s);
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

    static constexpr auto get_piece(u8 square) { return square & piece_mask; }
    static constexpr auto get_color(u8 square) { return square & color_mask; }

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

    }

    auto export_fen() const -> std::string
    {
        for (auto s = 0; s < detail_0x88::sz; ++s) {
        }

        return {};
    }

    auto print() const -> void
    {
        auto constexpr n = coord::nrow;
        for (auto i = n - 1; i >= 0; --i) {
            for (auto j = 0; j < n; ++j) {
                auto const s = coord::square(i, j);
                auto const p = me::enum_integer(pieces[s]);
                const auto* c = piece_symbols[colors[s] == color::white ? 0 : 1][p];
                fmt::print(fmt::bg((coord::file(s) + coord::rank(s)) % 2 == 0
                                       ? fmt::color::dim_gray
                                       : fmt::color::gray), "{} ", c);
            }
            fmt::print("\n");
        }
    }

    auto side() const -> side { return side_; }
    auto side() -> enum side& { return side_; }
    auto side(enum side si) -> void { side_ = si; }

    auto enpassant() const -> square { return enpassant_; }
    auto enpassant(square sq) -> void { enpassant_ = sq; }

    auto castling() const -> castle { return castle_; }
    auto castling(castle cs) -> void { castle_ = cs; }

    auto white_to_move() const -> bool { return side_ == side::white; }
    auto black_to_move() const -> bool { return side_ == side::black; }

    std::array<piece, detail_0x88::sz> pieces {detail_0x88::default_pieces()};
    std::array<color, detail_0x88::sz> colors {detail_0x88::default_colors()};

    private:
    static constexpr auto color_mask = u32{0x80};
    static constexpr auto piece_mask = ~color_mask;

    // board state
    enum side side_{side::white};          // false: white, true: black
    enum square enpassant_ {square::none}; // enpassant column (0..7, -1=uninitialized)
    enum castle castle_{castle::wk | castle::bk | castle::wq | castle::bq}; // castling rights
    i32 ply_{0}; // half-move counter, see: https://en.wikipedia.org/wiki/Ply_(game_theory)
    i32 movecount_{0}; // full-move counter
};  // board

}  // namespace chesslib

#endif
