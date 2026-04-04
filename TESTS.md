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
ctest -V
```

> **Note:** `ctest -V` is the recommended command because it provides verbose
> per-test output, which is helpful when diagnosing failures.

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

`Test/test_config.cpp` also includes explicit enum roundtrip tests for all
four backends:

- `Registry_enum_roundtrip`
- `JSON_enum_roundtrip`
- `INIFile_enum_roundtrip`
- `XML_enum_roundtrip`

`string_view` / `wstring_view` write-convenience overloads are also tested for all four backends.

## 4. Missing test cases to add

All 21 variant types are now covered across all four backends (Registry, JSON, XML, INI). Potential future additions:

- `GetTypeTag` validation for all text-to-type tag round-trips
- Error/exception path tests for malformed JSON/XML input

---

## 5. CTest integration notes

- `CMakeLists.txt` registers the test via `add_test()` for local use and
  CI/CDash integration.
- Use `ctest -V` as the default test runner so each test case and failure
  context is printed verbosely.
- Running `anafestica_test.exe` directly is still possible for targeted
  debugging, but `ctest -V` is the recommended command for normal runs.

---

## 6. Quick checklist

- [x] Builds
- [x] `ctest -V` passes
- [x] `anafestica_test.exe` passes directly
- [x] All 21 variant types covered across Registry, JSON, XML, and INI backends
- [x] Enum roundtrip covered across Registry, JSON, XML, and INI backends
