#ifndef CHESSLIB_BOARD_MOVE_GENERATOR_HPP
#define CHESSLIB_BOARD_MOVE_GENERATOR_HPP

#include <array>
#include "chesslib/core/types.hpp"
#include "chesslib/board/encoding.hpp"


namespace chesslib {

namespace helpers {


} // namespace helpers

struct move_generator {
    auto moves(move_list& m) const {
        auto const side     = board.white_to_move();
        auto const castling = board.castling();

        auto const& pieces  = board.pieces;
        auto const& colors  = board.colors;

        // clear the move list
        m.clear();

        auto add_move = [&m](u8 source, u8 target, u8 promotion, u8 capture, u8 double_pawn, u8 castling) {
            m.push_back({
                .source_square = source,
                .target_square = target,
                .promotion     = promotion,
                .capture       = capture,
                .double_pawn   = double_pawn,
                .castling      = castling
            });
        };

        auto add_promotions = [&m,&add_move](u8 src, u8 tgt, u8 cap, u8 dbl, u8 cst) {
            for (auto x : {piece::knight, piece::bishop, piece::rook, piece::queen}) {
                add_move(src, tgt, static_cast<u8>(x), cap, dbl, cst);
            }
        };

        auto ready_to_promote = [&](auto i) {
            auto [a, b] = side ? std::tuple{square::a7, square::h7}
                               : std::tuple{square::a2, square::h2};
            return i >= a && i <= b;
        };

        auto is_on_start = [&](auto i) {
            auto [a, b] = side ? std::tuple{square::a2, square::h2}
                               : std::tuple{square::a7, square::h7};
            return i >= a && i <= b;
        };

        auto add_sliding = [&](auto const& offsets, auto i) {
            auto const c = board.colors[i];
            for (auto o : offsets) {
                for (u8 j = i + o; coord::valid(j) ; j += o) {
                    if (pieces[j] != piece::none) { break; }
                    auto [q, d] = board[j];
                    if (c == d) { continue; }
                    add_move(i, j, 0, (q != piece::none && d != c), 0, 0);
                }
            }
        };

        for (u8 i = 0; i < detail_0x88::sz; ++i) {
            // get source square piece and color
            auto const [p, c] = board[i];
            if (p == piece::none) { continue; }

            auto mycolor = side ? color::white : color::black;
            if (c != mycolor) {
                continue;
            }

            switch(p) {
                case piece::pawn: {
                    // define the possible directions
                    auto [push_once, push_twice, capture_left, capture_right] = side
                        ? std::tuple{coord::no, coord::nn, coord::nw, coord::ne}
                        : std::tuple{coord::so, coord::ss, coord::sw, coord::se};

                    // use scopes to make the code nicer to read
                    // pawn pushes
                    {
                        u8 j = i + push_once;
                        auto [q, d] = board[j];
                        if (q == piece::none) {
                            if (ready_to_promote(i)) { add_promotions(i, j, 0, 0, 0); }
                            else                     { add_move(i, j, 0, 0, 0, 0); }
                        }

                        // check if I can push twice
                        if (is_on_start(i)) {
                            u8 k = i + push_twice;
                            auto [r, e] = board[k];
                            if (q == piece::none && r == piece::none) {
                                add_move(i, k, 0, 0, 1, 0);
                            }
                        }
                    }

                    // pawn captures
                    {
                        for (u8 j : {i + capture_left, i + capture_right}) {
                            auto [q, d] = board[j];
                            // can I capture a piece of the opposite color?
                            // or, can I capture en-passant?
                            if ((q != piece::none && c != d) || (q == piece::none && board.enpassant() == j)) {
                                if (ready_to_promote(i)) { add_promotions(i, j, 1, 0, 0); }
                                else                     { add_move(i, j, 0, 1, 0, 0); }
                            }
                        }
                    }
                    break;
                }

                case piece::knight: {
                    for (auto o : board::knight_offsets) {
                        u8 const j = i + o;
                        // check if target square is on the board
                        if (!coord::valid(j)) { continue; }

                        // get target square piece q and color c
                        auto const [q, d] = board[j];
                        if (c == d) { continue; } // cannot capture own piece

                        // the move is legal, we add it to the list
                        add_move(i , j, 0, (q != piece::none && d != c) ? u8{1} : u8{0}, 0, 0);
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
                    auto [kingside, queenside] = side ? std::tuple{castle::wk, castle::wq}
                                                      : std::tuple{castle::bk, castle::bq};

                    // if I have the right to castle, that means that the king and rook are on the initial squares
                    // but i still need to check two things:
                    // 1. the path is clear (no pieces occupy the squares between king and rook)
                    // 2. the king does not move through check

                    using namespace me::bitwise_operators;
                    // check if the king is on the initial square
                    // can I castle on the king side?
                    enum side const other_side = side ? side::black : side::white;
                    if (me::enum_integer(castling & kingside) != 0) {
                        auto path = side ? std::array{square::f1, square::g1}
                                         : std::array{square::f8, square::g8};

                        // check if there is something in the way or the square is attacked
                        if (std::ranges::all_of(path, [&](auto j) {
                            return pieces[j] == piece::none && !board.is_attacked(j, other_side);
                        })) {
                            auto j = me::enum_integer(path.back());
                            add_move(i, j, 0, 0, 0, me::enum_integer(kingside));
                        }
                    }
                    if (me::enum_integer(castling & queenside) != 0) {
                        auto path = side ? std::array{square::d1, square::c1}
                                         : std::array{square::d8, square::c8};
                        if (std::ranges::all_of(path, [&](auto j) {
                            return pieces[j] == piece::none && !board.is_attacked(j, other_side);
                        })) {
                            auto j = me::enum_integer(path.back());
                            add_move(i, j, 0, 0, 0, me::enum_integer(queenside));
                        }
                    }
                    // regular king moves
                    for (auto o : board::king_offsets) {
                        u8 j = i + o;
                        auto [q, d] = board[j];
                        if (coord::valid(j) && (q == piece::none || d != c) && !board.is_attacked(j, other_side)) {
                            add_move(i, j, 0, q != piece::none, 0, 0);
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

    board& board; // NOLINT

}; // generator
}  // namespace chesslib

#endif