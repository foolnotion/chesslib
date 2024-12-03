#include <string>
#include <ranges>

#include "chesslib/util/fen.hpp"
#include "chesslib/board/board.hpp"

using namespace chesslib::encoding;

namespace chesslib::fen {

    auto read(std::string_view fen) -> board {
        board b;
        std::ranges::fill(b.pieces, piece::none);
        std::ranges::fill(b.colors, color::none);
        auto& state = b.state();

        auto n = 0;
        for (auto field : std::ranges::views::split(fen, ' ')) {
            switch(n++) {
                case fen_record::piece_placement: {
                    // parse position
                    auto row = static_cast<i32>(square::a8);
                    for (auto line : std::ranges::views::split(field, '/')) {
                        auto sq = row-1;
                        for (auto c : line) {
                            if (std::isdigit(c) != 0) {
                                sq += c-'0';
                            }
                            else if (std::isalpha(c) != 0) {
                                sq += 1;

                                auto col     = std::isupper(c) != 0 ? color::white : color::black;
                                auto pic     = char2piece(static_cast<char>(std::tolower(c)));
                                b.pieces[sq] = pic;
                                b.colors[sq] = col;

                                // keep track of the king position
                                if (pic == piece::king) {
                                    auto& k = col == color::white ? state.white_king : state.black_king;
                                    k = static_cast<square>(sq);
                                }
                            }
                        }
                        row -= coord::ncol;
                    }
                    break;
                }
                case fen_record::active_color: {
                    // parsing side to move
                    b.side() = field.front() == 'w' ? side_to_move::white : side_to_move::black;
                    break;
                }
                case fen_record::castling_availability: {
                    // parsing castling rights
                    u8 castling = 0;
                    for (auto c : field) {
                        switch(c) {
                            case 'K':
                                castling |= me::enum_integer(castling_rights::wk);
                                break;
                            case 'Q':
                                castling |= me::enum_integer(castling_rights::wq);
                                break;
                            case 'k':
                                castling |= me::enum_integer(castling_rights::bk);
                                break;
                            case 'q':
                                castling |= me::enum_integer(castling_rights::bq);
                                break;
                            default:
                                break;
                        }
                    }
                    state.castling = static_cast<castling_rights>(castling);
                    break;
                }
                case fen_record::en_passant_target: {
                    // parsing en-passant
                    auto res = me::enum_cast<square>(std::string_view{field.begin(), field.end()});
                    if (res) {
                        state.enpassant = res.value();
                    }
                    break;
                }
                case fen_record::halfmove_clock: {
                    // parsing half-move clock: the number of half-moves since the
                    // last capture or pawn advance (used for the 50-move rule)
                    state.ply = std::stoi(std::string{field.begin(), field.end()});
                    break;

                }
                case fen_record::fullmove_number: {
                    // parsing full-move number: the number of full moves
                    state.count = std::stoi(std::string{field.begin(), field.end()});
                    break;
                }
                default: {
                    throw std::runtime_error("invalid fen string");
                    break;
                }
            }
        }
        return b;
    }

    auto write(board const& b) -> std::string {
        auto const& pieces = b.pieces;
        auto const& state  = b.state();

        std::string fen;
        // fill in the piece positions field
        for (i32 i = me::enum_integer(square::a8); i >= 0; i -= coord::ncol) {
            for (auto sq = i; coord::valid(sq); ) {
                // deal with empty squares
                auto k = sq;
                while(coord::valid(sq) && pieces[sq] == piece::none) { ++sq; }
                if (sq > k) {
                    ASSERT(sq-k <= 8);
                    fen.push_back(sq-k+'0');
                }

                while(coord::valid(sq) && pieces[sq] != piece::none) {
                    auto [p, c] = b[sq++];
                    fen.push_back(piece_letters[c == color::white ? 0 : 1][me::enum_integer(p)]);
                }
            }
            if (i >= coord::ncol) {
                fen.push_back('/');
            }
        }

        // fill in the side to move
        fen.push_back(' ');
        fen.push_back(b.white_to_move() ? 'w' : 'b');

        // fill in castling availability
        fen.push_back(' ');
        auto castling = state.castling;
        if ((castling & castling_rights::wk) == castling_rights::wk) { fen.push_back('K'); }
        if ((castling & castling_rights::wq) == castling_rights::wq) { fen.push_back('Q'); }
        if ((castling & castling_rights::bk) == castling_rights::bk) { fen.push_back('k'); }
        if ((castling & castling_rights::bq) == castling_rights::bq) { fen.push_back('q'); }
        if (me::enum_integer(castling) == 0) { fen.push_back('-'); }

        // fill in enpassant square
        fen.push_back(' ');
        fen += state.enpassant == square::none ? "-" : me::enum_name(state.enpassant);

        // fill in the ply and move count
        fen.push_back(' ');
        fen += fmt::format("{} {}", state.ply, state.count);
        return fen;
    }
}  // namespace chesslib::fen