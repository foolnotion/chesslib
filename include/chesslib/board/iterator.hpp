#ifndef CHESSLIB_ITER_HPP
#define CHESSLIB_ITER_HPP

#include <chesslib/core/types.hpp>

#include <array>
#include <iterator>

namespace chesslib {

template<piece P = piece::none>
struct piece_offsets {};

template<>
struct piece_offsets<piece::knight> {
    static constexpr std::array values{33, 31, 18, 14, -33, -31, -18, -14};
    static constexpr bool sliding{false};
};

template<>
struct piece_offsets<piece::bishop> {
    static constexpr std::array values{33, 31, 18, 14, -33, -31, -18, -14};
    static constexpr bool sliding{true};
};

template<typename PO /* piece_offsets */>
class move_iterator {
public:
    using value_type = u8;

    struct sentinel{};

    static constexpr auto values{PO::values};
    explicit move_iterator(u8 square) : square_{square} {}

    auto begin() const { return *this; }
    auto end() const { return sentinel{}; }

    auto operator==(sentinel /*unused*/) const -> bool { return iter_ == std::end(values); }

    auto operator==(move_iterator const& other) {
        return square_ == other.square_ && iter_ == other.iter_;
    }

    auto operator!=(move_iterator const& other) {
        return !(*this == other);
    }

    auto operator<(move_iterator const& other) {
        return std::tie(square_, iter_) < std::tie(other.square_, other.iter_);
    }

    auto operator*() const -> u8 { return square_ + *iter_; }
    auto operator*() -> u8 { return square_ + *iter_; }

    auto operator++() -> move_iterator& {
        ++iter_;
    }

    auto operator++(int) -> move_iterator {
        auto tmp{*this};
        ++(*this);
        return tmp;
    }

private:
    decltype(values)::iterator iter_;
    u8 square_;
};
}  // namespace chesslib

#endif