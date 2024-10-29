# Building with CMake

## Build

This project doesn't require any special command-line flags to build to keep
things simple.

Here are the steps for building in release mode with a single-configuration
generator, like the Unix Makefiles one:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -Dchesslib_DEVELOPER_MODE=1
cmake --build build
```

Here are the steps for building in release mode with a multi-configuration
generator, like the Visual Studio ones:

```sh
cmake -S . -B build
cmake --build build --config Release
```

### Building on Windows with VCPKG

This library compiles with `clang-cl`, so make sure to install it:\
https://learn.microsoft.com/en-us/cpp/build/clang-support-msbuild?view=msvc-170

Package management on Windows is done with `vcpkg`, so it needs to be installed as well.\
Assuming `vcpkg` is installed somewhere on your system at `$VCPKG_ROOT`:

```sh
cd chesslib
git submodule update --init --recursive
cmake -S . -B build -TClangCL -Dchesslib_DEVELOPER_MODE=1 -DCMAKE_TOOLCHAIN_FILE=C:\Dev\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build build -j -t chesslib_test
```

### Building on WSL/Ubuntu

Install dependencies:

```sh
sudo apt update
sudo apt install curl zip unzip tar
sudo apt install build-essential
sudo apt install cmake
sudo apt install pkg-config
sudo apt install clang
```

Install `vcpkg`:

```sh
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
```

Build `chesslib`:

```sh
cd chesslib
git submodule update --init --recursive
CXX=clang++ cmake -S . -B build -Dchesslib_DEVELOPER_MODE=1 -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
cmake --build build -j -t chesslib_test
```

## Install

This project doesn't require any special command-line flags to install to keep
things simple. As a prerequisite, the project has to be built with the above
commands already.

The below commands require at least CMake 3.15 to run, because that is the
version in which [Install a Project][2] was added.

Here is the command for installing the release mode artifacts with a
single-configuration generator, like the Unix Makefiles one:

```sh
cmake --install build
```

Here is the command for installing the release mode artifacts with a
multi-configuration generator, like the Visual Studio ones:

```sh
cmake --install build --config Release
```

### CMake package

This project exports a CMake package to be used with the [`find_package`][3]
command of CMake:

* Package name: `chesslib`
* Target name: `chesslib::chesslib`

Example usage:

```cmake
find_package(chesslib REQUIRED)
# Declare the imported target as a build requirement using PRIVATE, where
# project_target is a target created in the consuming project
target_link_libraries(
    project_target PRIVATE
    chesslib::chesslib
)
```

### Note to packagers

The `CMAKE_INSTALL_INCLUDEDIR` is set to a path other than just `include` if
the project is configured as a top level project to avoid indirectly including
other libraries when installed to a common prefix. Please review the
[install-rules.cmake](cmake/install-rules.cmake) file for the full set of
install rules.

[1]: https://cmake.org/download/
[2]: https://cmake.org/cmake/help/latest/manual/cmake.1.html#install-a-project
[3]: https://cmake.org/cmake/help/latest/command/find_package.html
