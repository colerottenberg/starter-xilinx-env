{
  description = "C++ development environment — CLI-native, containerized Linux";

  inputs = {
    nixpkgs.url     = "github:NixOS/nixpkgs/nixos-24.11";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
        llvm = pkgs.llvmPackages_17;

        devPkgs = with pkgs; [
          # ── Compilers ──────────────────────────────────────────────────────
          llvm.clang          # clang / clang++
          llvm.lld            # fast linker
          llvm.lldb           # lldb debugger
          gcc13               # gcc / g++ available as fallback

          # ── Build ──────────────────────────────────────────────────────────
          cmake
          ninja
          gnumake
          pkg-config
          bear               # wraps any build to emit compile_commands.json
          just               # justfile task runner

          # ── LSP / analysis (feeds Neovim) ──────────────────────────────────
          llvm.clang-tools   # clangd, clang-tidy, clang-format
          cppcheck
          include-what-you-use

          # ── Sanitiser support ──────────────────────────────────────────────
          # ASan/TSan/UBSan ship with clang — nothing extra needed
          valgrind

          # ── Debuggers ─────────────────────────────────────────────────────
          gdb
          rr                 # record-replay debugger

          # ── Profiling ─────────────────────────────────────────────────────
          heaptrack

          # ── Editor ────────────────────────────────────────────────────────
          neovim

          # ── Shell ────────────────────────────────────────────────────────
          zsh

          # ── CLI utilities ─────────────────────────────────────────────────
          ripgrep
          fd
          jq
          fzf
          bat
          eza                # modern ls
          delta              # better git diffs
          git
          curl
          file
          tree
          strace
          ltrace
          hexdump

          # ── Docs ──────────────────────────────────────────────────────────
          man-pages
          man-pages-posix
        ];

      in {
        # ── Default devshell ─────────────────────────────────────────────────
        devShells.default = pkgs.mkShell {
          name = "cpp-dev";
          buildInputs = devPkgs;
          hardeningDisable = [ "all" ];  # dev env — easier debugging

          shellHook = ''
            export CC=clang
            export CXX=clang++
            export LD=lld
            export CCACHE_DIR="$HOME/.ccache"

            # Link compile_commands.json to repo root for clangd
            if [[ -f build/compile_commands.json && ! -L compile_commands.json ]]; then
              ln -sf build/compile_commands.json compile_commands.json
            fi

            echo ""
            echo "  ∷ cpp-dev  clang=$(clang --version | awk '{print $3}' | head -1)  cmake=$(cmake --version | awk '{print $3}' | head -1)  ninja=$(ninja --version)"
            echo "  ∷ run 'just' to see available tasks"
            echo ""
          '';
        };

        # ── Minimal CI shell (no interactive tooling) ────────────────────────
        devShells.ci = pkgs.mkShell {
          name = "cpp-ci";
          buildInputs = with pkgs; [
            llvm.clang llvm.lld
            cmake ninja pkg-config bear just
          ];
          shellHook = ''
            export CC=clang
            export CXX=clang++
            export LD=lld
          '';
        };
      }
    );
}
