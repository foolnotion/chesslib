#ifndef CHESSLIB_CORE_SCOPE_EXIT_HPP
#define CHESSLIB_CORE_SCOPE_EXIT_HPP

#include <utility>

namespace chesslib {

template<typename F>
struct scope_exit {
    explicit scope_exit(F f) : f_{std::move(f)} {}
    ~scope_exit() noexcept { f_(); }
    scope_exit(scope_exit&&) = delete;
    scope_exit(scope_exit const&) = delete;
    auto operator=(scope_exit&&) -> scope_exit& = delete;
    auto operator=(scope_exit const&) -> scope_exit& = delete;

    private:
    F f_;
};

template<typename F>
scope_exit(F) -> scope_exit<F>;

}  // namespace chesslib

#endif
