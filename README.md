# Anafestica
A header-only library for the persistence of application settings in the Windows Registry or other media (JSON, BSON, YAML, XML, INI, etc.)

## Rationale

This library enables easy persistence of application settings for FMX, VCL, and other applications with minimal changes to existing code. For example, you can save the position, size, and state of GUI forms along with other custom attributes by adding just a few lines of code.

The core idea is to provide a hierarchical, heterogeneous container that resembles the Windows Registry but resides in application memory. Consider a Registry key as the base key: this serves as the "root image" of the heterogeneous container. When the application starts, the in-memory container tries to load data from the specified Windows Registry base key. If the Registry key exists, its content (including the base key and all subkeys) is loaded into memory. If the Registry key doesn't exist, the in-memory container is initialized with default attribute values. During application execution, you can read from or write to these keys and their values, but all changes remain confined to the application's memory. When the application terminates, the container's content is written back to the storage medium in its intended format. If the application crashes before writing data back, the data in the storage medium remains unchanged. Note that the container can also use different storage media than the Windows Registry, such as the filesystem or network, using formats like JSON, BSON, YAML, or XML.

The library consists of two main parts: a container part (which is generalized) and a serialization part. From the application's point of view, it provides a consistent interface and enables storing persistent data across different storage media and formats.

Windows applications' persistent data is typically stored in the Registry, following conventions based on the application's nature (whether it's a normal application, service, or other application type). By changing the serialization part via a template parameter (i.e., it's a Policy), you can specify both the data format and storage medium. It is also possible to use several different storage media and formats within the same application. Each serialization format is associated with a specific container, meaning each container is bound to a serializer that can have its own format and storage medium.

The current library version includes serializers for Windows Registry, JSON, BSON, YAML, XML, and INI files.

The BSON backend uses RAD Studio's native `System.JSON.BSON` support and follows the same hierarchical `values` / `nodes` layout as the JSON backend, making it a compact binary alternative when human-edited text files are not required.

