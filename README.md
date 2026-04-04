# cpp-nix-devenv

CLI-native C++ development environment.  
Toolchain managed by Nix.  Production target is Linux regardless of host OS.

---

## Architecture

```
host (macOS / Windows / Linux)
  └─ Docker container  [Debian + Nix]
       └─ nix develop  [clang17, cmake, ninja, gdb, rr, clangd, ...]
            └─ /workspace  ←──── bind-mount of your project root
```

Your source lives on the host — edit with your native Neovim.  
Everything else (compiler, linker, debugger, linter) runs inside the container on Linux.  
`compile_commands.json` is written to the project root so your host clangd/LSP picks it up transparently.

---

## First time

```sh
# Build the container image (takes a few minutes — downloads the Nix store)
just docker-build

# Or manually:
docker compose build
```

---

## Daily workflow

```sh
# Open a shell inside the container
just docker-shell
# equivalent: docker compose run --rm dev

# Inside the container — all just recipes are available:
just            # list recipes
just build      # configure + build (debug preset)
just test       # build + run CTest
just fmt        # clang-format all sources in-place
just tidy       # clang-tidy
just asan       # build + test under ASan+UBSan
just tsan       # build + test under TSan

# Run a single just recipe without entering the shell:
just docker-run build release
just docker-run test release
```

---

## Build presets

| Preset          | Use case                        |
|-----------------|---------------------------------|
| `debug`         | Day-to-day dev, full debug info |
| `release`       | Benchmarks, final binaries      |
| `relwithdebinfo`| Profiling (heaptrack, perf)     |
| `asan`          | Memory / UB errors              |
| `tsan`          | Data races                      |
| `msan`          | Uninitialised reads (clang only)|

```sh
just build release
just test  asan
PRESET=tsan just build   # override via env var
```

---

## clangd / LSP on the host

After any `just configure` (or `just build`), `compile_commands.json` is symlinked  
to the project root.  Your host Neovim + clangd picks it up automatically.

---

## Debugging

```sh
# gdb
just gdb <binary-name>

# lldb
just lldb <binary-name>

# rr — record a run, replay deterministically
just rr-record <binary-name>
just rr-replay

# valgrind memcheck
just valgrind <binary-name>
```

---

## Volumes

| Volume      | Purpose                                        |
|-------------|------------------------------------------------|
| `nix-store` | Persists the Nix package store across restarts |
| `ccache`    | Persists ccache across rebuilds                |

---

## Host UID mismatch

If files created inside the container are owned by root on the host, set:

```sh
USER_UID=$(id -u) USER_GID=$(id -g) docker compose build
```

Or export those vars in your shell's `.env`.
