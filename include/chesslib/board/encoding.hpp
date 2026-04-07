#ifndef CHESSLIB_BOARD_ENCODING_HPP
#define CHESSLIB_BOARD_ENCODING_HPP

#include "chesslib/core/types.hpp"

namespace chesslib::encoding {
constexpr auto length {128UL};

constexpr auto default_pieces() -> std::array<piece, length>
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

constexpr auto default_colors() -> std::array<color, length>
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
namespace coord
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
    constexpr auto file(int sq) -> i32 { return sq & 7; }
    constexpr auto rank(int sq) -> i32 { return sq >> 4; }
    constexpr auto file_rank(int sq) -> std::tuple<i32, i32> { return { sq & 7, sq >> 4 }; }
    constexpr auto same_rank(int sq1, int sq2) { return rank(sq1) == rank(sq2); }
    constexpr auto same_file(int sq1, int sq2) { return file(sq1) == file(sq2); }
    constexpr auto square_index(int r, int f) -> i32 { return (r << 4) + f; }
    constexpr auto valid_index(int sq) -> i32 {
        return (rank(sq) * 8) + file(sq);
    }
    constexpr auto valid(int sq) { return (static_cast<unsigned>(sq) & mask) == 0; }

}  // namespace coord
} // namespace chesslib::encoding


#endif
