#include <string>
#include <ranges>

#include "chesslib/util/fen.hpp"
#include "chesslib/board/encoding.hpp"

namespace chesslib {
    auto fen_parser::parse(std::string_view fen) -> board {
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
}  // namespace chesslib