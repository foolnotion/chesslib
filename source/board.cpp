#include <cstddef>
#include <cstdlib>
#include <string>
#include <utility>

#include <fmt/base.h>
#include <fmt/color.h>
#include <libassert/assert.hpp>
#include <magic_enum/magic_enum.hpp>

#include "chesslib/board/board.hpp"
#include "chesslib/board/encoding.hpp"
#include "chesslib/core/types.hpp"
#include "chesslib/core/zobrist.hpp"

namespace chesslib {

using namespace me::bitwise_operators; // castling_rights flag operations

auto board::print(std::string indent) const -> void
{
    constexpr auto n = static_cast<int>(coord::nrow);
    for (auto i = n - 1; i >= 0; --i) {
        fmt::print("{}", indent);
        for (auto j = 0; j < n; ++j) {
            auto const s = coord::square_index(i, j);
            auto const p = static_cast<size_t>(me::enum_integer(piece_at(s)));
            const auto* c = piece_symbols[color_at(s) == color::white ? 0 : 1][p];
            fmt::print(fmt::bg((coord::file(s) + coord::rank(s)) % 2 == 0
                                    ? fmt::color::dim_gray
                                    : fmt::color::gray), "{} ", c);
        }
        fmt::print("\n");
    }
}

// board debugging
auto board::diff(board const& a, board const& b) -> void {
    for (auto i = 0; std::cmp_less(i, encoding::length); ++i) {
        if (a[i] == b[i]) { continue; }

        auto const [p1, c1] = a[i];
        auto const [p2, c2] = b[i];

        auto const* s1 = piece_symbols[c1 == color::white ? 0 : 1][static_cast<size_t>(me::enum_integer(p1))];
        auto const* s2 = piece_symbols[c2 == color::white ? 0 : 1][static_cast<size_t>(me::enum_integer(p2))];

        fmt::print("{}: {} ≠ {}\n", me::enum_name(static_cast<square>(i)), s1, s2);
    }

    // compare enpassant
    if (a.enpassant() != b.enpassant()) {
        fmt::print("{} ≠ {}\n", me::enum_name(a.enpassant()), me::enum_name(b.enpassant()));
    }

    // compare castling flags
    if (a.castling() != b.castling()) {
        fmt::print("{:04b} ≠ {:04b}\n", static_cast<u8>(a.castling()), static_cast<u8>(b.castling()));
    }

    // compare side to move
    if (a.side() != b.side()) {
        fmt::print("{} ≠ {}\n", me::enum_name(a.side()), me::enum_name(b.side()));
    }
}

auto move_maker::make() -> void {
    // save a copy of the pre-move state for undo()
    state_ = board_.state();

    // work on a mutable local copy; commit via set_state() at the end
    auto state = state_;
    auto& castling  = state.castling;
    auto& enpassant = state.enpassant;
    hash_ = board_.hash();
    u64 h = hash_;

    auto const white_to_move = board_.white_to_move();

    // move_ stores raw 0x88 indices as u8; cast to square for the public API
    auto const src = static_cast<square>(move_.source_square);
    auto const tgt = static_cast<square>(move_.target_square);
    auto const src_i = static_cast<int>(move_.source_square);
    auto const tgt_i = static_cast<int>(move_.target_square);

    // get the piece that is currently moving
    auto [p, c] = board_[src];
    ASSERT(p != piece::none);

    auto const color_index = static_cast<int>(c);
    auto const piece_index = static_cast<int>(p);

    if (static_cast<u8>(castling) != 0) {
        h ^= zobrist::hasher::castling(static_cast<int>(castling));
    }
    if (enpassant != square::none) {
        h ^= zobrist::hasher::enpassant_file(coord::file(enpassant));
    }
    h ^= zobrist::hasher::piece(color_index, piece_index, src);

    // handle capture
    if (move_.capture) {
        auto const sq = move_.enpassant
            ? static_cast<square>(me::enum_integer(enpassant) + (white_to_move ? coord::so : coord::no))
            : tgt;
        ASSERT(board_.piece_at(sq) != piece::none);
        capture_info_ = {static_cast<u8>(me::enum_integer(sq)), board_.piece_at(sq), board_.color_at(sq)};

        auto const captured_piece = board_.piece_at(sq);
        auto const captured_color = board_.color_at(sq);
        h ^= zobrist::hasher::piece(static_cast<int>(captured_color), static_cast<int>(captured_piece), sq);

        if (board_.piece_at(sq) == piece::rook) {
            // capturing a rook removes the corresponding castling right
            if (white_to_move) {
                if (tgt == square::h8) { castling &= ~castling_rights::bk; }
                if (tgt == square::a8) { castling &= ~castling_rights::bq; }
            } else {
                if (tgt == square::h1) { castling &= ~castling_rights::wk; }
                if (tgt == square::a1) { castling &= ~castling_rights::wq; }
            }
        }

        board_.remove(sq);
    }

    board_.move_piece(src, tgt);

    if (move_.promotion) {
        board_.place(tgt, static_cast<piece>(move_.promotion), c);
    }

    auto const piece_on_target = move_.promotion ? static_cast<piece>(move_.promotion) : p;
    h ^= zobrist::hasher::piece(color_index, static_cast<int>(piece_on_target), tgt);

    auto const white_no_castling = ~(castling_rights::wk | castling_rights::wq);
    auto const black_no_castling = ~(castling_rights::bk | castling_rights::bq);

    if (move_.castling) {
        castling &= (white_to_move ? white_no_castling : black_no_castling);
        switch (tgt) {
            case square::g1:
                h ^= zobrist::hasher::piece(static_cast<int>(color::white), static_cast<int>(piece::rook), square::h1);
                h ^= zobrist::hasher::piece(static_cast<int>(color::white), static_cast<int>(piece::rook), square::f1);
                board_.swap_squares(square::h1, square::f1);
                break;
            case square::g8:
                h ^= zobrist::hasher::piece(static_cast<int>(color::black), static_cast<int>(piece::rook), square::h8);
                h ^= zobrist::hasher::piece(static_cast<int>(color::black), static_cast<int>(piece::rook), square::f8);
                board_.swap_squares(square::h8, square::f8);
                break;
            case square::c1:
                h ^= zobrist::hasher::piece(static_cast<int>(color::white), static_cast<int>(piece::rook), square::a1);
                h ^= zobrist::hasher::piece(static_cast<int>(color::white), static_cast<int>(piece::rook), square::d1);
                board_.swap_squares(square::a1, square::d1);
                break;
            case square::c8:
                h ^= zobrist::hasher::piece(static_cast<int>(color::black), static_cast<int>(piece::rook), square::a8);
                h ^= zobrist::hasher::piece(static_cast<int>(color::black), static_cast<int>(piece::rook), square::d8);
                board_.swap_squares(square::a8, square::d8);
                break;
            default: break;
        }
    }

    enpassant = square::none;
    switch (p) {
        case piece::pawn: {
            // always set the en-passant target square on a double pawn push so that
            // FEN export matches the spec and position hashes are consistent with
            // external tools regardless of whether an enemy pawn can capture
            if (std::abs(tgt_i - src_i) == coord::nn) {
                auto const offset = white_to_move ? coord::no : coord::so;
                enpassant = static_cast<square>(src_i + offset);
            }
            break;
        }
        // set the castling flags if rook or king move
        case piece::rook: {
            switch (src) {
                case square::h1: castling &= ~castling_rights::wk; break;
                case square::a1: castling &= ~castling_rights::wq; break;
                case square::h8: castling &= ~castling_rights::bk; break;
                case square::a8: castling &= ~castling_rights::bq; break;
                default: break;
            }
            break;
        }
        case piece::king: {
            switch (src) {
                case square::e1: castling &= white_no_castling; break;
                case square::e8: castling &= black_no_castling; break;
                default: break;
            }
            // update king position
            auto& k = white_to_move ? state.white_king : state.black_king;
            k = tgt;
            break;
        }
        default: {
            break;
        }
    }

    // Always advance the side to move. undo() restores the full saved state,
    // so this is automatically reversed without any extra work.
    state.side = (state.side == side_to_move::white) ? side_to_move::black : side_to_move::white;

    // Update move counters. The halfmove clock resets on a pawn move or capture,
    // otherwise increments. The fullmove number increments after black's move.
    if (p == piece::pawn || move_.capture) {
        state.halfmove_clock = 0;
    } else {
        state.halfmove_clock++;
    }
    if (!white_to_move) {
        state.fullmove_number++;
    }

    board_.set_state(state);

    if (static_cast<u8>(castling) != 0) {
        h ^= zobrist::hasher::castling(static_cast<int>(castling));
    }
    if (enpassant != square::none) {
        h ^= zobrist::hasher::enpassant_file(coord::file(enpassant));
    }
    h ^= zobrist::hasher::side_to_move();
    board_.set_hash(h);
}

auto move_maker::undo() -> void {
    auto const src = static_cast<square>(move_.source_square);
    auto const tgt = static_cast<square>(move_.target_square);
    board_.swap_squares(src, tgt);

    if (move_.promotion) {
        board_.place(src, piece::pawn, board_.color_at(src));
    }

    if (move_.castling) {
        switch (tgt) {
            case square::g1: board_.swap_squares(square::h1, square::f1); break;
            case square::c1: board_.swap_squares(square::a1, square::d1); break;
            case square::g8: board_.swap_squares(square::h8, square::f8); break;
            case square::c8: board_.swap_squares(square::a8, square::d8); break;
            default: break;
        }
    }

    if (move_.capture) {
        auto [s, cp, cc] = capture_info_;
        board_.place(static_cast<square>(s), cp, cc);
    }

    board_.set_state(state_);
    board_.set_hash(hash_);
    capture_info_ = {square::none, piece::none, color::none};
    state_        = {};
    hash_         = 0;
}

// Returns true if making this move leaves the moving side's king in check.
// After make() the side has been toggled, so we use the saved pre-move side
// stored in state_ to check the correct king.
auto move_maker::check() -> bool {
    make();
    auto result = board_.is_king_in_check(state_.side);
    undo();
    return result;
}

}  // namespace chesslib
