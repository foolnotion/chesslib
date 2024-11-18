#include "chesslib/core/zobrist.hpp"
#include "chesslib/board/board.hpp"

namespace chesslib::zobrist {
    auto hasher::operator()(chesslib::board const& b) const -> uint64_t {
        auto h{0UL};

        auto const& s = b.state();
        for (auto i = 0; i < encoding::length; ++i) {
            if (!coord::valid(i)) { continue; }
            if (b.pieces[i] == piece::none) { continue; }
            auto const [p, c] = b[i];
            auto const j = coord::rank(i) * 8 + coord::file(i);
            ASSERT(j < ptable.extent(3));
            h ^= ptable(static_cast<int>(c), static_cast<int>(p), j);
        }

        if (static_cast<u8>(s.castling)) {
            h ^= ctable[static_cast<u8>(s.castling)];
        }

        if (s.enpassant != square::none) {
            h ^= etable[coord::file(s.enpassant)];
        }

        if (b.black_to_move()) {
            h ^= side;
        }
        return h;
    }
} // namespace chesslib::zobrist