The YAML backend uses the external header-only [fkYAML](https://github.com/fktn-k/fkYAML) library and follows the same logical `values` / `nodes` layout as the JSON backend. Anafestica does not redistribute fkYAML: if you include `<anafestica/CfgYAML.h>` or `<anafestica/CfgYAMLSingleton.h>`, register the fkYAML include directory in RAD Studio's include search path for the Clang-based compilers you target (`bcc32c`, `bcc64`, `bcc64x`).

Encrypted file variants are available for the file-generating backends via
`CfgJSONCrypt.h`, `CfgBSONCrypt.h`, `CfgYAMLCrypt.h`, `CfgXMLCrypt.h`, and
`CfgIniFileCrypt.h`. They preserve the normal backend APIs but encrypt the
whole file with Windows CNG AES-GCM. By default the key material is bound to
the current machine and application identity; callers can also pass an explicit
`Anafestica::Crypt::TOptions`.

## Getting Started

The library consists only of header files, making it easy to use and integrate into your codebase without requiring additional compilation steps; you just need to include the necessary header files in your project. It can also be used in contexts other than GUI applications, but its real advantages are evident when developing GUI applications, where it greatly simplifies the management of persistent attributes such as form position, size, and state, as well as application-wide settings.

## Documentation

Try a [Quick Tour](QUICK_TOUR.md) in Anafestica.

For detailed API documentation, see [Library Documentation](LIBRARY_DOCUMENTATION.md).

### Prerequisites / Dependencies

The library selects the variant implementation automatically based on the compiler:

- **bcc64x** (Clang ≥ 15, 64-bit): `std::variant` is used. No preprocessor definitions are needed.
- **bcc64** (Clang < 15, 64-bit) and **bcc32c** (32-bit Clang): `boost::variant` is used, because `std::variant` has an unresolved assignment bug in those toolchains (see [RSP-27418](https://quality.embarcadero.com/browse/RSP-27418)).

The selection is performed inside `CfgNodeValueType.h` via the `__clang_major__` predefined macro; no manual `ANAFESTICA_USE_STD_VARIANT` definition is required.

When `boost::variant` is selected (`bcc64` or `bcc32c`), the Boost headers are required; install Boost through RAD Studio's GetIt package manager or make an existing Boost installation visible in the compiler include path. For **bcc64x** in recent RAD Studio Florence toolchains based on Clang 20, Anafestica uses `std::variant`, so Boost is not required by the library itself.

The YAML backend has one additional opt-in dependency: `CfgYAML.h` includes `<fkYAML/node.hpp>`. To use `Anafestica::YAML::TConfig` or `Anafestica::TConfigYAMLSingleton`, install fkYAML in header-only mode and add its include directory to RAD Studio's include search path for the Clang-based C++ compilers you build with. Projects that do not include the YAML headers do not need fkYAML.

If you build the bundled test projects, note that the current test harness uses **Boost.Test** on all three toolchains, including `bcc64x`. In other words: `bcc64x` does not need Boost for Anafestica's `std::variant` path, but the test executables still depend on Boost.Test.

Please note that only Clang-based compilers are supported by this library (i.e., bcc32c, bcc64, and bcc64x).

### Installing

Installation is not strictly necessary. Anafestica is header-only, so a project can use it as soon as the compiler can find the `anafestica` include directory. For one-off experiments you can add the repository folder directly to the project's C++ include path. For regular use across several projects, it is better to register the include path once in RAD Studio.

The recommended layout is to clone the repository under `$(BDSCOMMONDIR)`, which is normally `%PUBLIC%\Documents\Embarcadero\Studio\XX.X`, where `XX.X` is the RAD Studio version. For example, RAD Studio 13 uses `37.0`:

```bat
cd /d "%PUBLIC%\Documents\Embarcadero\Studio\37.0"
git clone https://github.com/gcardi/Anafestica.git
```

Then open a RAD Studio command prompt for the toolchain you want to update, or run that version's `rsvars.bat`, and execute:

```bat
cd /d "%PUBLIC%\Documents\Embarcadero\Studio\37.0\Anafestica"
register_anafestica.bat
```

`register_anafestica.bat` adds the Anafestica folder to the per-user RAD Studio C++ include-path registry entries under `HKCU\Software\Embarcadero\BDS\XX.X\C++\Paths` for `Win32`, `Win64`, and `Win64x`. It determines `XX.X` from `BDSCOMMONDIR`, so running it after the matching `rsvars.bat` is the clearest way to target the intended RAD Studio installation. Use `register_anafestica.bat --dry-run` to preview the registry changes without writing them.

If you prefer to configure the IDE manually, use **Tools → Options → Language → C++ → Paths and Directories** and add:

```text
$(BDSCOMMONDIR)\Anafestica
```

to the include path for each C++ platform you build with: `Win32` / `bcc32c`, `Win64` / `bcc64`, and `Win64x` / `bcc64x`. No library path is required for Anafestica itself.

If you plan to use the YAML backend, add the fkYAML header directory to the same include-path pages for the Clang-based compiler platforms you use. No library path or linker setting is required because fkYAML is consumed header-only by Anafestica.

You can register fkYAML the same way:

```bat
register_fkYAML.bat
```

By default this looks for the fkYAML repository next to Anafestica under `$(BDSCOMMONDIR)` and registers its `include` directory. Use `register_fkYAML.bat --dry-run` to preview the registry update.

## Building and running tests

See also: [TESTS.md](TESTS.md) for an extended test plan and variant coverage matrix.

The three C++Builder test projects are built and run with MSBuild using the
provided `test_all.bat` script.

The current `bcc64x` suite includes both the full `test_config.cpp` coverage for
all 21 `std::variant` alternatives and the shared 19-type
`test_config_simplified.cpp` module used for parity with the legacy toolchains.
By default, the regression suite exercises the built-in Registry, JSON, BSON,
XML, and INI backends. YAML coverage is opt-in because it depends on the
external fkYAML headers.

1. Run all three compilers (build + test):

```powershell
test_all.bat
```

2. Run a single toolchain (or a subset):

```powershell
test_all.bat bcc64x                 # build + run bcc64x only
test_all.bat bcc32c bcc64           # build + run two toolchains
```

3. Force a full recompile:

```powershell
test_all.bat --rebuild              # clean + rebuild all three from scratch
```

4. Run only existing executables (skip build):

```powershell
test_all.bat --no-build
```

5. Stop at first failure:

```powershell
test_all.bat --stop-on-error
```

6. Show full MSBuild/compiler commands:

```powershell
test_all.bat --verbose-build
```

7. Include optional YAML backend tests:

```powershell
test_all.bat --with-yaml
```

`--with-yaml` defines `ANAFESTICA_TEST_YAML` for the test projects. If fkYAML is
not visible through the active Clang-based compiler include path,
`test_config.cpp` compiles the YAML block out automatically instead of including
`CfgYAML.h`.

The build is incremental by default and uses MSBuild `/t:Make` with
`/v:minimal`. Use `--rebuild` to run `/t:Clean,Build`, and use
`--verbose-build` to switch to `/v:normal` when you want the full compiler
command lines for every toolchain. The script calls `rsvars.bat` to set up the
Embarcadero environment, then builds all selected targets first, and runs the
tests only if all builds succeeded:

| Project | Platform | Executable |
| ------- | -------- | ---------- |
| `Test\TestBcc32c\anafestica_test_bcc32c.cbproj` | Win32 | `Test\TestBcc32c\Win32\Release\anafestica_test_bcc32c.exe` |
| `Test\TestBcc64\anafestica_test_bcc64.cbproj` | Win64 | `Test\TestBcc64\Win64\Release\anafestica_test_bcc64.exe` |
| `Test\TestBcc64x\anafestica_test_bcc64x.cbproj` | Win64x | `Test\TestBcc64x\Win64x\Release\anafestica_test_bcc64x.exe` |

### Cleaning build artifacts

To remove all test build output directories:

```powershell
clear_tests.bat                     # clean all three toolchains
clear_tests.bat bcc64x              # clean bcc64x only
clear_tests.bat bcc32c bcc64        # clean two toolchains
```

This deletes the output directories (`Win32`, `Win64`, `Win64x`) under each
test project folder. It does not require MSBuild or `rsvars.bat`.

The test executables are currently based on **Boost.Test** (`unit_test_framework`) for all three compilers.

