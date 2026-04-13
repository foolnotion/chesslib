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
            bool ambig_same_file = false;
            bool ambig_same_rank = false;
            bool any_ambig       = false;

            for (auto const& c : legal_moves(b)) {
                if (c.source_square == m.source_square) { continue; }
                if (c.target_square != m.target_square)  { continue; }
                if (b.piece_at(c.source_square) != p)    { continue; }
                any_ambig = true;
                if (coord::file(c.source_square) == coord::file(src)) { ambig_same_file = true; }
                if (coord::rank(c.source_square) == coord::rank(src)) { ambig_same_rank = true; }
            }

            if (any_ambig) {
                if (!ambig_same_file) {
                    result += static_cast<char>('a' + coord::file(src));
                } else if (!ambig_same_rank) {
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
        result += legal_moves(b).empty() ? '#' : '+';
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
        for (auto const& m : legal_moves(b)) {
            if (m.castling && (m.target_square == square::c1 || m.target_square == square::c8)) {
                return m;
            }
        }
        return tl::unexpected{error::no_matching_move};
    }
    if (s == "O-O") {
        for (auto const& m : legal_moves(b)) {
            if (m.castling && (m.target_square == square::g1 || m.target_square == square::g8)) {
                return m;
            }
        }
        return tl::unexpected{error::no_matching_move};
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

    // Match against legal moves
    auto const moves = legal_moves(b);
    move const* found = nullptr;

    for (auto const& m : moves) {
        if (b.piece_at(m.source_square) != p)          { continue; }
        if (m.target_square != tgt_sq)                  { continue; }
        if (m.promotion != promo)                       { continue; }
        if (disambig_file >= 0 && coord::file(m.source_square) != disambig_file) { continue; }
        if (disambig_rank >= 0 && coord::rank(m.source_square) != disambig_rank) { continue; }

        if (found) { return tl::unexpected{error::ambiguous}; }
        found = &m;
    }

    if (!found) {
        return tl::unexpected{error::no_matching_move};
    }
    return *found;
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
