{
  description = "Chesslib Dev";

  inputs = {
    flake-parts.url = "github:hercules-ci/flake-parts";
    nixpkgs.url = "github:nixos/nixpkgs/master";
    foolnotion.url = "github:foolnotion/nur-pkg/master";

    foolnotion.inputs.nixpkgs.follows = "nixpkgs";
  };

  outputs = inputs@{ self, flake-parts, nixpkgs, foolnotion}:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = [ "x86_64-linux" "x86_64-darwin" "aarch64-darwin" ];

      perSystem = { pkgs, system, ... }:
        let
          pkgs = import self.inputs.nixpkgs {
            inherit system;
            overlays = [ foolnotion.overlay ];
          };
          stdenv = pkgs.llvmPackages_18.stdenv;
        in
        rec
        {
          devShells.default = stdenv.mkDerivation {
            name = "dev";
            src = ./.;

            nativeBuildInputs = with pkgs; [
              cmake
              clang-tools_18
              cppcheck
              include-what-you-use
              cmake-language-server
            ];

            buildInputs = with pkgs; [
              # dev
              gdb
              linuxPackages_latest.perf

              # deps
              catch2_3
              cpptrace
              fmt
              gch-small-vector
              libassert
              libdwarf
              magic-enum
              unordered_dense
              zstd
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
              unordered-dense
              zstd
            ];
          };
        };
    };
}
