#ifndef CHESSLIB_BOARD_MOVE_GENERATOR_HPP
#define CHESSLIB_BOARD_MOVE_GENERATOR_HPP

#include <array>
#include "chesslib/board/board.hpp"
#include "chesslib/core/scope_exit.hpp"

namespace chesslib {
using namespace encoding;
using namespace me::bitwise_operators; // castling_rights flag operations

namespace helpers {
} // namespace helpers

struct move_generator {

    board& b;

    auto ready_to_promote(auto i) const {
        return b.white_to_move() ? (i >= square::a7 && i <= square::h7)
                    : (i >= square::a2 && i <= square::h2);
    }

    auto is_on_start(auto i) const {
        return b.white_to_move() ? (i >= square::a2 && i <= square::h2)
                    : (i >= square::a7 && i <= square::h7);
    }

    auto moves(auto& m) const {
        auto const side     = b.white_to_move();
        auto const castling = b.castling();

        auto const in_check = b.is_king_in_check();

        // clear the move list
        m.clear();

        auto add_move = [&](int source, int target, u8 promotion, u8 capture, u8 double_pawn, u8 enpassant, u8 castling) -> void {
            m.push_back({
                .source_square = static_cast<u8>(source),
                .target_square = static_cast<u8>(target),
                .promotion     = promotion,
                .capture       = capture,
                .double_pawn   = double_pawn,
                .enpassant     = enpassant,
                .castling      = castling
            });
        };

        auto add_promotions = [&add_move](int src, int tgt, u8 cap, u8 dbl, u8 enp, u8 cst) -> void {
            for (auto x : {piece::knight, piece::bishop, piece::rook, piece::queen}) {
                add_move(src, tgt, static_cast<u8>(x), cap, dbl, enp, cst);
            }
        };

        auto add_sliding = [&](auto const& offsets, auto i) -> void {
            auto const c = b.color_at(i);
            for (auto o : offsets) {
                for (int j = i + o; coord::valid(j); j += o) {
                    auto [q, d] = b[j];
                    if (c == d && q != piece::none) { break; }
                    add_move(i, j, 0, (q != piece::none && q != piece::king && d != c), 0, 0, 0);
                    if (q != piece::none) { break; }
                }
            }
        };

        for (u8 i = 0; i < encoding::length; ++i) {
            if (!coord::valid(i)) { continue; }
            // get source square piece and color
            auto const [p, c] = b[i];
            if (p == piece::none) { continue; }

            auto mycolor = side ? color::white : color::black;
            if (c != mycolor) { continue; }

            switch(p) {
                case piece::pawn: {
                    // define the possible directions
                    auto [push_once, push_twice, capture_left, capture_right] = side
                        ? std::tuple{coord::no, coord::nn, coord::nw, coord::ne}
                        : std::tuple{coord::so, coord::ss, coord::sw, coord::se};

                    // use scopes to make the code nicer to read
                    // pawn pushes
                    {
                        int const j = i + push_once;
                        if (coord::valid(j)) {
                            auto const q = b.piece_at(j);
                            if (q == piece::none) {
                                if (ready_to_promote(i)) { add_promotions(i, j, 0, 0, 0, 0); }
                                else                     { add_move(i, j, 0, 0, 0, 0, 0); }
                            }

                            // check if I can push twice
                            if (is_on_start(i)) {
                                int const k = i + push_twice;
                                if (q == piece::none && b.piece_at(k) == piece::none) {
                                    add_move(i, k, 0, 0, 1, 0, 0);
                                }
                            }
                        }
                    }

                    // pawn captures
                    {
                        for (int const j : {i + capture_left, i + capture_right}) {
                            if (!coord::valid(j)) { continue; }
                            auto [q, d] = b[j];
                            // can I capture a piece of the opposite color?
                            // or, can I capture en-passant?
                            auto const enpassant = b.enpassant() == j;
                            if ((q != piece::none && c != d) || (q == piece::none && enpassant)) {
                                if (ready_to_promote(i)) { add_promotions(i, j, 1, 0, 0, 0); }
                                else                     { add_move(i, j, 0, 1, 0, enpassant, 0); }
                            }
                        }
                    }
                    break;
                }

                case piece::knight: {
                    for (auto o : board::knight_offsets) {
                        int const j = i + o;
                        // check if target square is on the board
                        if (!coord::valid(j)) { continue; }

                        // get target square piece q and color c
                        if (c == b.color_at(j)) { continue; } // cannot capture own piece

                        // the move is legal, we add it to the list
                        add_move(i, j, u8{0}, b.piece_at(j) != piece::none, u8{0}, u8{0}, u8{0});
                    }
                    break;
                }

                case piece::bishop: {
                    add_sliding(board::bishop_offsets, i);
                    break;
                }

                case piece::rook: {
                    add_sliding(board::rook_offsets, i);
                    break;
                }

                case piece::queen: {
                    add_sliding(board::queen_offsets, i);
                    break;
                }

                case piece::king: {
                    // regular king moves + castling
                    // castling rights flags
                    // temporarily remove the king so that is_attacked() sees through it when
                    // checking castling path squares (e.g. a rook on a8 still attacks g8 even
                    // with the king on e8 blocking the naive ray walk)
                    b.pieces_[i] = piece::none;
                    auto const restore_king = scope_exit{[&]() -> void { b.pieces_[i] = p; }};
                    auto other_side = side ? side_to_move::black : side_to_move::white;

                    if (!in_check) {
                        auto [kingside, queenside] = side ? std::tuple{castling_rights::wk, castling_rights::wq}
                                                        : std::tuple{castling_rights::bk, castling_rights::bq};

                        // Castling requires the king on its start square, the rook present on its
                        // start square, a clear path, and the king not moving through check.
                        // We verify king and rook placement explicitly rather than trusting the
                        // castling-rights bits alone, so malformed FENs cannot produce illegal moves.

                        // in order to correctly detect attacks from pieces whose "ray" is blocked by the king itself,
                        // we remove the king from the board before checking for attacks, for example:
                        // ♜-o-o-o-♔-x
                        // (here, the square marked "x" would still be in check from the rook)

                        auto const expected_king = side ? square::e1 : square::e8;
                        if (static_cast<square>(i) == expected_king) {
                            if (me::enum_integer(castling & kingside) != 0) {
                                auto const rook_sq = side ? square::h1 : square::h8;
                                auto const path    = side ? std::array{square::f1, square::g1}
                                                          : std::array{square::f8, square::g8};
                                if (b.piece_at(rook_sq) == piece::rook &&
                                    b.color_at(rook_sq) == mycolor &&
                                    std::ranges::all_of(path, [&](auto j) -> bool {
                                        return b.piece_at(j) == piece::none && !b.is_attacked(j, other_side);
                                    })) {
                                    add_move(i, me::enum_integer(path.back()), 0, 0, 0, 0, 1);
                                }
                            }
                            if (me::enum_integer(castling & queenside) != 0) {
                                auto const rook_sq      = side ? square::a1 : square::a8;
                                auto const path         = side ? std::array{square::d1, square::c1}
                                                               : std::array{square::d8, square::c8};
                                auto const rook_can_move = b.piece_at(side ? square::b1 : square::b8) == piece::none;
                                if (b.piece_at(rook_sq) == piece::rook &&
                                    b.color_at(rook_sq) == mycolor &&
                                    rook_can_move &&
                                    std::ranges::all_of(path, [&](auto j) -> bool {
                                        return b.piece_at(j) == piece::none && !b.is_attacked(j, other_side);
                                    })) {
                                    add_move(i, me::enum_integer(path.back()), 0, 0, 0, 0, 1);
                                }
                            }
                        }
                    } // if in_check

                    // regular king moves
                    for (auto o : board::king_offsets) {
                        int const j = i + o;
                        if (!coord::valid(j)) { continue; }
                        auto [q, d] = b[j];
                        if ((q == piece::none || d != c) && !b.is_attacked(j, other_side)) {
                            add_move(i, j, 0, static_cast<u8>(q != piece::none), 0, 0, 0);
                        }
                    }

                    break;
                }

                default: {
                    break;
                }
            }
        }
    }
}; // generator

inline auto legal_moves(board const& b) -> move_list {
    auto& mutable_board = const_cast<board&>(b);
    move_list pseudo;
    move_generator{mutable_board}.moves(pseudo);

    move_list legal;
    for (auto const& m : pseudo) {
        move_maker mm{mutable_board, m};
        if (!mm.check()) {
            legal.push_back(m);
        }
    }
    return legal;
}

// True when the side to move has no legal moves and their king is in check.
inline auto is_checkmate(board const& b) -> bool {
    return b.is_king_in_check() && legal_moves(b).empty();
}

// True when the side to move has no legal moves but their king is not in check.
inline auto is_stalemate(board const& b) -> bool {
    return !b.is_king_in_check() && legal_moves(b).empty();
}

}  // namespace chesslib

#endif
