#ifndef CHESSLIB_UTIL_FEN_HPP
#define CHESSLIB_UTIL_FEN_HPP

#include <string_view>

namespace chesslib {
    class board;

namespace fen {
    enum fen_record {
        piece_placement = 0,   // piece placement data
        active_color,          // the side to move ("w" = white, "b" = black)
        castling_availability, // castling flags ("KQkq" or "-" if neither side can castle)
        en_passant_target,     // square over which a pawn has just passed while moving two squares, or "-" for none
        halfmove_clock,        // number of halfmoves since the last capture or pawn advance, used for the fifty-move rule
        fullmove_number        // the number of full moves, it starts at 1 and is incremented after black's move
    };

    auto read(std::string_view fen) -> board;
    auto write(board const& b) -> std::string;
} // namespace fen
} // namespace chesslib

#endif