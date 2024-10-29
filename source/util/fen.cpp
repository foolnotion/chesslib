#include <string>
#include <ranges>

#include "chesslib/util/fen.hpp"
#include "chesslib/board/encoding.hpp"

namespace chesslib {
    auto fen_parser::parse(std::string_view fen) -> board {
        board b;
        u8 rank = 0;
        u8 sq   = 0;

        using namespace me::bitwise_operators;

        auto n = 0;
        for (auto field : std::ranges::views::split(fen, ' ')) {
            switch(n++) {
                case fen_record::piece_placement: {
                    // parse position
                    auto row = static_cast<u32>(square::a8);
                    for (auto line : std::ranges::views::split(field, '/')) {
                        auto sq = row;
                        for (auto c : line) {
                            if (std::isdigit(c) != 0) {
                                ASSERT(c-'0' <= 8, "fen digit cannot exceed 8");
                                auto s = sq;
                                for (; s < sq + c - '0'; ++s) {
                                    b.pieces[s] = piece::none;
                                    b.colors[s] = color::none;
                                }
                                sq = s;
                                continue;
                            }
                            if (std::isalpha(c) != 0) {
                                auto col   = std::isupper(c) != 0 ? color::white : color::black;
                                b.pieces[sq] = char2piece(static_cast<char>(std::tolower(c)));
                                b.colors[sq] = col;
                                ++sq;
                            }
                        }
                        row -= coord::ncol;
                    }
                    break;
                }
                case fen_record::active_color: {
                    // parsing side to move
                    b.side_ = field.front() == 'w' ? side::white : side::black;
                    break;
                }
                case fen_record::castling_availability: {
                    // parsing castling rights
                    u8 castling_rights = 0;
                    for (auto c : field) {
                        switch(c) {
                            case 'K':
                                castling_rights |= me::enum_integer(castle::wk);
                                break;
                            case 'Q':
                                castling_rights |= me::enum_integer(castle::wq);
                                break;
                            case 'k':
                                castling_rights |= me::enum_integer(castle::bk);
                                break;
                            case 'q':
                                castling_rights |= me::enum_integer(castle::bq);
                                break;
                            default:
                                break;
                        }
                    }
                    b.castle_ = static_cast<castle>(castling_rights);
                    break;
                }
                case fen_record::en_passant_target: {
                    // parsing en-passant
                    auto res = me::enum_cast<square>(std::string_view{field.begin(), field.end()});
                    if (res) {
                        b.enpassant_ = res.value();
                    }
                    break;
                }
                case fen_record::halfmove_clock: {
                    // parsing half-move clock: the number of halfmoves since the
                    // last capture or pawn advance (used for the 50-move rule)
                    b.ply_ = std::stoi(std::string{field.begin(), field.end()});
                    break;

                }
                case fen_record::fullmove_number: {
                    // parsing full-move numbe: the number of full moves
                    b.movecount_ = std::stoi(std::string{field.begin(), field.end()});
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