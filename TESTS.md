# Anafestica Test Plan

This document describes how to run tests, configuration rules, and current test coverage.

## 1. Build and run tests with CMake/Ninja

1. Create build folder:

```powershell
mkdir build
cd build
```

1. Configure with CMake (Ninja):

```powershell
cmake -G Ninja ..
# if needed
cmake -G Ninja -DBOOST_ROOT="C:\\Program Files (x86)\\Embarcadero\\Studio\\37.0" ..
```

1. Build:

```powershell
ninja
# or
cmake --build .
```

1. Run tests:

```powershell
cd build
.\anafestica_test.exe
```

> **Note:** Do not use `ctest` or `ctest -V` to run the tests. Due to how ctest
> redirects stdout to a pipe, the Embarcadero RTL crashes with a segmentation
> fault / access violation mid-run. The same binary runs correctly when launched
> directly. Always invoke `anafestica_test.exe` directly.

1. Clean:

```powershell
cd ..
rd /S /Q build
```

---

## 2. Runtime requirements for test suite

- Windows
- C++Builder with RTL/VCL support
- Boost unit_test_framework (linked in `CMakeLists.txt`)
- HKCU registry write permission (required by registry tests)

---

## 3. Current coverage for `TConfigNodeValueType`

`TConfigNodeValueType` is a variant that includes 21 alternatives:

- int, unsigned int, long, unsigned long, char, unsigned char, short, unsigned short
- long long, unsigned long long, bool, System::String
- System::TDateTime, float, double, System::Currency
- StringCont (`std::vector<String>`), System::Sysutils::TBytes, BytesCont (`std::vector<Byte>`)
- **std::string** (UTF-8, tag `str`)
- **std::wstring** (UTF-16, tag `wstr`)

`Test/test_types.cpp` covers low-level registry primitives (QWORD, MultiSz, binary, expand-string).

`Test/test_config.cpp` covers full roundtrip through all four backends for all 21 types:

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
| `str` | std::string (UTF-8) | ✓ | ✓ | ✓ | ✓ |
| `wstr` | std::wstring (UTF-16) | ✓ | ✓ | ✓ | ✓ |

`string_view` / `wstring_view` write-convenience overloads are also tested for all four backends.

## 4. Missing test cases to add

All 21 variant types are now covered across all four backends (Registry, JSON, XML, INI). Potential future additions:

- `GetTypeTag` validation for all text-to-type tag round-trips
- Error/exception path tests for malformed JSON/XML input

---

## 5. CTest integration notes

- `CMakeLists.txt` registers the test via `add_test()` for potential CI/CDash
  integration, but **ctest cannot be used to run tests locally**.
- Running via ctest causes a segmentation fault / access violation mid-run.
  The root cause is that ctest redirects the child process stdout to a pipe;
  the Embarcadero RTL then switches to full block-buffering and crashes before
  flushing any output, making the failure impossible to diagnose through ctest.
- **Always run `anafestica_test.exe` directly.**

---

## 6. Quick checklist

- [x] Builds
- [x] `anafestica_test.exe` passes directly
- [ ] `ctest -V` — known crash (SEGFAULTs due to Embarcadero RTL + pipe redirection incompatibility)
- [x] All 21 variant types covered across Registry, JSON, XML, and INI backends
