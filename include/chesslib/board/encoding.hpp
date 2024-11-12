#ifndef CHESSLIB_DETAIL_0x88_HPP
#define CHESSLIB_DETAIL_0x88_HPP

#include "chesslib/core/types.hpp"

namespace chesslib::encoding {
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
} // namespace chesslib::encoding


#endif