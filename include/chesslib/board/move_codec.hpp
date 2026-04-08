#ifndef CHESSLIB_BOARD_MOVE_CODEC_HPP
#define CHESSLIB_BOARD_MOVE_CODEC_HPP

#include "chesslib/board/encoding.hpp"
#include "chesslib/core/types.hpp"

// Compact 16-bit move encoding (CPW convention):
//
//   bits 15-10  from    0-63 (valid_index, remapped from 0x88)
//   bits  9-4   to      0-63 (valid_index, remapped from 0x88)
//   bits  3-0   flags
//
// Flags:
//   0000  quiet               1000  knight promotion
//   0001  double pawn push    1001  bishop promotion
//   0010  kingside castle     1010  rook promotion
//   0011  queenside castle    1011  queen promotion
//   0100  capture             1100  knight promotion + capture
//   0101  en passant          1101  bishop promotion + capture
//   0110  (reserved)          1110  rook promotion + capture
//   0111  (reserved)          1111  queen promotion + capture
//
// Decode helpers:
//   flags & 0x8  -> promotion
//   flags & 0x4  -> capture
//   flags & 0x3  -> promo piece (0=N, 1=B, 2=R, 3=Q)

namespace chesslib::codec {

using namespace encoding;

constexpr auto encode(move m) -> u16 {
    auto const from = static_cast<u16>(coord::valid_index(m.source_square));
    auto const to   = static_cast<u16>(coord::valid_index(m.target_square));

    u16 flags{};
    if (m.castling) {
        flags = (m.target_square == square::g1 || m.target_square == square::g8)
                    ? u16{0b0010}   // kingside
                    : u16{0b0011};  // queenside
    } else if (m.enpassant) {
        flags = 0b0101;
    } else if (m.double_pawn) {
        flags = 0b0001;
    } else if (m.promotion) {
        // piece enum: knight=1, bishop=2, rook=3, queen=4 -> flags bits 1-0: 0,1,2,3
        auto const promo = static_cast<u16>(m.promotion - 1);
        flags = static_cast<u16>(0b1000 | (m.capture ? 0b0100 : 0) | promo);
    } else if (m.capture) {
        flags = 0b0100;
    }

    return static_cast<u16>((from << 10) | (to << 4) | flags);
}

constexpr auto decode(u16 encoded) -> move {
    auto const from_idx = static_cast<int>((encoded >> 10) & 0x3F);
    auto const to_idx   = static_cast<int>((encoded >>  4) & 0x3F);
    auto const flags    = static_cast<int>( encoded        & 0x0F);

    // valid index (0-63) back to 0x88 square
    auto const src = static_cast<u8>(coord::square_index(from_idx >> 3, from_idx & 7));
    auto const tgt = static_cast<u8>(coord::square_index(to_idx   >> 3, to_idx   & 7));

    move m{};
    m.source_square = src;
    m.target_square = tgt;

    if (flags & 0x8) {
        m.promotion = static_cast<u8>((flags & 0x3) + 1); // +1: N=1, B=2, R=3, Q=4
        m.capture   = static_cast<u8>((flags & 0x4) != 0);
    } else {
        switch (flags) {
            case 0b0001: m.double_pawn = 1; break;
            case 0b0010: [[fallthrough]];  // kingside
            case 0b0011: m.castling    = 1; break;  // queenside
            case 0b0100: m.capture     = 1; break;
            case 0b0101: m.capture     = 1; m.enpassant = 1; break;
            default: break;
        }
    }

    return m;
}

} // namespace chesslib::codec

#endif
