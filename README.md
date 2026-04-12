# chesslib

A C++20 chess rules and move-generation library. Originally developed as a
live-coded teaching example for a chess programming university lecture.

## Features

- 0x88 board encoding with bit-packed move representation
- Pseudo-legal move generation with legality filtering
- FEN parsing and serialisation (`tl::expected`-based error handling)
- UCI move parsing
- Incremental Zobrist hashing
- Perft testing utilities

## Dependencies

All dependencies are managed via the [Nix](https://nixos.org/) flake:

| Library | Role | Visibility |
|---|---|---|
| [fmt](https://github.com/fmtlib/fmt) | Formatting | Private |
| [libassert](https://github.com/jeremy-rifkin/libassert) | Assertions | Private |
| [magic-enum](https://github.com/Neargye/magic_enum) | Enum reflection | Public |
| [tl-expected](https://github.com/TartanLlama/expected) | Error handling | Public |
| [gch-small-vector](https://github.com/gharveymn/small_vector) | Move list (SBO) | Public |
| [mdspan](https://github.com/kokkos/mdspan) | Zobrist table layout | Private |
| [unordered-dense](https://github.com/martinus/unordered_dense) | Hash containers | Public |

## Building

```sh
nix develop --command cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
nix develop --command cmake --build build
```

To build and run tests:

```sh
nix develop --command cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -Dchesslib_DEVELOPER_MODE=ON
nix develop --command cmake --build build
nix develop --command ctest --test-dir build --output-on-failure
```

To verify the installed package can be consumed via `find_package`:

```sh
nix develop --command cmake --install build --prefix "$PWD/install"
nix develop --command cmake -S test/package-install -B build/package-install -DCMAKE_PREFIX_PATH="$PWD/install"
nix develop --command cmake --build build/package-install
./build/package-install/chesslib_package_smoke
```

## Consuming via CMake

After installing the library, use `find_package`:

```cmake
find_package(chesslib REQUIRED)
target_link_libraries(my_target PRIVATE chesslib::chesslib)
```

Or with CMake's `FetchContent` (requires Nix or manual dependency setup):

```cmake
include(FetchContent)
FetchContent_Declare(chesslib GIT_REPOSITORY https://github.com/foolnotion/chesslib.git)
FetchContent_MakeAvailable(chesslib)
target_link_libraries(my_target PRIVATE chesslib::chesslib)
```

## Quick example

```cpp
#include <chesslib/chesslib.hpp>

int main() {
    chesslib::board b{"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"};

    auto moves = chesslib::legal_moves(b);

    for (auto const& m : moves) {
        auto uci = chesslib::uci::to_string(m);
        // ...
    }
}
```

## License

MIT
