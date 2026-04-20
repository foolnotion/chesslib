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
        move_maker mm{mutable_board, candidate};
        if (!mm.check()) {
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

        move_maker mm{mutable_board, candidate};
        if (mm.check()) { continue; }

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
        move_maker mm{mutable_board, candidate};
        if (mm.check()) { continue; }
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

// Target-driven SAN resolution: derives candidate source squares from the
// target square and piece type rather than generating all pseudo-legal moves.
// For PGN workloads this is significantly faster because only O(1..8) candidate
// source squares are tested per call instead of the full move list.
auto find_san_move_targeted(board& b, piece p, int tgt_sq, u8 promo,
                            int disambig_file, int disambig_rank) -> tl::expected<move, error>
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

    // Small fixed-size buffer; a queen has at most 8 rays so 16 is ample.
    move candidates[16];
    int  n_cand = 0;

    auto push_cand = [&](int src_sq, u8 cap, u8 dbl, u8 enp) {
        candidates[n_cand++] = move{
            .source_square = static_cast<u8>(src_sq),
            .target_square = static_cast<u8>(tgt_sq),
            .promotion     = promo,
            .capture       = cap,
            .double_pawn   = dbl,
            .enpassant     = enp,
            .castling      = 0,
        };
    };

    if (p == piece::pawn) {
        // Pawn move to the back rank must always promote (and vice versa).
        bool const is_promo_rank = white ? (coord::rank(tgt_sq) == 7) : (coord::rank(tgt_sq) == 0);
        if (is_promo_rank != (promo != 0)) {
            return tl::unexpected{error::no_matching_move};
        }

        // Backward offsets from target to source for each pawn move type.
        // White pawns move north (+16); black pawns move south (-16).
        int const push1 = white ? -coord::no  : coord::no;   // -16 or +16
        int const push2 = white ? -coord::nn  : coord::nn;   // -32 or +32
        int const cap_l = white ? -coord::nw : -coord::sw;   // white: -15; black: +17
        int const cap_r = white ? -coord::ne : -coord::se;   // white: -17; black: +15

        // Quiet pushes (target must be empty)
        if (b.piece_at(tgt_sq) == piece::none) {
            int const src1 = tgt_sq + push1;
            if (is_our_piece(src1)) {
                push_cand(src1, 0, 0, 0);
            }
            // Double push: pawn must be on its starting rank; intermediate must be clear.
            int const src2 = tgt_sq + push2;
            int const mid  = tgt_sq + push1;
            bool const on_start = white ? (src2 >= square::a2 && src2 <= square::h2)
                                        : (src2 >= square::a7 && src2 <= square::h7);
            if (on_start && coord::valid(src2) && coord::valid(mid)
                    && b.piece_at(mid) == piece::none && is_our_piece(src2)) {
                push_cand(src2, 0, 1, 0);
            }
        }

        // Diagonal captures (normal or en passant)
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
        // Non-pawn: target must be empty or hold a capturable enemy piece.
        // Capturing the king is never a legal SAN move and the move generator
        // never generates it; reject early to stay consistent.
        if (b.color_at(tgt_sq) == my_color) {
            return tl::unexpected{error::no_matching_move};
        }
        if (b.piece_at(tgt_sq) == piece::king) {
            return tl::unexpected{error::no_matching_move};
        }
        u8 const cap = b.piece_at(tgt_sq) != piece::none ? 1u : 0u;

        auto try_src = [&](int src_sq) {
            if (is_our_piece(src_sq)) { push_cand(src_sq, cap, 0, 0); }
        };

        // Sliding: reverse-ray from target; stop at first piece on each ray.
        // That piece is the only candidate along that ray (pieces behind it are blocked).
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

    // Run legality checks only on the small set of candidates.
    move const* found = nullptr;
    for (int i = 0; i < n_cand; ++i) {
        move_maker mm{b, candidates[i]};
        if (mm.check()) { continue; }
        if (found != nullptr) { return tl::unexpected{error::ambiguous}; }
        found = &candidates[i];
    }

    return found ? tl::expected<move, error>{*found} : tl::unexpected{error::no_matching_move};
}

} // namespace

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

        // Disambiguation: scan for other legal moves of the same piece type to the same target
        if (p != piece::pawn) {
            auto const info = disambiguation_flags(b, m, p);

            if (info.any_ambig) {
                if (!info.same_file) {
                    result += static_cast<char>('a' + coord::file(src));
                } else if (!info.same_rank) {
                    result += static_cast<char>('1' + coord::rank(src));
                } else {
                    // Both file and rank needed (3+ pieces)
                    result += static_cast<char>('a' + coord::file(src));
                    result += static_cast<char>('1' + coord::rank(src));
                }
            }
        }

        // Pawn capture: always show source file
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

    // Check / checkmate suffix
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

    // Strip check/mate suffix (these are derived, not load-bearing for matching)
    if (s.back() == '+' || s.back() == '#') {
        s.remove_suffix(1);
    }

    if (s.empty()) {
        return tl::unexpected{error::invalid_syntax};
    }

    // Castling — test O-O-O before O-O (prefix match order)
    if (s == "O-O-O") {
        return find_san_move(b, [](move const& m) {
            return m.castling && (m.target_square == square::c1 || m.target_square == square::c8);
        });
    }
    if (s == "O-O") {
        return find_san_move(b, [](move const& m) {
            return m.castling && (m.target_square == square::g1 || m.target_square == square::g8);
        });
    }

    // Promotion suffix: strip "=Q" (standard) or bare "Q" when preceded by a rank digit
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

    // Target square: last 2 chars
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

    // Capture marker (informational only — we match on the legal move itself)
    if (!s.empty() && s.back() == 'x') { s.remove_suffix(1); }

    // Piece type: uppercase first char = piece; absent or lowercase = pawn
    piece p = piece::pawn;
    if (!s.empty() && std::isupper(static_cast<unsigned char>(s.front()))) {
        p = char_to_piece(s.front());
        if (p == piece::none) {
            return tl::unexpected{error::invalid_syntax};
        }
        s.remove_prefix(1);
    }

    // Disambiguation: 0-2 chars remain
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
