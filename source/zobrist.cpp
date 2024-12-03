#include "chesslib/core/zobrist.hpp"
#include "chesslib/board/board.hpp"

namespace chesslib::zobrist {
    auto hasher::operator()(chesslib::board const& b) const -> uint64_t {
        auto h{0UL};

        auto const& s = b.state();
        for (auto k = 0; k < encoding::length; ++k) {
            if (!coord::valid(k)) { continue; }
            if (b.pieces[k] == piece::none) { continue; }
            auto const [p, c] = b[k];

            auto const i = static_cast<int>(c); // color index
            auto const j = static_cast<int>(p); // piece index
            ASSERT(i < n_sides);
            ASSERT(j < n_pieces);
            ASSERT(k < n_squares);
            h ^= ptable(i, j, k);
        }

        if (static_cast<u8>(s.castling)) {
            h ^= ctable(static_cast<int>(s.castling));
        }

        if (s.enpassant != square::none) {
            h ^= etable(coord::file(s.enpassant));
        }

        if (b.black_to_move()) {
            h ^= side;
        }
        return h;
    }
} // namespace chesslib::zobrist