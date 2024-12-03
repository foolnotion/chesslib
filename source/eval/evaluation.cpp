#include <chesslib/eval/piece_square_tables.hpp>
#include <chesslib/board/board.hpp>
#include <fmt/ranges.h>

namespace chesslib {
    // Port of Pesto's evaluation function
    // https://www.chessprogramming.org/PeSTO%27s_Evaluation_Function
    auto evaluate(board const& b) -> i64 {
        static std::array game_phase_inc{0,1,1,2,4,0};

        std::array mid{0, 0};
        std::array end{0, 0};
        [[maybe_unused]] auto game_phase{0};

        auto const& pieces = b.pieces;
        auto const& colors = b.colors;
        auto const s = static_cast<u8>(b.side());

        // evaluate each piece
        for (auto const sq : valid_squares) {
            if (pieces[sq] == piece::none) {
                continue;
            }
            auto const p = static_cast<u8>(pieces[sq]);
            auto const c = static_cast<u8>(colors[sq]);
            auto const k = coord::valid_index(sq);
            mid[c] += evaluation::tables::midgame_table[c][p][k];
            end[c] += evaluation::tables::endgame_table[c][p][k];
            game_phase += game_phase_inc[p];
        }

        // threshold for transition from midgame to endgame
        constexpr auto mid_threshold = 24;

        // tapered eval
        auto mid_score = mid[s] - mid[s ^ 1U];
        auto end_score = end[s] - end[s ^ 1U];
        auto mid_phase = std::min(game_phase, mid_threshold);
        auto end_phase = mid_threshold - mid_phase;
        fmt::print("mid: {}\n", mid);
        fmt::print("end: {}\n", end);
        fmt::print("mid_score: {}\nmid_phase: {}\nend_score: {}\nend_phase: {}\n", mid_score, mid_phase, end_score, end_phase);
        return (mid_score * mid_phase + end_score * end_phase) / 24;
    }
    } // namespace chesslib