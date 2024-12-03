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

// board debugging
auto board::diff(board const& a, board const& b) -> void {
    for (auto i = 0; i < encoding::length; ++i) {
        if (a[i] == b[i]) { continue; }

        auto const [p1, c1] = a[i];
        auto const [p2, c2] = b[i];

        auto const* s1 = piece_symbols[c1 == color::white ? 0 : 1][static_cast<int>(p1)];
        auto const* s2 = piece_symbols[c2 == color::white ? 0 : 1][static_cast<int>(p2)];

        fmt::print("{}: {} ≠ {}\n", me::enum_name(static_cast<square>(i)), s1, s2);
    }

    // compare enpassant
    if (a.enpassant() != b.enpassant()) {
        fmt::print("{} ≠ {}\n", me::enum_name(a.enpassant()), me::enum_name(b.enpassant()));
    }

    // compare castling flags
    if (a.castling() != b.castling()) {
        fmt::print("{:04b} ≠ {:04b}\n", (u8)a.castling(), (u8)b.castling());
    }

    // compare side to move
    if (a.side() != b.side()) {
        fmt::print("{} ≠ {}\n", me::enum_name(a.side()), me::enum_name(b.side()));
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
            ? enpassant + (white_to_move ? coord::so : coord::no)
            : tgt;
        ASSERT(pieces[sq] != piece::none);
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

    board_.do_move(src, tgt);

    if (move_.promotion) {
        pieces[tgt] = static_cast<piece>(move_.promotion);
    }

    auto const white_no_castling = ~(castling_rights::wk | castling_rights::wq);
    auto const black_no_castling = ~(castling_rights::bk | castling_rights::bq);

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

    enpassant = square::none;
    switch (p) {
        case piece::pawn: {
            // check if there's an enemy pawn next to my target square which could capture me en-passant
            auto check = [&](auto i) { return coord::valid(i) && pieces[i] == piece::pawn && colors[i] != c; };
            if (std::abs(tgt-src) == coord::nn && (check(tgt-1) || check(tgt+1))) {
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