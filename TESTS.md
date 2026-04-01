# Anafestica Test Plan

This document describes how to run tests, configuration rules, and current test coverage.

## 1. Build and run tests with CMake/Ninja

1. Create build folder:

```powershell
mkdir build
cd build
```

2. Configure with CMake (Ninja):

```powershell
cmake -G Ninja ..
# if needed
cmake -G Ninja -DBOOST_ROOT="C:\\Program Files (x86)\\Embarcadero\\Studio\\37.0" ..
```

3. Build:

```powershell
ninja
# or
cmake --build .
```

4. Run tests:

```powershell
cd build
.\anafestica_test.exe
```

> **Note:** Do not use `ctest` or `ctest -V` to run the tests. Due to how ctest
> redirects stdout to a pipe, the Embarcadero RTL crashes with a segmentation
> fault / access violation mid-run. The same binary runs correctly when launched
> directly. Always invoke `anafestica_test.exe` directly.

6. Clean:

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

`TConfigNodeValueType` is a variant that includes:
- int, unsigned int, long, unsigned long, char, unsigned char, short, unsigned short
- long long, unsigned long long, bool, System::String,
- System::TDateTime, float, double, System::Currency,
- StringCont (`std::vector<String>`), System::Sysutils::TBytes, BytesCont (`std::vector<Byte>`)

Existing tests in `Test/test_types.cpp` cover:
- `TT_ULL` (`unsigned long long`): roundtrip and type mismatch behavior (QWORD)
- `TT_SV` (`StringCont`): MultiSz roundtrip
- `TT_DAB`, `TT_VB` (Sysutils::TBytes and vector<Byte>): roundtrip and mismatch behavior
- `TT_SZ` (System::String): ExpandString semantics

## 4. Missing test cases to add

- `TT_I`, `TT_U`, `TT_L`, `TT_UL`, `TT_C`, `TT_UC`, `TT_S`, `TT_US`, `TT_LL`, `TT_B`
- `TT_DT` (System::TDateTime)
- `TT_FLT`, `TT_DBL`, `TT_CUR`
- `GetTypeTag` validation for all text-to-type tags
- `TConfigNodeValueType` conversion/serialization in JSON/XML/Registry

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
- [ ] Add tests covering all variant types
