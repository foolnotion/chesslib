#include "chesslib/board/board.hpp"

namespace chesslib {

auto board::print() const -> void
{
    auto constexpr n = coord::nrow;
    for (auto i = n - 1; i >= 0; --i) {
        for (auto j = 0; j < n; ++j) {
            auto const s = coord::square_index(i, j);
            auto const p = me::enum_integer(pieces[s]);
            const auto* c = piece_symbols[colors[s] == color::white ? 0 : 1][p];
            fmt::print(fmt::bg((coord::file(s) + coord::rank(s)) % 2 == 0
                                    ? fmt::color::dim_gray
                                    : fmt::color::gray), "{} ", c);
        }
        fmt::print("\n");
    }
}

auto move_maker::make() -> void {
    auto& state = board_.state();
    auto& castling  = state.castling;
    auto& enpassant = state.enpassant;

    // make a copy of the old state
    state_ = board_.state();

    auto& pieces = board_.pieces;
    auto& colors = board_.colors;
    auto const white_to_move = board_.white_to_move();

    auto src = move_.source_square;
    auto tgt = move_.target_square;

    // get the piece that is currently moving
    auto [p, c] = board_[src];
    ASSERT(p != piece::none);

    // swap target and destination squares & colors
    if (move_.capture) {
        auto sq = move_.enpassant
            ? src + (coord::file(src) < coord::file(tgt) ? +1 : -1)
            : tgt;

        capture_info_ = {sq, pieces[sq], colors[sq] };

        if (pieces[sq] == piece::rook) {
            // capturing a rook removes castling rights
            if (white_to_move) {
                if (tgt == square::h8) { state.castling &= ~castling_rights::bk; }
                if (tgt == square::a8) { state.castling &= ~castling_rights::bq; }
            } else {
                if (tgt == square::h1) { state.castling &= ~castling_rights::wk; }
                if (tgt == square::a1) { state.castling &= ~castling_rights::wq; }
            }
        }

        pieces[sq] = piece::none;
        colors[sq] = color::none;
    }

    board_.swap(src, tgt);

    if (move_.promotion) {
        pieces[tgt] = static_cast<piece>(move_.promotion);
    }



    auto const white_no_castling = ~(castling_rights::wk | castling_rights::wq);
    auto const black_no_castling = ~(castling_rights::bk | castling_rights::bq);

    enpassant = square::none;

    if (move_.castling) {
        castling &= (white_to_move ? white_no_castling : black_no_castling);
        switch(tgt) {
            case square::g1: board_.swap(square::h1, square::f1); break;
            case square::g8: board_.swap(square::h8, square::f8); break;
            case square::c1: board_.swap(square::a1, square::d1); break;
            case square::c8: board_.swap(square::a8, square::d8); break;
            default: break;
        }
    }

    switch (p) {
        case piece::pawn: {
            // check if enpassant is possible and set the flag
            auto offsets = white_to_move
                ? std::array{coord::nn + coord::we, coord::nn + coord::ea}
                : std::array{coord::ss + coord::we, coord::ss + coord::ea};

            if (std::abs(src - tgt) == coord::nn && std::ranges::any_of(offsets, [&](auto o) {
                auto j = src + o;
                if (!coord::valid(j)) { return false; }
                auto [q, d] = board_[j];
                return q == piece::pawn && c != d;
            })) {
                auto const offset = white_to_move ? coord::no : coord::so;
                enpassant = static_cast<square>(src + offset);
            }
            break;
        }
        // set the castle flags if rook or king move
        case piece::rook: {
            switch(src) {
                case square::h1: castling &= ~(castling_rights::wk); break;
                case square::a1: castling &= ~(castling_rights::wq); break;
                case square::h8: castling &= ~(castling_rights::bk); break;
                case square::a8: castling &= ~(castling_rights::bq); break;
                default: break;
            }
            break;
        }
        case piece::king: {
            switch(src) {
                case square::e1: castling &= white_no_castling; break;
                case square::e8: castling &= black_no_castling; break;
                default: break;
            }
            // update king position
            auto& k = white_to_move ? state.white_king : state.black_king;
            k = static_cast<square>(tgt);
            break;
        }
        default: {
            break;
        }
    }
}

auto move_maker::undo() -> void {
    auto src = move_.source_square;
    auto tgt = move_.target_square;
    board_.swap(src, tgt);

    if(move_.promotion) {
        board_.pieces[src] = piece::pawn;
    }

    if (move_.castling) {
        switch(tgt) {
            case square::g1: board_.swap(square::h1, square::f1); break;
            case square::c1: board_.swap(square::a1, square::d1); break;
            case square::g8: board_.swap(square::h8, square::f8); break;
            case square::c8: board_.swap(square::a8, square::d8); break;
            default: break;
        }
    }

    if (move_.capture) {
        auto [s, p, c] = capture_info_;
        board_.pieces[s] = p;
        board_.colors[s] = c;
    }

    // restore old state
    board_.state() = state_;
    capture_info_ = {square::none, piece::none, color::none};
    state_        = {};
}

auto move_maker::check() -> bool {
    make();
    auto result = board_.is_king_in_check();
    undo();
    return result;
}
}  // namespace chesslib