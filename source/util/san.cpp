#include <cctype>
#include <string>
#include <string_view>

#include <tl/expected.hpp>

#include "chesslib/board/board.hpp"
#include "chesslib/board/encoding.hpp"
#include "chesslib/board/move_generator.hpp"
#include "chesslib/core/types.hpp"
#include "chesslib/util/san.hpp"

namespace chesslib::san {

using namespace encoding;

namespace {

constexpr auto piece_letter(piece p) -> char {
    constexpr char letters[] = {'P', 'N', 'B', 'R', 'Q', 'K', '?'};
    return letters[static_cast<int>(p)];
}

constexpr auto char_to_piece(char c) -> piece {
    switch (std::toupper(static_cast<unsigned char>(c))) {
        case 'N': return piece::knight;
        case 'B': return piece::bishop;
        case 'R': return piece::rook;
        case 'Q': return piece::queen;
        case 'K': return piece::king;
        default:  return piece::none;
    }
}

constexpr auto is_promo_letter(char c) -> bool {
    return c == 'Q' || c == 'R' || c == 'B' || c == 'N';
}

constexpr auto is_rank_digit(char c) -> bool { return c >= '1' && c <= '8'; }
constexpr auto is_file_letter(char c) -> bool { return c >= 'a' && c <= 'h'; }

auto has_any_legal_move(board const& b) -> bool {
    auto& mutable_board = const_cast<board&>(b);
    move_list pseudo;
    move_generator{mutable_board}.moves(pseudo);
    for (auto const& candidate : pseudo) {
        auto const moving_piece = b.piece_at(candidate.source_square);
        if (detail::is_move_legal(mutable_board, candidate, moving_piece)) {
            return true;
        }
    }
    return false;
}

struct disambiguation_info {
    bool any_ambig{false};
    bool same_file{false};
    bool same_rank{false};
};

auto disambiguation_flags(board const& b, move current, piece p) -> disambiguation_info {
    auto& mutable_board = const_cast<board&>(b);
    move_list pseudo;
    move_generator{mutable_board}.moves(pseudo);

    disambiguation_info info;

    for (auto const& candidate : pseudo) {
        if (candidate.source_square == current.source_square) { continue; }
        if (candidate.target_square != current.target_square) { continue; }
        if (b.piece_at(candidate.source_square) != p) { continue; }

        if (!detail::is_move_legal(mutable_board, candidate, p)) { continue; }

        info.any_ambig = true;
        if (coord::file(candidate.source_square) == coord::file(current.source_square)) { info.same_file = true; }
        if (coord::rank(candidate.source_square) == coord::rank(current.source_square)) { info.same_rank = true; }
    }

    return info;
}

auto find_san_move(board const& b, auto&& predicate) -> tl::expected<move, error> {
    auto& mutable_board = const_cast<board&>(b);
    move_list pseudo;
    move_generator{mutable_board}.moves(pseudo);

    move const* found = nullptr;
    for (auto const& candidate : pseudo) {
        if (!predicate(candidate)) { continue; }
        auto const moving_piece = b.piece_at(candidate.source_square);
        if (!detail::is_move_legal(mutable_board, candidate, moving_piece)) { continue; }
        if (found != nullptr) {
            return tl::unexpected{error::ambiguous};
        }
        found = &candidate;
    }

    if (found == nullptr) {
        return tl::unexpected{error::no_matching_move};
    }
    return *found;
}

auto find_san_move_targeted(board& b, piece p, int tgt_sq, u8 promo,
                            int disambig_file, int disambig_rank) -> tl::expected<move, error>
{
    auto candidates = detail::enumerate_candidates(b, p, tgt_sq, promo,
                                                   disambig_file, disambig_rank);

    move const* found = nullptr;
    for (int i = 0; i < candidates.size(); ++i) {
        if (!detail::is_move_legal(b, candidates[i], p)) { continue; }
        if (found != nullptr) { return tl::unexpected{error::ambiguous}; }
        found = &candidates[i];
    }

    return found ? tl::expected<move, error>{*found} : tl::unexpected{error::no_matching_move};
}

} // namespace

// ---- detail implementations (exposed for san_replay) ----

namespace detail {

auto is_move_legal(board& b, move current, piece p) -> bool {
    if (p == piece::king || current.castling) {
        move_maker mm{b, current};
        return !mm.check();
    }

    auto const saved_hash = b.hash();
    auto const saved_state = b.state();
    auto const src = static_cast<square>(current.source_square);
    auto const tgt = static_cast<square>(current.target_square);
    auto const mover_color = b.color_at(src);

    auto capture_sq = square::none;
    auto captured_piece = piece::none;
    auto captured_color = color::none;

    if (current.enpassant) {
        auto const offset = saved_state.side == side_to_move::white
            ? coord::so
            : coord::no;
        capture_sq = static_cast<square>(
            me::enum_integer(saved_state.enpassant) + offset);
    } else if (current.capture) {
        capture_sq = tgt;
    }

    if (capture_sq != square::none) {
        captured_piece = b.piece_at(capture_sq);
        captured_color = b.color_at(capture_sq);
        b.remove(capture_sq);
    }

    b.move_piece(src, tgt);
    if (current.promotion) {
        b.place(tgt, static_cast<piece>(current.promotion), mover_color);
    }

    auto const legal = !b.is_king_in_check(saved_state.side);

    if (current.promotion) {
        b.remove(tgt);
        b.place(src, p, mover_color);
    } else {
        b.move_piece(tgt, src);
    }

    if (capture_sq != square::none) {
        b.place(capture_sq, captured_piece, captured_color);
    }

    b.set_hash(saved_hash);
    return legal;
}

auto enumerate_candidates(board& b, piece p, int tgt_sq, u8 promo,
                          int disambig_file, int disambig_rank)
    -> candidate_list
{
    auto const white    = b.white_to_move();
    auto const my_color = white ? color::white : color::black;

    auto matches_disambig = [&](int src_sq) -> bool {
        if (disambig_file >= 0 && coord::file(src_sq) != disambig_file) { return false; }
        if (disambig_rank >= 0 && coord::rank(src_sq) != disambig_rank) { return false; }
        return true;
    };

    auto is_our_piece = [&](int src_sq) -> bool {
        return coord::valid(src_sq)
            && b.piece_at(src_sq) == p
            && b.color_at(src_sq) == my_color
            && matches_disambig(src_sq);
    };

    candidate_list candidates;

    auto push_cand = [&](int src_sq, u8 cap, u8 dbl, u8 enp) {
        candidates.push_back(move{
            .source_square = static_cast<u8>(src_sq),
            .target_square = static_cast<u8>(tgt_sq),
            .promotion     = promo,
            .capture       = cap,
            .double_pawn   = dbl,
            .enpassant     = enp,
            .castling      = 0,
        });
    };

    if (p == piece::pawn) {
        bool const is_promo_rank = white ? (coord::rank(tgt_sq) == 7) : (coord::rank(tgt_sq) == 0);
        if (is_promo_rank != (promo != 0)) {
            return candidates;
        }

        int const push1 = white ? -coord::no  : coord::no;
        int const push2 = white ? -coord::nn  : coord::nn;
        int const cap_l = white ? -coord::nw : -coord::sw;
        int const cap_r = white ? -coord::ne : -coord::se;

        if (b.piece_at(tgt_sq) == piece::none) {
            int const src1 = tgt_sq + push1;
            if (is_our_piece(src1)) {
                push_cand(src1, 0, 0, 0);
            }
            int const src2 = tgt_sq + push2;
            int const mid  = tgt_sq + push1;
            bool const on_start = white ? (src2 >= square::a2 && src2 <= square::h2)
                                        : (src2 >= square::a7 && src2 <= square::h7);
            if (on_start && coord::valid(src2) && coord::valid(mid)
                    && b.piece_at(mid) == piece::none && is_our_piece(src2)) {
                push_cand(src2, 0, 1, 0);
            }
        }

        auto const ep     = static_cast<int>(b.enpassant());
        bool const has_ep = static_cast<square>(ep) != square::none;
        for (int o : {cap_l, cap_r}) {
            int const src = tgt_sq + o;
            if (!coord::valid(src)) { continue; }
            if (!is_our_piece(src)) { continue; }
            bool const normal_cap = b.piece_at(tgt_sq) != piece::none && b.color_at(tgt_sq) != my_color;
            bool const ep_cap     = has_ep && ep == tgt_sq;
            if (!normal_cap && !ep_cap) { continue; }
            push_cand(src, 1, 0, ep_cap ? 1u : 0u);
        }

    } else {
        if (b.color_at(tgt_sq) == my_color) {
            return candidates;
        }
        if (b.piece_at(tgt_sq) == piece::king) {
            return candidates;
        }
        u8 const cap = b.piece_at(tgt_sq) != piece::none ? 1u : 0u;

        auto try_src = [&](int src_sq) {
            if (is_our_piece(src_sq)) { push_cand(src_sq, cap, 0, 0); }
        };

        auto slide = [&](auto const& offsets) {
            for (auto o : offsets) {
                for (int j = tgt_sq + o; coord::valid(j); j += o) {
                    if (b.piece_at(j) != piece::none) { try_src(j); break; }
                }
            }
        };

        auto jump = [&](auto const& offsets) {
            for (auto o : offsets) { try_src(tgt_sq + o); }
        };

        switch (p) {
            case piece::knight: jump(board::knight_offsets);  break;
            case piece::bishop: slide(board::bishop_offsets); break;
            case piece::rook:   slide(board::rook_offsets);   break;
            case piece::queen:  slide(board::queen_offsets);  break;
            case piece::king:   jump(board::king_offsets);    break;
            default: break;
        }
    }

    return candidates;
}

auto parse_castle_move(board const& b, bool kingside) -> tl::expected<move, error> {
    auto const white = b.white_to_move();
    auto const my_color = white ? color::white : color::black;
    auto const enemy = white ? side_to_move::black : side_to_move::white;

    auto const king_src = white ? square::e1 : square::e8;
    auto const king_tgt = kingside
        ? (white ? square::g1 : square::g8)
        : (white ? square::c1 : square::c8);
    auto const rook_sq = kingside
        ? (white ? square::h1 : square::h8)
        : (white ? square::a1 : square::a8);
    auto const required_right = kingside
        ? (white ? castling_rights::wk : castling_rights::bk)
        : (white ? castling_rights::wq : castling_rights::bq);

    if (b.piece_at(king_src) != piece::king || b.color_at(king_src) != my_color) {
        return tl::unexpected{error::no_matching_move};
    }
    if (b.piece_at(rook_sq) != piece::rook || b.color_at(rook_sq) != my_color) {
        return tl::unexpected{error::no_matching_move};
    }
    if (me::enum_integer(b.castling() & required_right) == 0) {
        return tl::unexpected{error::no_matching_move};
    }
    if (b.is_king_in_check()) {
        return tl::unexpected{error::no_matching_move};
    }

    auto const path_clear = kingside
        ? (white ? std::array{square::f1, square::g1}
                 : std::array{square::f8, square::g8})
        : (white ? std::array{square::d1, square::c1}
                 : std::array{square::d8, square::c8});

    for (auto sq : path_clear) {
        if (b.piece_at(sq) != piece::none || b.is_attacked(sq, enemy)) {
            return tl::unexpected{error::no_matching_move};
        }
    }

    if (!kingside) {
        auto const rook_path_sq = white ? square::b1 : square::b8;
        if (b.piece_at(rook_path_sq) != piece::none) {
            return tl::unexpected{error::no_matching_move};
        }
    }

    return move{
        .source_square = static_cast<u8>(king_src),
        .target_square = static_cast<u8>(king_tgt),
        .promotion = 0,
        .capture = 0,
        .double_pawn = 0,
        .enpassant = 0,
        .castling = 1,
    };
}

} // namespace detail

// ---- public API ----

auto to_string(board& b, move m) -> std::string {
    std::string result;

    if (m.castling) {
        bool const kingside = (m.target_square == square::g1 || m.target_square == square::g8);
        result = kingside ? "O-O" : "O-O-O";
    } else {
        auto const src = static_cast<int>(m.source_square);
        auto const tgt = static_cast<int>(m.target_square);
        auto const p   = b.piece_at(src);

        if (p != piece::pawn) {
            result += piece_letter(p);
        }

        if (p != piece::pawn) {
            auto const info = disambiguation_flags(b, m, p);

            if (info.any_ambig) {
                if (!info.same_file) {
                    result += static_cast<char>('a' + coord::file(src));
                } else if (!info.same_rank) {
                    result += static_cast<char>('1' + coord::rank(src));
                } else {
                    result += static_cast<char>('a' + coord::file(src));
                    result += static_cast<char>('1' + coord::rank(src));
                }
            }
        }

        if (p == piece::pawn && m.capture) {
            result += static_cast<char>('a' + coord::file(src));
        }

        if (m.capture) { result += 'x'; }

        result += static_cast<char>('a' + coord::file(tgt));
        result += static_cast<char>('1' + coord::rank(tgt));

        if (m.promotion) {
            result += '=';
            result += piece_letter(static_cast<piece>(m.promotion));
        }
    }

    move_maker mm{b, m};
    mm.make();
    if (b.is_king_in_check()) {
        result += has_any_legal_move(b) ? '+' : '#';
    }
    mm.undo();

    return result;
}

auto from_string(board& b, std::string_view s) -> tl::expected<move, error> {
    if (s.empty()) {
        return tl::unexpected{error::invalid_syntax};
    }

    if (s.back() == '+' || s.back() == '#') {
        s.remove_suffix(1);
    }

    if (s.empty()) {
        return tl::unexpected{error::invalid_syntax};
    }

    if (s == "O-O-O") {
        return detail::parse_castle_move(b, false);
    }
    if (s == "O-O") {
        return detail::parse_castle_move(b, true);
    }

    u8 promo{0};
    if (s.size() >= 2 && is_promo_letter(s.back())) {
        char const penult = s[s.size() - 2];
        if (penult == '=') {
            promo = static_cast<u8>(char_to_piece(s.back()));
            s.remove_suffix(2);
        } else if (is_rank_digit(penult)) {
            promo = static_cast<u8>(char_to_piece(s.back()));
            s.remove_suffix(1);
        }
    }

    if (s.size() < 2) {
        return tl::unexpected{error::invalid_syntax};
    }
    char const tgt_file = s[s.size() - 2];
    char const tgt_rank = s[s.size() - 1];
    if (!is_file_letter(tgt_file) || !is_rank_digit(tgt_rank)) {
        return tl::unexpected{error::invalid_syntax};
    }
    auto const tgt_sq = static_cast<u8>(coord::square_index(tgt_rank - '1', tgt_file - 'a'));
    s.remove_suffix(2);

    if (!s.empty() && s.back() == 'x') { s.remove_suffix(1); }

    piece p = piece::pawn;
    if (!s.empty() && std::isupper(static_cast<unsigned char>(s.front()))) {
        p = char_to_piece(s.front());
        if (p == piece::none) {
            return tl::unexpected{error::invalid_syntax};
        }
        s.remove_prefix(1);
    }

    if (s.size() > 2) {
        return tl::unexpected{error::invalid_syntax};
    }

    int disambig_file = -1;
    int disambig_rank = -1;

    if (s.size() == 2) {
        if (!is_file_letter(s[0]) || !is_rank_digit(s[1])) {
            return tl::unexpected{error::invalid_syntax};
        }
        disambig_file = s[0] - 'a';
        disambig_rank = s[1] - '1';
    } else if (s.size() == 1) {
        if      (is_file_letter(s[0])) { disambig_file = s[0] - 'a'; }
        else if (is_rank_digit(s[0]))  { disambig_rank = s[0] - '1'; }
        else { return tl::unexpected{error::invalid_syntax}; }
    }

    return find_san_move_targeted(b, p, tgt_sq, promo, disambig_file, disambig_rank);
}

auto to_string(board const& b, move m) -> std::string {
    board tmp = b;
    return to_string(tmp, m);
}

auto from_string(board const& b, std::string_view s) -> tl::expected<move, error> {
    board tmp = b;
    return from_string(tmp, s);
}

} // namespace chesslib::san
