#ifndef CHESSLIB_BOARD_0X88_HPP
#define CHESSLIB_BOARD_0X88_HPP

#include <array>

#include <fmt/core.h>
#include <fmt/color.h>

#include <magic_enum.hpp>
#include <magic_enum_utility.hpp>

#include <libassert/assert.hpp>

#include "chesslib/core/types.hpp"

namespace me = magic_enum;

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

    return {
        r, n, b, q, k, b, n, r, x, x, x, x, x, x, x, x,
        p, p, p, p, p, p, p, p, x, x, x, x, x, x, x, x,
        x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x,
        x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x,
        x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x,
        x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x,
        p, p, p, p, p, p, p, p, x, x, x, x, x, x, x, x,
        r, n, b, q, k, b, n, r, x, x, x, x, x, x, x, x
    };
}

constexpr auto default_colors() -> std::array<color, sz>
{
    constexpr auto w = color::white;
    constexpr auto b = color::black;
    constexpr auto x = color::none;

    return {
        b, b, b, b, b, b, b, b, x, x, x, x, x, x, x, x,
        b, b, b, b, b, b, b, b, x, x, x, x, x, x, x, x,
        x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x,
        x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x,
        x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x,
        x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x,
        w, w, w, w, w, w, w, w, x, x, x, x, x, x, x, x,
        w, w, w, w, w, w, w, w, x, x, x, x, x, x, x, x
    };
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
    static constexpr u8 nrow {8};
    static constexpr u8 ncol {16};
    static constexpr u8 mask {0x88};

    // navigation offsets
    static constexpr i8 nw {+15};  // north-west
    static constexpr i8 no {+16};  // north
    static constexpr i8 ne {+17};  // north-east

    static constexpr i8 sw {-17};  // south-west
    static constexpr i8 so {-16};  // south
    static constexpr i8 se {-15};  // south-east

    static constexpr i8 we {-1};  // west
    static constexpr i8 ea {+1};  // east

    static constexpr i8 nn {no + no}; // north+north
    static constexpr i8 ss {so + so}; // south+south
    static constexpr i8 ww {we + we}; // west+west
    static constexpr i8 ee {ea + ea}; // east+east

    // rank identifiers
    static constexpr u8 r1 {square::a1 >> 4U};
    static constexpr u8 r2 {square::a2 >> 4U};
    static constexpr u8 r3 {square::a3 >> 4U};
    static constexpr u8 r4 {square::a4 >> 4U};
    static constexpr u8 r5 {square::a5 >> 4U};
    static constexpr u8 r6 {square::a6 >> 4U};
    static constexpr u8 r7 {square::a7 >> 4U};
    static constexpr u8 r8 {square::a8 >> 4U};

    // file identifiers
    static constexpr u8 f1 {square::a1 & 7U};
    static constexpr u8 f2 {square::b1 & 7U};
    static constexpr u8 f3 {square::c1 & 7U};
    static constexpr u8 f4 {square::d1 & 7U};
    static constexpr u8 f5 {square::e1 & 7U};
    static constexpr u8 f6 {square::f1 & 7U};
    static constexpr u8 f7 {square::g1 & 7U};
    static constexpr u8 f8 {square::h1 & 7U};

    // methods
    static constexpr auto square(u8 rank, u8 file) { return (rank << 4U) + file; }
    static constexpr auto valid(u8 sq) { return (sq & mask) == 0; }
    static constexpr auto file(u8 sq) { return sq & 7U; }  // NOLINT
    static constexpr auto rank(u8 sq) { return sq >> 4U; }
    static constexpr auto same_rank(u8 sq1, u8 sq2)
    {
        return rank(sq1) == rank(sq2);
    }
    static constexpr auto same_file(u8 sq1, u8 sq2)
    {
        return file(sq1) == file(sq2);
    }
};

class board
{
  public:
    board() = default;

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

    static constexpr auto get_piece(u8 square) { return square & piece_mask; }
    static constexpr auto get_color(u8 square) { return square & color_mask; }

    // show the bit patterns in the 0x88 encoding
    static auto print_binary()  -> void
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

