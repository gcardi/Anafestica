# Anafestica Test Plan

This document describes how to run tests, configuration rules, and current test coverage.

## 1. Build and run tests

The three C++Builder `.cbproj` test projects are built and run with MSBuild
using the provided `test_all.bat` script.

```powershell
test_all.bat                        # build + run all three compilers
test_all.bat bcc64x                 # build + run bcc64x only
test_all.bat bcc32c bcc64           # build + run two toolchains
test_all.bat --rebuild              # clean + rebuild all three
test_all.bat --no-build             # skip build, run existing executables only
test_all.bat --stop-on-error        # abort on first failure
test_all.bat --verbose-build        # show full MSBuild/compiler commands
test_all.bat --with-yaml            # opt in to YAML tests when fkYAML is registered
```

The build is incremental by default (MSBuild `/t:Make`); use `--rebuild` to
force a clean rebuild (`/t:Clean,Build`). By default the script uses MSBuild
`/v:minimal`, so the exact compiler command lines shown can differ by
toolchain; use `--verbose-build` for consistent full command output. The script
calls `rsvars.bat` to set up the Embarcadero environment, builds all selected
targets first, then runs the tests only if all builds succeeded:

YAML tests are optional. By default, `test_all.bat` does not define
`ANAFESTICA_TEST_YAML`, so `CfgYAML.h` is not included by the test suite and
fkYAML is not required. Use `--with-yaml` to request YAML coverage. If
`--with-yaml` is present but `<fkYAML/node.hpp>` is not visible through the
active Clang-based toolchain include path, the YAML test block is compiled out
automatically.

| Project | Platform | Compiler | Executable |
| ------- | -------- | -------- | ---------- |
| `Test\TestBcc32c\anafestica_test_bcc32c.cbproj` | Win32 | bcc32c | `Win32\Release\anafestica_test_bcc32c.exe` |
| `Test\TestBcc64\anafestica_test_bcc64.cbproj` | Win64 | bcc64 | `Win64\Release\anafestica_test_bcc64.exe` |
| `Test\TestBcc64x\anafestica_test_bcc64x.cbproj` | Win64x | bcc64x | `Win64x\Release\anafestica_test_bcc64x.exe` |

At the end it prints a summary with pass/fail/skip counts and exits with
a non-zero code if any test suite failed.

### Test-case counts per toolchain

The three suites do not run exactly the same set of cases. Default per-file
case counts, without `--with-yaml`:

| File | bcc32c | bcc64 | bcc64x |
| ---- | :----: | :---: | :----: |
| `test_config.cpp` | 107 | 107 | 122 |
| `test_config_simplified.cpp` | 19 | 19 | 19 |
| `test_node_ops.cpp` | 20 | 20 | 20 |
| `test_type_mismatch.cpp` | 25 | 25 | 25 |
| `test_types.cpp` | 7 | 7 | 7 |
| `test_singleton_version_info.cpp` | 2 | 2 | 2 |
| `test_bccXX_variant_compat.cpp` | 2 | 2 | — |
| **Total** | **182** | **182** | **195** |

With `--with-yaml` and fkYAML available to the selected toolchain include
path, the YAML block adds 22 cases on `bcc32c` / `bcc64` and 25 cases on
`bcc64x`:

| File | bcc32c | bcc64 | bcc64x |
| ---- | :----: | :---: | :----: |
| `test_config.cpp` | 129 | 129 | 147 |
| **Total** | **204** | **204** | **220** |

Despite not having a `variant_compat` module, bcc64x now reports **more** cases
than the two boost-variant toolchains. The net +13 delta breaks down as:

- **+15** by default in `test_config.cpp` — extra roundtrip cases for `str`
  and `wstr` across the five default backends, plus enum and
  `string_view`/`wstring_view`
  write-convenience tests that only compile on the `std::variant` path.
- **+18** with `--with-yaml` and fkYAML available, because the optional YAML
  backend adds the corresponding `std::string`, `std::wstring`, and
  `string_view` / `wstring_view` cases on `bcc64x`.
- `Test\Shared\test_config_simplified.cpp` now provides the shared 19-type
  subset for all three toolchains, so parity with the legacy toolchains is
  restored without duplicating the bcc64x-only
  `std::string`/`std::wstring` coverage.
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
- Anafestica include path registered in RAD Studio's per-user C++ include settings; `register_anafestica.bat` can update the `HKCU\Software\Embarcadero\BDS\XX.X\C++\Paths` entries automatically, and `register_anafestica.bat --dry-run` previews the change
- fkYAML headers registered in RAD Studio's include search path only when running `test_all.bat --with-yaml`
- `register_fkYAML.bat` can register the fkYAML include directory automatically when the repository is installed next to Anafestica under `$(BDSCOMMONDIR)`
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

