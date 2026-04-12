#ifndef CHESSLIB_ZOBRIST_HPP
#define CHESSLIB_ZOBRIST_HPP

#include <cstdint>

namespace chesslib {
class board;

namespace zobrist {

struct hasher {
    auto operator()(board const& b) const -> uint64_t;
    static auto recompute(board const& b) -> uint64_t;

    // individual hash components — for incremental hash updates
    static auto piece(int color_idx, int piece_idx, int square_idx) -> uint64_t;
    static auto castling(int rights) -> uint64_t;
    static auto enpassant_file(int file) -> uint64_t;
    static auto side_to_move() -> uint64_t;
};

} // namespace zobrist
} // namespace chesslib
#endif