    auto import_fen(std::string_view fen)
    {
        clear();
        u8 rank = 0;
        u8 sq   = 0;

        for (char c : fen) {
            if (std::isspace(c)) {
                break;
                // ignore for now:
                // - side to move
                // - castling rights
                // - enpassant
                // - halfmove clock
                // - fullmove number
            }
            if (c == '/') {
                continue;
            }
            if (std::isdigit(c) != 0) {
                auto s = sq;
                for (; s < sq + c - '0'; ++s) {
                    pieces[s] = piece::none;
                    colors[s] = color::none;
                }
                sq = s;
            } else if (std::isalpha(c) != 0) {
                auto color = (std::isupper(c) != 0) ? color::white : color::black;
                pieces[sq] = char2piece(std::tolower(c));
                colors[sq] = color;
                sq++;
            }
            if (sq % 8 == 0) {
                sq += 8;
            }
        }
    }

    auto export_fen() const -> std::string
    {
        for (auto s = 0; s < detail_0x88::sz; ++s) {

        }

        return {};
    }

    inline auto operator[](int i) const -> std::pair<piece, color> {
        return { pieces[i], colors[i] };
    }

    auto make_move(u8 from_square, u8 to_square) -> bool
    {
        pieces[to_square]   = pieces[from_square];
        pieces[from_square] = piece::none;

        colors[to_square]   = colors[from_square];
        colors[from_square] = color::none;

        return true;
    }

    auto print() const -> void
    {
        auto k = 1;
        for (u8 sq = 0; sq < detail_0x88::sz; ++sq) {
            if (!coord::valid(sq)) { continue; }

            auto p  = me::enum_integer(pieces[sq]);
            const auto *ch = piece_symbols[colors[sq] == color::white ? 0 : 1][p];

            fmt::print(fmt::bg((coord::file(sq)+coord::rank(sq)) % 2 == 0 ? fmt::color::gray : fmt::color::dim_gray), "{} ", ch);

            if (k++ == coord::nrow) {
                k = 1;
                fmt::print("\n");
            }
        }
    }

    std::array<piece, detail_0x88::sz> pieces{ detail_0x88::default_pieces() };
    std::array<color, detail_0x88::sz> colors{ detail_0x88::default_colors() };

  private:
    static constexpr auto color_mask = u32 {0x80};
    static constexpr auto piece_mask = ~color_mask;

    // board state
    bool side_{};     // false: white, true: black
    i8 enpassant_{};  // enpassant column (0..7, -1=uninitialized)
    castle castle_{}; // castling rights
}; // board

struct movegen {
    static constexpr std::array knight_offsets{33, 31, 18, 14, -33, -31, -18, -14};

    static auto valid_moves(board const& board) -> i32
    {
        auto const& pieces = board.pieces;
        auto const& colors = board.colors;

        for(auto sq = 0; sq < detail_0x88::sz; ++sq) {
            if (!coord::valid(sq)) { continue; }
            if (pieces[sq] == piece::none) { continue; }
        }

        return 0;
    }

    auto valid_moves(board const& board, u8 square) -> std::vector<u8> {
        auto const p = board.pieces[square];

        switch(p) {
            case piece::king:   return moves<piece::king>(board, square);
            case piece::queen:  return moves<piece::queen>(board, square);
            case piece::rook:   return moves<piece::rook>(board, square);
            case piece::bishop: return moves<piece::bishop>(board, square);
            case piece::knight: return moves<piece::knight>(board, square);
            case piece::pawn:   return moves<piece::pawn>(board, square);
            default: return {};
        }
    }

    template<piece P = piece::none>
    auto moves(board const& /*unused*/, u8 /*unused*/) -> std::vector<u8>{
        ASSERT(P != piece::none, "invalid piece");
    }

    template<>
    auto moves<piece::king>(board const& board, u8 square) -> std::vector<u8> {
        return {};
    }

    template<>
    auto moves<piece::queen>(board const& board, u8 square) -> std::vector<u8> {
        return {};
    }

    template<>
    auto moves<piece::rook>(board const& board, u8 square) -> std::vector<u8> {
        return {};
    }

    template<>
    auto moves<piece::bishop>(board const& board, u8 square) -> std::vector<u8> {
        return {};
    }

    template<>
    auto moves<piece::knight>(board const& board, u8 square) -> std::vector<u8> {
        std::vector<u8> result;
        result.reserve(knight_offsets.size());

        auto col = board.colors[square];
        for (auto o : knight_offsets) {
            auto const s = static_cast<u8>(square + o);
            if (!coord::valid(s)) { continue; } // square is off-board
            if (board.pieces[s] != piece::none && board.colors[s] == col) { continue; } // square is occupied
            result.push_back(s);
        }
        return result;
    }

    template<>
    auto moves<piece::pawn>(board const& board, u8 square) -> std::vector<u8> {
        return {};
    }
}; // generator

}  // namespace chesslib

#endif