Most of the suite now lives under `Test\Shared` and is compiled into all three
toolchain-specific `.cbproj` projects. The remaining per-toolchain files are
the `boost::variant` compatibility modules in `Test\TestBcc32c` and
`Test\TestBcc64`, plus the bcc64x-specific `runner.cpp`.

`Test/Shared/test_types.cpp` covers low-level registry primitives (QWORD,
MultiSz, binary, expand-string).

`Test/Shared/test_config.cpp` covers full roundtrip through the five default
backends, plus the optional YAML backend when `test_all.bat --with-yaml` is
used and fkYAML is available. It builds as 122 default cases on bcc64x (all 21
`std::variant` alternatives plus the bcc64x-only convenience tests) and as 107
default cases on bcc64/bcc32c (the shared 19-type subset). With YAML enabled,
those counts become 147 and 129 respectively.

`Test/Shared/test_config_simplified.cpp` provides the shared 19-type subset for
all three toolchains without introducing any `boost::variant`-specific
behaviour on the `bcc64x` path.

The matrix includes the optional YAML backend. Its column applies only when
`--with-yaml` is used and fkYAML is available on the RAD Studio include search
path for the active compiler platform.

| Tag | Type | Registry | JSON | BSON | YAML opt-in | XML | INI |
| --- | ---- | :------: | :--: | :--: | :---------: | :-: | :-: |
| `i` | int | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| `u` | unsigned int | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| `l` | long | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| `ul` | unsigned long | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| `c` | char | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| `uc` | unsigned char | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| `s` | short | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| `us` | unsigned short | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| `ll` | long long | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| `ull` | unsigned long long | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| `b` | bool | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| `sz` | System::String | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| `dt` | System::TDateTime | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| `flt` | float | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| `dbl` | double | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| `cur` | System::Currency | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| `sv` | StringCont (vector\<String\>) | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| `dab` | TBytes | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| `vb` | BytesCont (vector\<Byte\>) | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| `str` | std::string (UTF-8) — bcc64x only | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| `wstr` | std::wstring (UTF-16) — bcc64x only | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |

`Test/Shared/test_config.cpp` also includes explicit enum roundtrip tests for all
five default backends, plus YAML when `--with-yaml` is active and fkYAML is
available:

- `Registry_enum_roundtrip`
- `JSON_enum_roundtrip`
- `BSON_enum_roundtrip`
- `YAML_enum_roundtrip`
- `INIFile_enum_roundtrip`
- `XML_enum_roundtrip`

`string_view` / `wstring_view` write-convenience overloads are also tested for
all five default backends, and for YAML when enabled (bcc64x only).

### Type-mismatch tests

`Test/Shared/test_type_mismatch.cpp` verifies that reading a value with a C++ type
that differs from the type tag stored by the backend silently returns the
default-initialised value (`T{}`).  The variant alternative written by
`PutItem<A>()` does not match the `std::get_if<B>()` performed by
`GetItem<B>()`, so the assignment is skipped and the caller gets `T{}`.

Five representative mismatch pairs are tested across the five backends covered
by `test_type_mismatch.cpp` (Registry, JSON, BSON, INI, XML):

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
  `TConfigNode_JSON_Erase`, `TConfigNode_BSON_Erase`,
  `TConfigNode_XML_Erase`, `TConfigNode_INIFile_Erase`)
  populate values and a sub-node, flush, reopen, delete one value and the
  sub-node, flush, and reopen again to verify the removed names are gone.

One known backend gap is surfaced as `BOOST_WARN_MESSAGE` rather than a hard
failure so the suite still passes:

- **Registry**: `DeleteSubNode` does not always remove the empty sub-key
  across a flush/reopen cycle.

## 4. Quick checklist

- [x] Builds (MSBuild via `test_all.bat`)
- [x] `test_all.bat` passes all three compilers
- [x] All 21 variant types covered (bcc64x) / 19 variant types (bcc64, bcc32c) across Registry, JSON, BSON, XML, and INI backends by default; YAML is covered with `--with-yaml` when fkYAML is available
- [x] Enum roundtrip covered across Registry, JSON, BSON, XML, and INI backends by default; YAML is covered with `--with-yaml` when fkYAML is available
- [x] Type-mismatch silent-default behaviour covered across Registry, JSON, BSON, XML, and INI backends
- [x] `TConfigNode` in-memory operations and per-backend erase persistence covered
