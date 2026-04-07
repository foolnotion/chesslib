# Building

## Prerequisites

All dependencies are managed via the [Nix](https://nixos.org/) flake. Enter the
development shell with:

```sh
nix develop
```

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Build and test

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -Dchesslib_DEVELOPER_MODE=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Install

```sh
cmake --install build
```

## CMake package

This project exports a CMake package for use with `find_package`:

- Package name: `chesslib`
- Target name: `chesslib::chesslib`

```cmake
find_package(chesslib REQUIRED)
target_link_libraries(my_target PRIVATE chesslib::chesslib)
```

See [`cmake/install-rules.cmake`](cmake/install-rules.cmake) for the full
install layout. Note that `CMAKE_INSTALL_INCLUDEDIR` is set to a versioned path
(`include/chesslib-<version>`) when installed as a top-level project.
