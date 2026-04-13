# Anafestica — Project Guide for Claude Code

## What is this project?

Anafestica is a **header-only C++17 library** for persisting application settings
on Windows. It provides a hierarchical, heterogeneous in-memory container with
pluggable storage backends: **Windows Registry, JSON, XML, and INI files**.
It targets **C++Builder** (Embarcadero RAD Studio) and its Clang-based compilers
(`bcc32c`, `bcc64`, `bcc64x`).

## Repository layout

```text
anafestica/       — Public header files (the library itself)
App/              — Demo / sample application
Demo/             — Additional demo code
Test/             — Boost.Test unit tests
build/            — CMake build output (gitignored)
docs/             — Documentation assets (images, diagrams)
CMakeLists.txt    — Build system for tests (CMake + Ninja + bcc64x)
```

## Building and running tests

```powershell
mkdir build && cd build
cmake -G Ninja ..
ninja
ctest -V
```

- Compiler: `bcc64x` (Clang 20, C++Builder)
- Test framework: Boost.Test (`unit_test_framework`)
- Boost is auto-detected from the Embarcadero registry; override with `-DBOOST_ROOT=...`
- Tests require Windows, HKCU registry write access, and RTL/VCL support
- Use `ctest -V` (verbose) as the standard test command

## Code conventions

- **Header-only** — all library code lives in `anafestica/*.h`; do not add `.cpp` files to the library
- **C++17** standard; no C++20 features
- Variant selection is auto-detected in `CfgNodeValueType.h` via `__clang_major__`: `std::variant` for bcc64x (Clang ≥ 15), `boost::variant` for bcc64/bcc32c — no manual `ANAFESTICA_USE_STD_VARIANT` definition needed
- NVI (Non-Virtual Interface) pattern on `TConfig` — public non-virtual methods delegate to `protected virtual Do*` hooks
- RAII lifecycle: constructors load from storage, destructors flush back
- Embarcadero types are used throughout: `System::String`, `System::TDateTime`, `System::Currency`, `System::Sysutils::TBytes`
- Windows-only; no cross-platform pretense

## Commit message style

Follow **Conventional Commits** format as used in recent history:

```text
type(scope): short description
```

Examples: `feat(ini):`, `fix(xml):`, `test(config):`, `docs(tests):`, `docs:`

- `main` — stable branch
- `develop` — active development branch (default working branch)
- PRs target `main`

## Things to keep in mind

- The `build/` directory is ephemeral; never commit its contents
- Test coverage spans all 21 variant types × 4 backends; maintain this when adding types or backends
- The library is installed by cloning into `$(BDSCOMMONDIR)` — paths matter
- See [TESTS.md](TESTS.md) for the full test plan and coverage matrix
- See [LIBRARY_DOCUMENTATION.md](LIBRARY_DOCUMENTATION.md) for API docs
- See [QUICK_TOUR.md](QUICK_TOUR.md) for usage examples
