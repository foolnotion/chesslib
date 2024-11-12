#ifndef CHESSLIB_BOARD_MOVE_GENERATOR_HPP
#define CHESSLIB_BOARD_MOVE_GENERATOR_HPP

#include <array>
#include "chesslib/board/board.hpp"


namespace chesslib {

namespace helpers {
} // namespace helpers

struct move_generator {
    using coord = encoding::coord;

    board& b; // NOLINT

    auto moves(move_list& m) const {
        auto const side     = b.white_to_move();
        auto const castling = b.castling();
        auto const& pieces  = b.pieces;

        auto const in_check = b.is_king_in_check();

        // clear the move list
        m.clear();

        auto add_move = [&](u8 source, u8 target, u8 promotion, u8 capture, u8 double_pawn, u8 enpassant, u8 castling) {
            ASSERT(coord::valid(source));
            ASSERT(coord::valid(target));
            // fmt::print("{}{}-{}\n", piece_letters[0][static_cast<u8>(b.pieces[source])], me::enum_name(static_cast<square>(source)), me::enum_name(static_cast<square>(target)));
            m.push_back({
                .source_square = source,
                .target_square = target,
                .promotion     = promotion,
                .capture       = capture,
                .double_pawn   = double_pawn,
                .enpassant     = enpassant,
                .castling      = castling
            });
        };

        auto add_promotions = [&add_move](u8 src, u8 tgt, u8 cap, u8 dbl, u8 enp, u8 cst) {
            for (auto x : {piece::knight, piece::bishop, piece::rook, piece::queen}) {
                add_move(src, tgt, static_cast<u8>(x), cap, dbl, enp, cst);
            }
        };

        auto ready_to_promote = [&](auto i) {
            return side ? (i >= square::a7 && i <= square::h7)
                        : (i >= square::a2 && i <= square::h2);
        };

        auto is_on_start = [&](auto i) {
            return side ? (i >= square::a2 && i <= square::h2)
                        : (i >= square::a7 && i <= square::h7);
        };

        auto add_sliding = [&](auto const& offsets, auto i) {
            auto const c = b.colors[i];
            for (auto o : offsets) {
                for (u8 j = i + o; coord::valid(j) ; j += o) {
                    auto [q, d] = b[j];
                    if (c == d && q != piece::none) { break; }
                    add_move(i, j, 0, (q != piece::none && q != piece::king && d != c), 0, 0, 0);
                    if (pieces[j] != piece::none) { break; }
                }
            }
        };

        for (u8 i = 0; i < encoding::sz; ++i) {
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
                        u8 j = i + push_once;
                        if (coord::valid(j)) {
                            auto [q, d] = b[j];
                            if (q == piece::none) {
                                if (ready_to_promote(i)) { add_promotions(i, j, 0, 0, 0, 0); }
                                else                     { add_move(i, j, 0, 0, 0, 0, 0); }
                            }

                            // check if I can push twice
                            if (is_on_start(i)) {
                                u8 k = i + push_twice;
                                auto [r, e] = b[k];
                                if (q == piece::none && r == piece::none) {
                                    add_move(i, k, 0, 0, 1, 0, 0);
                                }
                            }
                        }
                    }

                    // pawn captures
                    {
                        for (u8 j : {i + capture_left, i + capture_right}) {
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
                        u8 const j = i + o;
                        // check if target square is on the board
                        if (!coord::valid(j)) { continue; }

                        // get target square piece q and color c
                        auto const [q, d] = b[j];
                        if (c == d) { continue; } // cannot capture own piece

                        // the move is legal, we add it to the list
                        add_move(i , j, 0, (q != piece::none && d != c) ? u8{1} : u8{0}, 0, 0, 0);
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
                    b.pieces[i] = piece::none; // take the king off the board in order to get all attacks properly
                    auto other_side = side ? side_to_move::black : side_to_move::white;

                    if (!in_check) {
                        auto [kingside, queenside] = side ? std::tuple{castling_rights::wk, castling_rights::wq}
                                                        : std::tuple{castling_rights::bk, castling_rights::bq};

                        // if I have the right to castle, that means that the king and rook are on the initial squares
                        // but i still need to check two things:
                        // 1. the path is clear (no pieces occupy the squares between king and rook)
                        // 2. the king does not move through check

                        // in order to correctly detect attacks from pieces whose "ray" is blocked by the king itself,
                        // we remove the king from the board before checking for attacks, for example:
                        // ♜-o-o-o-♔-x
                        // (here, the square marked "x" would still be in check from the rook)


                        // check if the king is on the initial square
                        // can I castle on the king side?
                        if (me::enum_integer(castling & kingside) != 0) {
                            auto path = side ? std::array{square::f1, square::g1}
                                            : std::array{square::f8, square::g8};

                            // check if there is something in the way or the square is attacked
                            if (std::ranges::all_of(path, [&](auto j) {
                                return pieces[j] == piece::none && !b.is_attacked(j, other_side);
                            })) {
                                auto j = me::enum_integer(path.back());
                                add_move(i, j, 0, 0, 0, 0, 1);
                            }
                        }
                        if (me::enum_integer(castling & queenside) != 0) {
                            auto path = side ? std::array{square::d1, square::c1}
                                            : std::array{square::d8, square::c8};

                            auto rook_can_move = pieces[side ? square::b1 : square::b8] == piece::none;
                            if (rook_can_move && std::ranges::all_of(path, [&](auto j) {
                                return pieces[j] == piece::none && !b.is_attacked(j, other_side);
                            })) {
                                auto j = me::enum_integer(path.back());
                                add_move(i, j, 0, 0, 0, 0, 1);
                            }
                        }
                    } // if in_check

                    // regular king moves
                    for (auto o : board::king_offsets) {
                        u8 j = i + o;
                        if (!coord::valid(j)) { continue; }
                        auto [q, d] = b[j];
                        if ((q == piece::none || d != c) && !b.is_attacked(j, other_side)) {
                            add_move(i, j, 0, q != piece::none, 0, 0, 0);
                        }
                    }

                    b.pieces[i] = p; // put the king back on the board
                    break;
                }

                default: {
                    break;
                }
            }
        }
    }
}; // generator
}  // namespace chesslib

#endif