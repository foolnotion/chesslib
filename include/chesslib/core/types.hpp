#ifndef CHESSLIB_CORE_TYPES_HPP
#define CHESSLIB_CORE_TYPES_HPP

#include <array>
#include <cctype>
#include <cstdint>

namespace chesslib {

// signed types
using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

// unsigned types
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

// pieces
enum class piece : u8 {
    king, queen, rook, bishop, knight, pawn, none
};

static constexpr auto char2piece(char c) {
    switch(c) {
        case 'k': return piece::king;
        case 'q': return piece::queen;
        case 'r': return piece::rook;
        case 'b': return piece::bishop;
        case 'n': return piece::knight;
        case 'p': return piece::pawn;
        default:  return piece::none;
    }
}

// colors
enum class color : u8 {
    white, black, none
};

// squares
enum square : u8 {
    a1 =   0, b1, c1, d1, e1, f1, g1, h1, // 1st row
    a2 =  16, b2, c2, d2, e2, f2, g2, h2, // 2nd row
    a3 =  32, b3, c3, d3, e3, f3, g3, h3, // 3rd row
    a4 =  48, b4, c4, d4, e4, f4, g4, h4, // 4th row
    a5 =  64, b5, c5, d5, e5, f5, g5, h5, // 5th row
    a6 =  80, b6, c6, d6, e6, f6, g6, h6, // 6th row
    a7 =  96, b7, c7, d7, e7, f7, g7, h7, // 7th row
    a8 = 112, b8, c8, d8, e8, f8, g8, h8, // 8th row
};

// castling rights
enum class castle : u8 {
    wk = 1U << 0U,
    wq = 1U << 1U,
    bk = 1U << 2U,
    bq = 1U << 3U
};

// ascii pieces for printing
static constexpr std::array piece_symbols = {
    std::array{ "♔", "♕", "♖", "♗", "♘", "♙", " " }, // white
    std::array{ "♚", "♛", "♜", "♝", "♞", "♟︎", " " }  // black
};
}  // namespace chesslib

#endif
