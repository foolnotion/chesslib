#ifndef CHESSLIB_UTIL_FEN_HPP
#define CHESSLIB_UTIL_FEN_HPP

#include <cstdint>
#include <string>
#include <string_view>
#include <tl/expected.hpp>

namespace chesslib {
    class board;

namespace fen {
    enum fen_record : std::uint8_t {
        piece_placement = 0,   // piece placement data
        active_color,          // the side to move ("w" = white, "b" = black)
        castling_availability, // castling flags ("KQkq" or "-" if neither side can castle)
        en_passant_target,     // square over which a pawn has just passed while moving two squares, or "-" for none
        halfmove_clock,        // number of halfmoves since the last capture or pawn advance, used for the fifty-move rule
        fullmove_number        // the number of full moves, it starts at 1 and is incremented after black's move
    };

    enum class error : std::uint8_t {
        invalid_piece_placement, // piece placement field is malformed (bad digit, out-of-bounds square)
        invalid_active_color,    // active color field is not 'w' or 'b'
        invalid_halfmove_clock,  // halfmove clock field is not a valid integer
        invalid_fullmove_number, // fullmove number field is not a valid integer
        too_many_fields,         // more than 6 space-separated fields
    };

    struct parse_error {
        fen::error reason;
        std::string input; // the offending field as given
    };

    // Primary non-throwing entry point. Returns an error if the FEN is malformed.
    auto read(std::string_view fen) -> tl::expected<board, parse_error>;

    // Convenience wrapper — throws std::runtime_error on parse failure.
    auto read_or_throw(std::string_view fen) -> board;

    auto write(board const& b) -> std::string;
} // namespace fen
} // namespace chesslib

#endif
