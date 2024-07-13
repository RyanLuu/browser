{
  description = "Browser flake";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = inputs@{ flake-parts, ... }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = [
        "x86_64-linux"
      ];
      perSystem = { config, self', inputs', pkgs, system, lib, ... }:
        let
          stdenv = pkgs.keepDebugInfo pkgs.clangStdenv;
        in
        rec {
          devShells.default = pkgs.mkShell.override { inherit stdenv; } {
            name = "clang";
            LD_LIBRARY_PATH = "${lib.makeLibraryPath packages.default.buildInputs}";
            packages = with pkgs;
              [
                clang-tools
                man-pages
                man-pages-posix
                libllvm
                valgrind
                gdb
              ] ++ packages.default.buildInputs; # get man pages for build inputs
          };
          packages.default = stdenv.mkDerivation rec {
            name = "browser";
            src = ./src;

            buildInputs = with pkgs; [
              glib
              harfbuzz
              freetype
              sokol
              xorg.libX11
              xorg.libXi
              xorg.libXcursor
              xorg.libSM
              xorg.libXext
              ffmpeg
              libglvnd
              libGL
            ];

            nativeBuildInputs = with pkgs; [
              pkg-config
            ];

            buildPhase =
              let
                sources = builtins.concatStringsSep " "
                  [ "main.c" ];
                libs = builtins.concatStringsSep " "
                  (builtins.concatMap (l: if l.meta ? pkgConfigModules then l.meta.pkgConfigModules else [ ]) buildInputs);
              in
              ''
                NIX_CFLAGS_COMPILE="-Wall -Werror -Wpedantic $(pkg-config --libs-only-other --cflags ${libs}) $NIX_CFLAGS_COMPILE"
                NIX_LDFLAGS="$(pkg-config --libs-only-L --libs-only-l ${libs}) $NIX_LDFLAGS"
                LD_LIBRARY_PATH="${lib.makeLibraryPath buildInputs}"
                cc -o ${name} ${sources}
              '';

            installPhase = ''
              mkdir -p $out/bin
              echo $NIX_CFLAGS_COMPILE | tr ' ' '\n' > $out/compile_flags.txt
              cp ${name} $out/bin/
            '';

            postFixup = ''
              patchelf --add-rpath ${lib.getLib pkgs.libGL}/lib $out/bin/${name}
            '';
          };
        };
    };
}

