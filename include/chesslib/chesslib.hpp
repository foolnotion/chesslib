#ifndef CHESSLIB_CHESSLIB_HPP
#define CHESSLIB_CHESSLIB_HPP

#include <string>

#include <fmt/core.h>

#include "board/encoding.hpp"

namespace chesslib {

/**
 * @brief Return the name of this header-only library
 */
inline auto name() -> std::string
{
    return fmt::format("{}", "chesslib");
}

}  // namespace chesslib

#endif
