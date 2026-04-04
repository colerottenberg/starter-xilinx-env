# ─────────────────────────────────────────────────────────────────────────────
# C++ Dev — justfile
# run `just` to list all recipes
# ─────────────────────────────────────────────────────────────────────────────

set shell := ["bash", "-euo", "pipefail", "-c"]
set dotenv-load := true

# Dirs
build_dir    := "build"
src_dir      := "src"
preset       := env_var_or_default("PRESET", "debug")

# ── Default: list recipes ─────────────────────────────────────────────────────
[private]
default:
    @just --list --unsorted

# ── Configure ─────────────────────────────────────────────────────────────────

# Configure with a named CMake preset  [debug|release|relwithdebinfo|asan|tsan|msan]
configure preset=preset:
    cmake --preset {{preset}}
    @# Keep compile_commands.json at repo root for clangd
    @ln -sf {{build_dir}}/{{preset}}/compile_commands.json compile_commands.json 2>/dev/null || true
    @echo "→ configured preset '{{preset}}'"

# ── Build ─────────────────────────────────────────────────────────────────────

# Build current preset (configures first if needed)
build preset=preset: (configure preset)
    cmake --build --preset {{preset}} --parallel

# Build + run in one shot
run preset=preset target="": (build preset)
    #!/usr/bin/env bash
    bin=$(find {{build_dir}}/{{preset}} -maxdepth 2 -type f -executable | head -1)
    [[ -n "{{target}}" ]] && bin="{{build_dir}}/{{preset}}/{{target}}"
    echo "→ running $bin"
    exec "$bin"

# ── Clean ─────────────────────────────────────────────────────────────────────

# Remove a specific preset's build dir
clean preset=preset:
    rm -rf {{build_dir}}/{{preset}}
    @echo "→ cleaned {{build_dir}}/{{preset}}"

# Nuke entire build directory
clean-all:
    rm -rf {{build_dir}} compile_commands.json
    @echo "→ cleaned all build artifacts"

# ── Test ──────────────────────────────────────────────────────────────────────

# Run CTest for a preset
test preset=preset: (build preset)
    ctest --preset {{preset}} --output-on-failure

# Run tests matching a pattern
test-filter pattern preset=preset: (build preset)
    ctest --preset {{preset}} --output-on-failure -R "{{pattern}}"

# ── Sanitisers ────────────────────────────────────────────────────────────────

# Build + test under AddressSanitizer + UBSan
asan: (test "asan")

# Build + test under ThreadSanitizer
tsan: (test "tsan")

# Build + test under MemorySanitizer
msan: (test "msan")

# ── Static analysis ───────────────────────────────────────────────────────────

# Run clang-tidy across all source files
tidy preset=preset: (configure preset)
    #!/usr/bin/env bash
    mapfile -t files < <(find {{src_dir}} -name '*.cpp' -o -name '*.cc' -o -name '*.cxx')
    echo "→ clang-tidy on ${#files[@]} files"
    clang-tidy \
      -p {{build_dir}}/{{preset}} \
      --use-color \
      "${files[@]}"

# Run cppcheck
cppcheck:
    cppcheck \
      --enable=all \
      --inconclusive \
      --suppress=missingIncludeSystem \
      --project=compile_commands.json \
      --error-exitcode=1

# Check includes with include-what-you-use
iwyu preset=preset: (configure preset)
    iwyu_tool.py -p {{build_dir}}/{{preset}} -- -Xiwyu --no_fwd_decls

# ── Formatting ────────────────────────────────────────────────────────────────

# Format all source files in-place
fmt:
    find {{src_dir}} -name '*.cpp' -o -name '*.cc' -o -name '*.cxx' \
                     -o -name '*.h'  -o -name '*.hpp' \
      | xargs clang-format -i
    @echo "→ formatted"

# Check formatting without modifying files (CI)
fmt-check:
    find {{src_dir}} -name '*.cpp' -o -name '*.cc' -o -name '*.cxx' \
                     -o -name '*.h'  -o -name '*.hpp' \
      | xargs clang-format --dry-run --Werror
    @echo "→ format OK"

# ── Debugging ─────────────────────────────────────────────────────────────────

# Launch gdb on a binary
gdb target preset=preset: (build preset)
    gdb -ex "set pagination off" -ex run --args {{build_dir}}/{{preset}}/{{target}}

# Launch lldb on a binary
lldb target preset=preset: (build preset)
    lldb {{build_dir}}/{{preset}}/{{target}}

# Record execution with rr, then replay
rr-record target preset=preset: (build preset)
    rr record {{build_dir}}/{{preset}}/{{target}}

rr-replay:
    rr replay

# ── Profiling ─────────────────────────────────────────────────────────────────

# Heap profile a binary with heaptrack
heaptrack target preset="relwithdebinfo": (build preset)
    heaptrack {{build_dir}}/{{preset}}/{{target}}
    @echo "→ open the .gz with: heaptrack_gui <file>"

# Valgrind memcheck
valgrind target preset=preset: (build preset)
    valgrind \
      --leak-check=full \
      --show-leak-kinds=all \
      --track-origins=yes \
      --error-exitcode=1 \
      {{build_dir}}/{{preset}}/{{target}}

# ── Utilities ─────────────────────────────────────────────────────────────────

# Dump compile_commands.json entry for a file (quick clangd debug)
cc-entry file:
    jq '.[] | select(.file | endswith("{{file}}"))' compile_commands.json

# Count lines of code
loc:
    @find {{src_dir}} \( -name '*.cpp' -o -name '*.cc' -o -name '*.h' -o -name '*.hpp' \) \
      | xargs wc -l | sort -rn | head -20

# ── Docker shortcuts (run from host) ─────────────────────────────────────────

# Build the dev container image
docker-build:
    docker compose build

# Open an interactive shell in the container
docker-shell:
    docker compose run --rm dev

# Run a just recipe inside the container
docker-run +args:
    docker compose run --rm dev just {{args}}

# ── CI gate (runs everything non-interactive) ─────────────────────────────────
ci: fmt-check (configure "release") (build "release") (test "release") tidy cppcheck
    @echo "✓ CI passed"
