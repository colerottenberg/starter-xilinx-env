cmake_minimum_required(VERSION 3.25)
project(myproject LANGUAGES CXX)

# ── Standards & baseline flags ────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD          23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

# ── compile_commands.json (clangd) ────────────────────────────────────────────
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Warning flags (applied to all targets via interface lib) ──────────────────
add_library(project_warnings INTERFACE)
target_compile_options(project_warnings INTERFACE
  -Wall -Wextra -Wpedantic
  -Wshadow
  -Wnon-virtual-dtor
  -Wold-style-cast
  -Wcast-align
  -Wunused
  -Woverloaded-virtual
  -Wconversion
  -Wsign-conversion
  -Wdouble-promotion
  -Wformat=2
  -Wimplicit-fallthrough
  # clang-specific
  $<$<CXX_COMPILER_ID:Clang>:-Wno-gnu-zero-variadic-macro-arguments>
)

# ── Source ────────────────────────────────────────────────────────────────────
add_subdirectory(src)

# ── Tests ─────────────────────────────────────────────────────────────────────
include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
