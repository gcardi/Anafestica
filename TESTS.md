# Anafestica Test Plan

This document describes how to run tests, configuration rules, and current test coverage.

## 1. Build and run tests

The three C++Builder `.cbproj` test projects are built and run with MSBuild
using the provided `test_all.bat` script.

```powershell
test_all.bat                        # build + run all three compilers
test_all.bat bcc64x                 # build + run bcc64x only
test_all.bat bcc32c bcc64           # build + run two toolchains
test_all.bat --rebuild              # force full recompile for all three
test_all.bat --no-build             # skip build, run existing executables only
test_all.bat --stop-on-error        # abort on first failure
```

The build is incremental by default (MSBuild `/t:Make`); use `--rebuild` to
force a full recompile (`/t:Build`). The script calls `rsvars.bat` to set up
the Embarcadero environment, builds all selected targets first, then runs the
tests only if all builds succeeded:

| Project | Platform | Compiler | Executable |
| ------- | -------- | -------- | ---------- |
| `Test\TestBcc32c\anafestica_test_bcc32c.cbproj` | Win32 | bcc32c | `Win32\Release\anafestica_test_bcc32c.exe` |
| `Test\TestBcc64\anafestica_test_bcc64.cbproj` | Win64 | bcc64 | `Win64\Release\anafestica_test_bcc64.exe` |
| `Test\TestBcc64x\anafestica_test_bcc64x.cbproj` | Win64x | bcc64x | `Win64x\Release\anafestica_test_bcc64x.exe` |

At the end it prints a summary with pass/fail/skip counts and exits with
a non-zero code if any test suite failed.

### Test-case counts per toolchain

The three suites do not run exactly the same set of cases. Per-file case counts:

| File | bcc32c | bcc64 | bcc64x |
| ---- | :----: | :---: | :----: |
| `test_config.cpp` | 85 | 85 | 97 |
| `test_config_simplified.cpp` | 19 | 19 | 19 |
| `test_node_ops.cpp` | 19 | 19 | 19 |
| `test_type_mismatch.cpp` | 20 | 20 | 20 |
| `test_types.cpp` | 7 | 7 | 7 |
| `test_singleton_version_info.cpp` | 2 | 2 | 2 |
| `test_bccXX_variant_compat.cpp` | 2 | 2 | — |
| **Total** | **154** | **154** | **164** |

Despite not having a `variant_compat` module, bcc64x now reports **more** cases
than the two boost-variant toolchains. The net +10 delta breaks down as:

- **+12** in `test_config.cpp` — extra roundtrip cases for `str` and `wstr`
  across the four backends, plus enum and `string_view`/`wstring_view`
  write-convenience tests that only compile on the `std::variant` path.
- `test_config_simplified.cpp` now exists under `Test\TestBcc64x` as the
  shared 19-type subset, so parity with the legacy toolchains is restored
  without duplicating the bcc64x-only `std::string`/`std::wstring` coverage.
- **−2** — no `test_bcc64x_variant_compat.cpp` exists. This one **is**
  intentional: the `variant_compat` files pin down `boost::variant` quirks
  that are specific to the pre-Clang-15 toolchains and do not apply on the
  `std::variant` path used by bcc64x.

### Cleaning build artifacts

To remove all test build output directories, use the `clear_tests.bat` script:

```powershell
clear_tests.bat                     # clean all three toolchains
clear_tests.bat bcc64x              # clean bcc64x only
clear_tests.bat bcc32c bcc64        # clean two toolchains
```

This deletes the `Win32`, `Win64`, and/or `Win64x` output directories under
each test project folder. It does not require MSBuild or `rsvars.bat`.

---

## 2. Runtime requirements for test suite

- Windows
- C++Builder with RTL/VCL support
- Boost.Test `unit_test_framework` for all three test executables, even though the library code on `bcc64x` itself uses the `std::variant` path and does not require Boost for value storage
- HKCU registry write permission (required by registry tests)

---

## 3. Current coverage for `TConfigNodeValueType`

`TConfigNodeValueType` holds **21 alternatives on bcc64x** (std::variant path, Clang ≥ 15)
and **19 alternatives on bcc64/bcc32c** (boost::variant path) — the two C++ string types
are absent there because Boost 1.70's `mpl::list` is hardcoded to a 20-type limit:

- int, unsigned int, long, unsigned long, char, unsigned char, short, unsigned short
- long long, unsigned long long, bool, System::String
- System::TDateTime, float, double, System::Currency
- StringCont (`std::vector<String>`), System::Sysutils::TBytes, BytesCont (`std::vector<Byte>`)
- **std::string** (UTF-8, tag `str`) — bcc64x only
- **std::wstring** (UTF-16, tag `wstr`) — bcc64x only

`Test/TestBcc64x/test_types.cpp` covers low-level registry primitives (QWORD, MultiSz, binary, expand-string).

`Test/TestBcc64x/test_config.cpp` covers full roundtrip through all four backends for all 21 types (bcc64x).
`Test/TestBcc64/test_config.cpp` covers the same four backends for the 19 common types (bcc64).
`Test/TestBcc64x/test_config_simplified.cpp` adds the shared 19-type registry-only subset to the `bcc64x` suite as well, restoring per-module parity with `bcc32c` and `bcc64` without introducing any `boost::variant`-specific behaviour on the `bcc64x` path.

