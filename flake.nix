{
  description = "Chesslib Dev";

  inputs = {
    flake-parts.url = "github:hercules-ci/flake-parts";
    nixpkgs.url = "github:nixos/nixpkgs/master";
    foolnotion.url = "github:foolnotion/nur-pkg/master";

    foolnotion.inputs.nixpkgs.follows = "nixpkgs";
  };

  outputs =
    inputs@{
      self,
      flake-parts,
      nixpkgs,
      foolnotion,
    }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = [
        "x86_64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];

      perSystem =
        { pkgs, system, ... }:
        let
          pkgs = import self.inputs.nixpkgs {
            inherit system;
            overlays = [ foolnotion.overlay ];
          };
          stdenv = pkgs.llvmPackages_20.stdenv;
        in
        rec {
          devShells.default = stdenv.mkDerivation {
            name = "dev";
            src = ./.;

            nativeBuildInputs = with pkgs; [
              cmake
              clang-tools
              cppcheck
              include-what-you-use
              cmake-language-server
            ];

            buildInputs = with pkgs; [
              # dev
              cxxopts
              gdb
              gcc14
              perf
              valgrind
              hotspot
              hyperfine

              # deps
              catch2_3
              cpptrace
              fmt
              gch-small-vector
              mdspan
              nanobench
              libassert
              libdwarf
              magic-enum
              nlohmann_json
              unordered_dense
              zstd
              gtl
              tl-expected
            ];
          };

          packages.default = stdenv.mkDerivation rec {
            name = "chesslib";
            src = ./.;

            nativeBuildInputs = with pkgs; [ cmake ];

            cmakeBuildType = "Release";

            buildInputs = with pkgs; [
              cpptrace
              fmt
              gch-small-vector
              libassert
              libdwarf
              magic-enum
              mdspan
              unordered_dense
              zstd
            ];
          };
        };
    };
}
