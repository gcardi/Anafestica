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
	Shared/         — Shared test modules compiled by all toolchains
	TestBcc32c/     — Win32 project + bcc32c-specific compatibility files
	TestBcc64/      — Win64 project + bcc64-specific compatibility files
	TestBcc64x/     — Win64x project + bcc64x-specific runner
docs/             — Documentation assets (images, diagrams)
test_all.bat      — Build and run all three test suites via MSBuild
clear_tests.bat   — Remove test build artifacts (selective or all)
```

## Building and running tests

```powershell
test_all.bat                        # build + run all three compilers
test_all.bat bcc64x                 # build + run bcc64x only
test_all.bat --rebuild bcc32c bcc64 # clean + rebuild two toolchains
test_all.bat --no-build             # run existing executables only
test_all.bat --stop-on-error        # abort on first failure
test_all.bat --verbose-build        # show full MSBuild/compiler commands

clear_tests.bat                     # remove build artifacts for all three
clear_tests.bat bcc64x              # remove bcc64x artifacts only
```

- Compilers: `bcc32c`, `bcc64`, `bcc64x` (C++Builder)
- Test framework: Boost.Test (`unit_test_framework`) for all three test executables
- Tests require Windows, HKCU registry write access, and RTL/VCL support

## Code conventions

- **Header-only** — all library code lives in `anafestica/*.h`; do not add `.cpp` files to the library
- **C++17** standard; no C++20 features
- Variant selection is auto-detected in `CfgNodeValueType.h` via `__clang_major__`: `std::variant` for bcc64x (Clang ≥ 15), `boost::variant` for bcc64/bcc32c — no manual `ANAFESTICA_USE_STD_VARIANT` definition needed
- The library itself does not require Boost on the bcc64x `std::variant` path, but the bundled test harness still uses Boost.Test on bcc64x
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

- Test coverage spans all 21 variant types × 4 backends on bcc64x, plus a shared 19-type simplified module across all three toolchains; maintain this when adding types or backends
- The library is installed by cloning into `$(BDSCOMMONDIR)` — paths matter
- See [TESTS.md](TESTS.md) for the full test plan and coverage matrix
- See [LIBRARY_DOCUMENTATION.md](LIBRARY_DOCUMENTATION.md) for API docs
- See [QUICK_TOUR.md](QUICK_TOUR.md) for usage examples