| Tag | Type | Registry | JSON | XML | INI |
| --- | ---- | :------: | :--: | :-: | :-: |
| `i` | int | ✓ | ✓ | ✓ | ✓ |
| `u` | unsigned int | ✓ | ✓ | ✓ | ✓ |
| `l` | long | ✓ | ✓ | ✓ | ✓ |
| `ul` | unsigned long | ✓ | ✓ | ✓ | ✓ |
| `c` | char | ✓ | ✓ | ✓ | ✓ |
| `uc` | unsigned char | ✓ | ✓ | ✓ | ✓ |
| `s` | short | ✓ | ✓ | ✓ | ✓ |
| `us` | unsigned short | ✓ | ✓ | ✓ | ✓ |
| `ll` | long long | ✓ | ✓ | ✓ | ✓ |
| `ull` | unsigned long long | ✓ | ✓ | ✓ | ✓ |
| `b` | bool | ✓ | ✓ | ✓ | ✓ |
| `sz` | System::String | ✓ | ✓ | ✓ | ✓ |
| `dt` | System::TDateTime | ✓ | ✓ | ✓ | ✓ |
| `flt` | float | ✓ | ✓ | ✓ | ✓ |
| `dbl` | double | ✓ | ✓ | ✓ | ✓ |
| `cur` | System::Currency | ✓ | ✓ | ✓ | ✓ |
| `sv` | StringCont (vector\<String\>) | ✓ | ✓ | ✓ | ✓ |
| `dab` | TBytes | ✓ | ✓ | ✓ | ✓ |
| `vb` | BytesCont (vector\<Byte\>) | ✓ | ✓ | ✓ | ✓ |
| `str` | std::string (UTF-8) — bcc64x only | ✓ | ✓ | ✓ | ✓ |
| `wstr` | std::wstring (UTF-16) — bcc64x only | ✓ | ✓ | ✓ | ✓ |

`Test/TestBcc64x/test_config.cpp` also includes explicit enum roundtrip tests for all
four backends:

- `Registry_enum_roundtrip`
- `JSON_enum_roundtrip`
- `INIFile_enum_roundtrip`
- `XML_enum_roundtrip`

`string_view` / `wstring_view` write-convenience overloads are also tested for all four backends (bcc64x only).

### Type-mismatch tests

`Test/TestBcc64x/test_type_mismatch.cpp` (shared with the bcc64 target) verifies that reading a value with a C++ type
that differs from the type tag stored by the backend silently returns the
default-initialised value (`T{}`).  The variant alternative written by
`PutItem<A>()` does not match the `std::get_if<B>()` performed by
`GetItem<B>()`, so the assignment is skipped and the caller gets `T{}`.

Five representative mismatch pairs are tested across all four backends:

| Stored as | Read as | Why |
| --------- | ------- | --- |
| `int` (−42) | `double` | Two numeric types that users can confuse, but they are distinct variant alternatives |
| `String` ("Hello") | `int` | String-to-numeric: common real-world mistake (value has tag `sz`, code reads as `i`) |
| `int` (−42) | `String` | Reverse direction: numeric stored, code expects a string |
| `bool` (true) | `int` | C++ converts bool↔int implicitly, but the variant treats them as separate alternatives |
| `double` (3.14…) | `float` | Closely related FP types, easy to mix up, yet distinct in the variant |

### Node-operation tests

`test_node_ops.cpp` (built into all three test targets) exercises `TConfigNode`
members that were previously untested:

- **In-memory suite (`TConfigNode_InMemory`)** — `PutItem`, `GetSubNode`,
  `operator[]`, `DeleteItem` (soft-erase semantics), `DeleteSubNode` (marks
  child deleted via `Clear()`), `Clear` (recursive), `ItemExists`,
  `SubNodeExists`, `GetNodeCount`, `GetValueCount`, `EnumerateNodes`,
  `EnumerateValueNames`, `EnumerateValues`, `IsDeleted`, `IsModified`, plus
  depth-limit guards for persistence `Read` / `Write`.
- **Per-backend erase-persistence suites** (`TConfigNode_Registry_Erase`,
  `TConfigNode_JSON_Erase`, `TConfigNode_XML_Erase`, `TConfigNode_INIFile_Erase`)
  populate values and a sub-node, flush, reopen, delete one value and the
  sub-node, flush, and reopen again to verify the removed names are gone.

One known backend gap is surfaced as `BOOST_WARN_MESSAGE` rather than a hard
failure so the suite still passes:

- **Registry**: `DeleteSubNode` does not always remove the empty sub-key
  across a flush/reopen cycle.

## 4. Quick checklist

- [x] Builds (MSBuild via `test_all.bat`)
- [x] `test_all.bat` passes all three compilers
- [x] All 21 variant types covered (bcc64x) / 19 variant types (bcc64, bcc32c) across Registry, JSON, XML, and INI backends
- [x] Enum roundtrip covered across Registry, JSON, XML, and INI backends
- [x] Type-mismatch silent-default behaviour covered across all four backends
- [x] `TConfigNode` in-memory operations and per-backend erase persistence covered
