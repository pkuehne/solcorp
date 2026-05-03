{
  description =
    "Dev shell for a C++20 SDL2/Flecs/ImGui/Lua project (CMake + Ninja)";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.11";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };

        # Graphics/platform bits
        linuxLibs = with pkgs; [
          libGL
          xorg.libX11
          xorg.libXcursor
          xorg.libXext
          xorg.libXfixes
          xorg.libXi
          xorg.libXrandr
          xorg.libXrender
          xorg.libXinerama
          mesa
          mesa.drivers
          wayland
          libxkbcommon
          vulkan-loader
        ];

        darwinFrameworks = with pkgs.darwin.apple_sdk.frameworks; [
          Cocoa
          IOKit
          CoreVideo
          OpenGL
        ];

        # Deps that pkg-config needs for SDL2_*:
        sdlImageDeps = with pkgs; [ libpng libjpeg libtiff libwebp zlib lerc ];
        sdlTtfDeps = with pkgs; [
          freetype
          harfbuzz
          glib
          graphite2
          libsysprof-capture
          pcre2
        ];

        commonLibs = with pkgs;
          [
            SDL2
            SDL2_image
            SDL2_ttf
            SDL2_mixer
            lua5_4
            spdlog
            fmt
            imgui
            catch2_3
          ] ++ sdlImageDeps ++ sdlTtfDeps;

        tools = with pkgs; [
          cmake
          ninja
          clang-tools # clangd/clang-tidy (must precede clang so wrapped binaries win PATH)
          clang
          gdb
          pkg-config
          git
          patchelf
          wl-clipboard
          wayland-utils
          mesa-demos
          stylua
          lua54Packages.luacheck
        ];

        platformLibs = if pkgs.stdenv.isLinux then
          linuxLibs
        else if pkgs.stdenv.isDarwin then
          darwinFrameworks
        else
          [ ];
      in {
        devShells.default = pkgs.mkShell {
          buildInputs = commonLibs ++ platformLibs;
          nativeBuildInputs = tools;

          # Keep this out; it breaks transitive .pc discovery
          # PKG_CONFIG_PATH = ...

          shellHook = ''
            export CC=clang
            export CXX=clang++

            # Prefer WSLg sockets
            if [ -S /mnt/wslg/runtime-dir/wayland-0 ]; then
              export XDG_RUNTIME_DIR=/mnt/wslg/runtime-dir
              export WAYLAND_DISPLAY=wayland-0
            fi
            if [ -S /mnt/wslg/.X11-unix/X0 ]; then
              export DISPLAY=:0
              export XAUTHORITY=/mnt/wslg/.Xauthority
            fi

            # Make sure Mesa can find DRI/EGL drivers
            export LIBGL_DRIVERS_PATH="${pkgs.mesa.drivers}/lib/dri"''${LIBGL_DRIVERS_PATH:+:$LIBGL_DRIVERS_PATH}
            export LD_LIBRARY_PATH="${pkgs.mesa.drivers}/lib"''${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
            export EGL_PLATFORM=wayland
          '';
        };
      });
}
