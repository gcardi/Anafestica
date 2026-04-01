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
ctest -V
```

5. For extended output on failure (CI):

```powershell
cd build
ctest --output-on-failure -V
```

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

- `CMakeLists.txt` already contains:

```cmake
set(CTEST_OUTPUT_ON_FAILURE TRUE CACHE INTERNAL "Show output when test fails")
```

- Always use `ctest -V` for full log
- Use `ctest -jN` for parallel execution in CI

---

## 6. Quick checklist

- [x] Builds
- [x] `ctest -V` passes
- [x] `anafestica_test.exe` passes directly
- [ ] Add tests covering all variant types
