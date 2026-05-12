# Anafestica Library Documentation

## Overview

Anafestica is a header-only C++ library designed for the persistence of application settings in various storage media, including the Windows Registry, JSON files, BSON files, YAML files, XML files, and INI files. It provides a hierarchical, heterogeneous container that mimics the Windows Registry structure but operates in application memory, allowing seamless saving and loading of configuration data.

The library is particularly well-suited for GUI applications (FMX, VCL, and others) where it simplifies the management of persistent attributes such as form positions, sizes, states, and custom application settings. It requires minimal code changes to existing applications and supports multiple storage formats through a policy-based design.

## Key Features

- **Header-only library**: No compilation required, just include the necessary headers
- **Multiple storage backends**: Windows Registry, JSON files, BSON files, YAML files, XML files, INI files
- **Hierarchical data structure**: Tree-like organization similar to Windows Registry
- **Type-safe operations**: Supports various data types including primitives, strings, dates, and collections
- **Singleton pattern support**: Easy access through singleton classes
- **Form persistence helpers**: Specialized classes for VCL and FMX form persistence
- **Cross-platform compatibility**: Works with Embarcadero C++ compilers (bcc32c, bcc64, bcc64x)

## Architecture

The library consists of two main parts:

1. **Container Part**: A generalized hierarchical container (`TConfigNode`) that holds configuration data in memory
2. **Serialization Part**: Policy-based serializers that handle reading from and writing to different storage media

The container uses a tree structure where each node can contain:

- Named values of various types

- Named sub-nodes (child nodes)

The following class diagram illustrates the full architecture, including inheritance hierarchies, composition/aggregation relationships, and the design patterns used throughout the library (notation follows Gamma et al., *Design Patterns*, 1994):

![Anafestica Class Diagram](docs/assets/images/class_diagram.svg)

## Core Classes

### TConfig (Abstract Base Class)

The base class for all configuration implementations. It provides the interface for configuration operations.

#### Public Interface

```cpp
class TConfig {
public:
    TConfig(bool ReadOnly, bool FlushAllItems);
    TConfigNode& GetRootNode();
    void Flush();
    ValueContType CreateValueList(TConfigPath const & Path);
    NodeContType CreateNodeList(TConfigPath const & Path);
    void SaveValueList(TConfigPath const & Path, ValueContType const & Values);
    void DeleteNode(TConfigPath const & Path);
    bool GetReadOnlyFlag() const noexcept;
    bool GetAlwaysFlushNodeFlag() const noexcept;
    bool ShouldFlushOnDestruction() const noexcept;   // see "Flush-on-destruction" below
protected:
    void MarkForFlush() noexcept;                     // used by the migration ctors
};
```

**Constructor Parameters:**

- `ReadOnly`: If true, prevents writing changes back to storage
- `FlushAllItems`: If true, flushes all items regardless of modification status

**Key Methods:**

- `GetRootNode()`: Returns the root configuration node
- `Flush()`: Writes all pending changes to storage
- `CreateValueList()`: Creates a list of values for a given path
- `CreateNodeList()`: Creates a list of sub-nodes for a given path
- `SaveValueList()`: Saves a list of values to a given path
- `DeleteNode()`: Deletes a node at the specified path
- `ShouldFlushOnDestruction()`: Predicate consulted by each backend's
  destructor.  Returns `true` only when the object is writable AND either
  some caller marked it for forced flush (the migration ctors do this) OR
  the in-memory tree has at least one pending `Write` / `Erase` operation.

**Flush-on-destruction semantics (behavioural change):**

Every backend (Registry, JSON, BSON, XML, INIFile, YAML) used to call
`DoFlush()` unconditionally from its destructor whenever the object was
not read-only.  They now consult `ShouldFlushOnDestruction()` instead,
so an unmodified `TConfig` no longer rewrites (or even creates) its
backing file or registry key.  The practical consequences:

- **Fresh install, no modifications** — no file or registry key is
  created.  Earlier versions of the library produced an empty stub
  artifact in this case; applications that relied on that side-effect
  (for example, as a "user has launched the app at least once"
  sentinel) must instead call `Flush()` explicitly or write a value
  before destruction.
- **Existing storage, no modifications** — the backing file is left
  byte-for-byte untouched, and no registry write traffic is generated.
- **Existing storage, with modifications** — flushed as before; see
  the per-attribute note below for what "flushed" means in practice.
- **Migration constructor** — flushes unconditionally because
  `MarkForFlush()` is called after loading from the source file, so the
  destination ends up populated even when the caller never modified
  anything between load and destruction.

**Per-attribute dirty tracking (not changed by this work):**

`ShouldFlushOnDestruction()` is a **whole-object** gate that decides
whether `DoFlush()` runs at all.  Inside `DoFlush()`, every backend's
`DoSaveValueList` has always operated **per attribute**: it writes only
values whose state is `Operation::Write`, deletes only values whose
state is `Operation::Erase`, and leaves `Operation::None` values
untouched on disk.  The file backends additionally re-read the existing
destination as a merge base, so unmodified values pass through verbatim
rather than being rewritten.

The migration constructor is the **only** code path that flips on
`FlushAllItems` to write every attribute regardless of state, and it
does so only because the values came from a different source file and
would otherwise never reach the destination.  Normal single-argument
construction, including the path taken by the `GetConfigSingleton`
helpers when no migration is needed, preserves the per-attribute
behaviour.

### TConfigNode

The main class representing a node in the configuration tree. Each node can contain values and sub-nodes.

#### Public Interface

```cpp
class TConfigNode {
public:
    TConfigNode();
    TConfigNode& GetSubNode(String Id);
    TConfigNode& operator[](String Id);

    // Value operations
    template<typename T>
    void GetItem(String Id, T& Val, Operation Op = Operation::None);

    template<typename T>
    T GetItem(String Id, Operation Op = Operation::None);

    template<typename T>
    bool PutItem(String Id, T&& Val, Operation Op = Operation::Write);

    // String list operations
    void GetItem(String Id, TStrings& Val, Operation Op = Operation::None);
    bool PutItem(String Id, TStrings& Val, Operation Op = Operation::Write);

    // Enumeration methods
    size_t GetNodeCount() const noexcept;
    size_t GetValueCount() const noexcept;

    template<typename OutputIterator>
    void EnumerateNodes(OutputIterator Output) const;

    template<typename OutputIterator>
    void EnumeratePairs(OutputIterator Out) const;

    template<typename OutputIterator>
    void EnumerateValueNames(OutputIterator Out) const;

    // Modification tracking
    bool IsDeleted() const noexcept;
    bool IsModified() const noexcept;

    // Node operations
    void DeleteItem(String Id);
    void DeleteSubNode(String Id);
    void Clear();
    bool ItemExists(String Id) const noexcept;
    bool SubNodeExists(String Id) const noexcept;
};
```

**Key Methods:**

**Node Navigation:**
- `GetSubNode(Id)`: Gets or creates a sub-node with the given ID
- `operator[](Id)`: Same as GetSubNode, allows array-like access

**Value Operations:**
- `GetItem(Id, Val)`: Retrieves a value, storing it in `Val`
- `PutItem(Id, Val)`: Stores a value
- `DeleteItem(Id)`: Marks a value for deletion

**Enumeration:**
- `EnumerateNodes()`: Lists all sub-node names. The `OutputIterator` receives `String` values representing the names of sub-nodes.
- `EnumerateValueNames()`: Lists all value names. The `OutputIterator` receives `String` values representing the names of stored values.
- `EnumeratePairs()`: Lists all name-value pairs. The `OutputIterator` receives `std::pair<String, ValueType>` elements, where `ValueType` is a variant that can hold any supported configuration value type (primitives, strings, dates, collections, etc.).

**Supported Data Types:**
The library supports the following data types through template specialization:

- `int`, `unsigned int`, `long`, `unsigned long`
- `char`, `unsigned char`, `short`, `unsigned short`
- `long long`, `unsigned long long`
- `bool`
- `String` (System::String — wide, UTF-16)
- `TDateTime`
- `float`, `double`, `Currency`
- `StringCont` (std::vector\<String\>)
- `TBytes`, `BytesCont` (std::vector\<Byte\>)
- `std::string` (narrow, treated as UTF-8; type tag `str`)
- `std::wstring` (wide UTF-16; type tag `wstr`)

**Convenience write overloads for string views:**
`std::string_view` and `std::wstring_view` are accepted by `PutItem` as a
write-path convenience. They are materialised to `std::string` / `std::wstring`
before being stored in the variant. Deserialization always returns owning
`std::string` / `std::wstring`, never a view (a view cannot own the data it
points to, so it cannot be safely stored in or returned from persistent storage).

**Special Handling for Enums:**
Enumerated types are automatically handled. If the enum has RTTI information, it's stored as a string; otherwise, as an integer.

### TFileVersionInfo

A utility class for extracting version information from the VERSIONINFO resource of a compiled executable. This class provides access to all standard version information fields that can be embedded in Windows executables through the VERSIONINFO resource.

#### Properties and Constructor

```cpp
class TFileVersionInfo {
public:
    explicit TFileVersionInfo(String FileName);
    
    __property String CompanyName = { read = GetCompanyName };
    __property String ProductName = { read = GetProductName };
    __property String ProductVersion = { read = GetProductVersion };
    __property String FileDescription = { read = GetFileDescription };
    __property String FileVersion = { read = GetFileVersion };
    __property String InternalName = { read = GetInternalName };
    __property String LegalCopyright = { read = GetLegalCopyright };
    __property String OriginalFilename = { read = GetOriginalFilename };
    __property String Comments = { read = GetComments };
};
```

**Constructor Parameters:**
- `FileName`: Path to the executable file containing VERSIONINFO resource

**Available Properties:**
- `CompanyName`: Company name from version info
- `ProductName`: Product name from version info
- `ProductVersion`: Product version string (e.g., "1.0.0.0")
- `FileDescription`: File description
- `FileVersion`: File version string
- `InternalName`: Internal name
- `LegalCopyright`: Copyright information
- `OriginalFilename`: Original filename
- `Comments`: Additional comments

**Example Usage:**

```cpp
#include <anafestica/FileVersionInfo.h>

using Anafestica::TFileVersionInfo;

// Get version info from current executable
String GetModuleFileName() {
    return ::GetModuleName(reinterpret_cast<NativeUInt>(HInstance));
}

void SetupApplicationCaption(TForm* Form) {
    TFileVersionInfo versionInfo{GetModuleFileName()};
    
    Form->Caption = Format(
        _T("%s, Version %s"),
        ARRAYOFCONST((
            versionInfo.ProductName,
            versionInfo.ProductVersion
        ))
    );
}

// Display version information in About dialog
void ShowVersionInfo() {
    TFileVersionInfo info{GetModuleFileName()};
    
    ShowMessage(
        Format(
            _T("Product: %s\nVersion: %s\nCompany: %s\nCopyright: %s"),
            ARRAYOFCONST((
                info.ProductName,
                info.ProductVersion,
                info.CompanyName,
                info.LegalCopyright
            ))
        )
    );
}
```

This class is commonly used to dynamically display version information in application captions, About dialogs, or for configuration purposes (as seen in the singleton classes that use version info to determine registry paths). The VERSIONINFO resource is typically set in your project's version information settings in the IDE.

## Type Encoding Conventions

Every value stored by Anafestica is tagged with one of 21 (or 19 on non-`bcc64x` compilers) C++ types drawn from the `TConfigNodeValueType` variant. Because none of the six backends natively distinguishes all of these types, the library uses a small set of **type tags** that are persisted alongside the data so the correct C++ type can be reconstructed on read.

The same tags are shared by every backend. What differs is **where** the tag is written (in the value's name, in a separate attribute, as a nested JSON key, …) and which types — if any — are allowed to be written without a tag because the storage format itself is unambiguous for them.

This section documents these conventions so that you can read, hand-edit, or hand-author persisted configurations without reverse-engineering them from the code.

### Shared Type Tags

The canonical definitions live in `anafestica/CfgNodeValueType.h` (macros `ANA_TT_*` and enum `TypeTag`). The table below lists each C++ type, its tag spelling as it appears on disk / in the registry, and notes on encoding.

| C++ type                    | Tag    | Notes                                                                    |
| --------------------------- | ------ | ------------------------------------------------------------------------ |
| `int`                       | `i`    | Decimal integer                                                          |
| `unsigned int`              | `u`    | Decimal integer                                                          |
| `long`                      | `l`    | Decimal integer                                                          |
| `unsigned long`             | `ul`   | Decimal integer                                                          |
| `char`                      | `c`    | Decimal integer (not a character glyph)                                  |
| `unsigned char`             | `uc`   | Decimal integer                                                          |
| `short`                     | `s`    | Decimal integer                                                          |
| `unsigned short`            | `us`   | Decimal integer                                                          |
| `long long`                 | `ll`   | Decimal integer                                                          |
| `unsigned long long`        | `ull`  | Decimal integer (stringified in text backends to preserve precision)     |
| `bool`                      | `b`    | Stored as `1` / `0` (INI, JSON number) or `True` / `False` (XML text)    |
| `System::String`            | `sz`   | UTF-16 string, stored as UTF-8 on disk                                   |
| `System::TDateTime`         | `dt`   | ISO-8601 text (`DateToISO8601(..., false)`)                              |
| `float`                     | `flt`  | Decimal with `.` separator, 9 significant digits                         |
| `double`                    | `dbl`  | Decimal with `.` separator, 17 significant digits                        |
| `System::Currency`          | `cur`  | Decimal with `.` separator (`CurrToStr`)                                 |
| `StringCont` (`vec<String>`)| `sv`   | Multi-string; backend-specific serialization (see each section)          |
| `System::Sysutils::TBytes`  | `dab`  | Base-64 in text backends; native `REG_BINARY` in the registry            |
| `BytesCont` (`vec<Byte>`)   | `vb`   | Base-64 in text backends; native `REG_BINARY` in the registry            |
| `std::string` (bcc64x only) | `str`  | UTF-8                                                                    |
| `std::wstring` (bcc64x only)| `wstr` | UTF-16, stored as UTF-8 on disk                                          |

The `std::string` / `std::wstring` alternatives exist only when the `std::variant` path is active (bcc64x / Clang ≥ 15). On `bcc32c` / `bcc64` those tags are neither written nor parsed.

## Concrete Implementations

### Registry::TConfig

Implements configuration storage in the Windows Registry.

```cpp
namespace Registry {
class TConfig : public Anafestica::TConfig {
public:
    TConfig(HKEY RootKey, String KeyPath, bool ReadOnly = false, bool FlushAllItems = false);
};
}
```

**Constructor Parameters:**
- `RootKey`: Registry root key (e.g., HKEY_CURRENT_USER)
- `KeyPath`: Path within the registry
- `ReadOnly`, `FlushAllItems`: Same as base class

**Type encoding:**
The registry backend uses native Windows Registry value types (`REG_DWORD`, `REG_QWORD`, `REG_SZ`, `REG_MULTI_SZ`, `REG_BINARY`) as the *first* layer of typing. Because several C++ types collapse onto the same `REG_*` kind (for example, `int`, `unsigned int`, `long`, `unsigned long`, `short`, `unsigned short`, `char`, `unsigned char`, and `bool` all serialize as `REG_DWORD`), a second layer is needed to tell them apart on read. That second layer is a **tag suffix** appended to the value *name*, written in the form:

```text
ValueName:(TypeTag)
```

Note the single colon (the INI backend uses a double colon; see below). When the suffix is **absent**, the reader treats the value as the "canonical" C++ type for that `REG_*` kind (see table below).

| `REG_*` kind  | Canonical C++ type (no tag) | Tagged alternatives                                                         |
| ------------- | --------------------------- | --------------------------------------------------------------------------- |
| `REG_DWORD`   | `int`                       | `:(u)`, `:(l)`, `:(ul)`, `:(c)`, `:(uc)`, `:(s)`, `:(us)`, `:(b)`           |
| `REG_QWORD`   | `long long`                 | `:(ull)`                                                                    |
| `REG_SZ`      | `System::String`            | `:(str)` (UTF-8, bcc64x only), `:(wstr)` (UTF-16, bcc64x only)              |
| `REG_BINARY`  | `TBytes`                    | `:(dt)` (TDateTime), `:(flt)`, `:(dbl)`, `:(cur)`, `:(vb)` (BytesCont)      |
| `REG_MULTI_SZ`| `StringCont`                | — (no tagged alternatives)                                                  |

So, for example:

- A value named `Port` written as `REG_DWORD = 5432` round-trips as `int`.
- A value named `Port:(u)` written as `REG_DWORD = 5432` round-trips as `unsigned int`.
- A value named `LastRun:(dt)` written as `REG_BINARY` is read back as `TDateTime` via `TRegistry::ReadDateTime`.
- A value named `Enabled:(b)` written as `REG_DWORD = 1` round-trips as `bool`.

Binary values for `TDateTime`, `float`, `double`, and `Currency` are stored in `TRegistry`'s native binary layout (`WriteDateTime`, `WriteFloat`, `WriteCurrency`) — they are *not* text-encoded, so manually composing them requires producing the exact byte layout that those APIs expect.

When you edit the registry by hand, adding or removing the `:(TypeTag)` suffix is what controls which C++ alternative the library will load.

### JSON::TConfig

Implements configuration storage in JSON files.

```cpp
namespace JSON {
class TConfig : public Anafestica::TConfig {
public:
    // Standard ctor: load from FileName, persist to FileName.
    TConfig(String FileName, bool ReadOnly = false, bool Compact = true,
            bool FlushAllItems = false, bool ExplicitTypes = false);

    // Migration ctor: load from LoadFileName, persist to SaveFileName.
    // When SaveFileName already exists, the destination wins (the load
    // reads from SaveFileName, ignoring LoadFileName).  FlushAllItems is
    // forced true internally so loaded values reach the destination.
    TConfig(String LoadFileName, String SaveFileName,
            bool ReadOnly = false, bool Compact = true,
            bool ExplicitTypes = false);

    // Named-constructor wrapper around the migration ctor — preferred at
    // call sites for readability.
    static TConfig Migrate(String LoadFileName, String SaveFileName,
                           bool ReadOnly = false, bool Compact = true,
                           bool ExplicitTypes = false);
};
}
```

**Constructor Parameters:**
- `FileName`: Path to the JSON file
- `Compact`: If true, outputs compact JSON; if false, pretty-printed
- `ExplicitTypes`: If true, every value is tagged (see below)
- `ReadOnly`, `FlushAllItems`: Same as base class
- `LoadFileName` / `SaveFileName`: For the migration ctor — source and
  destination, respectively.  See "Configuration migration" below.

**Storage layout:**
Nested `TConfigPath` components become nested JSON objects. Each group of values lives under a `values` child object, and each value is **either** a plain key/value pair **or** a one-entry object that carries the type tag as its inner key:

```json
{
  "node": {
    "values": {
      "Port":     5432,
      "Count":    { "u":   42 },
      "LastRun":  { "dt":  "2026-04-15T12:00:00.000" },
      "Payload":  { "dab": "SGVsbG8=" }
    }
  }
}
```

**Type encoding:**
The JSON backend, like the registry backend, recognizes a handful of "canonical" C++ types whose values can be written **bare** (without any wrapping object) because the JSON type is already unambiguous. The remaining variant alternatives are always wrapped as `{ "TypeTag": value }`.

| C++ type          | Bare form          | Tagged form (always acceptable on read) |
| ----------------- | ------------------ | --------------------------------------- |
| `int`             | `"Name": 42`       | `"Name": { "i": 42 }`                   |
| `bool`            | `"Name": true`     | `"Name": { "b": true }`                 |
| `System::String`  | `"Name": "hello"`  | `"Name": { "sz": "hello" }`             |

The constructor's `ExplicitTypes` flag controls **writing** only: when `true`, even `int`, `bool`, and `String` are written in the tagged form. When `false` (the default), they are written bare. Reading accepts either form regardless of the flag.

All other types are *always* written tagged. In particular:

- Integer types other than `int` are stored as JSON numbers (`{ "u": 42 }`, `{ "l": 42 }`, …).
- `unsigned long long` is stored as a JSON **string** (`{ "ull": "18446744073709551615" }`) to avoid the JSON-number precision cliff at 2⁵³.
- `TDateTime` is a JSON string in ISO-8601 form: `{ "dt": "2026-04-15T12:00:00.000" }`.
- `float`, `double` are JSON numbers; `Currency` is a JSON string using `.` as the decimal separator.
- `StringCont` is a JSON array of strings wrapped with the `sv` tag: `{ "sv": ["one","two"] }`.
- `TBytes` / `BytesCont` are Base-64 JSON strings: `{ "dab": "SGVsbG8=" }`, `{ "vb": "…" }`.
- On `bcc64x`, `std::string` uses `str` (UTF-8 string) and `std::wstring` uses `wstr` (UTF-16 string, transcoded to UTF-8 on disk).

When editing a JSON file by hand, you can freely switch between the bare and tagged forms for the three canonical types. For every other alternative you must keep (or add) the wrapping `{ "TypeTag": value }` object — otherwise the reader will fall back to the canonical bare-form interpretation for the matching JSON type.

### BSON::TConfig

Implements configuration storage in BSON files using RAD Studio's native `System.JSON.BSON` reader/writer support.

```cpp
namespace BSON {
class TConfig : public Anafestica::TConfig {
public:
    TConfig(String FileName, bool ReadOnly = false,
            bool FlushAllItems = false, bool ExplicitTypes = false);
};
}
```

**Constructor Parameters:**
- `FileName`: Path to the BSON file
- `ExplicitTypes`: If true, every value is tagged (see below)
- `ReadOnly`, `FlushAllItems`: Same as base class

**Storage layout:**
The BSON backend uses the same logical document shape as the JSON backend: nested `TConfigPath` components become nested objects, values live under a `values` object, and child nodes live under a `nodes` object.

In other words, this JSON:

```json
{
  "node": {
    "values": {
      "Port":     5432,
      "Count":    { "u":   42 },
      "LastRun":  { "dt":  "2026-04-15T12:00:00.000" },
      "Payload":  { "dab": "SGVsbG8=" }
    }
  }
}
```

is represented in BSON with the same object/key structure, just serialized as BSON instead of text JSON.

**Type encoding:**
`BSON::TConfig` intentionally follows the same conventions as `JSON::TConfig`.

| C++ type          | Bare form          | Tagged form (always acceptable on read) |
| ----------------- | ------------------ | --------------------------------------- |
| `int`             | `"Name": 42`       | `"Name": { "i": 42 }`                   |
| `bool`            | `"Name": true`     | `"Name": { "b": true }`                 |
| `System::String`  | `"Name": "hello"`  | `"Name": { "sz": "hello" }`             |

The constructor's `ExplicitTypes` flag has the same meaning as in the JSON backend: when `true`, even `int`, `bool`, and `String` are emitted in tagged form. Reading accepts either form regardless of the flag.

All other types are always written tagged, with the same tag names and payload conventions as JSON:

- Integer types other than `int` are stored with explicit tags.
- `unsigned long long` is stored as a tagged string to preserve precision.
- `TDateTime` is stored as an ISO-8601 string under the `dt` tag.
- `float`, `double`, and `Currency` follow the same numeric/string rules as JSON.
- `StringCont` is stored as a tagged array of strings.
- `TBytes` / `BytesCont` are stored as Base-64 strings under `dab` / `vb`.
- On `bcc64x`, `std::string` uses `str` and `std::wstring` uses `wstr`.

The practical difference from JSON is therefore the transport format: BSON is binary and compact, while preserving the same Anafestica-facing data model and roundtrip semantics.

### YAML::TConfig

Implements configuration storage in YAML files using the external header-only fkYAML library.

```cpp
namespace YAML {
class TConfig : public Anafestica::TConfig {
public:
    TConfig(String FileName, bool ReadOnly = false,
            bool FlushAllItems = false, bool ExplicitTypes = false);
};
}
```

**Constructor Parameters:**
- `FileName`: Path to the YAML file
- `ExplicitTypes`: If true, every value is tagged (see below)
- `ReadOnly`: Same as base class
- `FlushAllItems`: Kept for API parity, but YAML always rebuilds the whole document on flush

**Dependency:**
`CfgYAML.h` includes `<fkYAML/node.hpp>`. Anafestica does not redistribute fkYAML, so projects that include `<anafestica/CfgYAML.h>` or `<anafestica/CfgYAMLSingleton.h>` must install fkYAML in header-only mode and register the fkYAML include directory in RAD Studio's include search path for the Clang-based compiler platforms they target (`bcc32c`, `bcc64`, `bcc64x`). Projects that do not include the YAML headers do not need fkYAML. The repository includes `register_fkYAML.bat` to update those per-user RAD Studio registry entries automatically.

**Storage layout:**
The YAML backend uses the same logical document shape as the JSON backend: nested `TConfigPath` components become nested mappings, values live under a `values` mapping, and child nodes live under a `nodes` mapping.

```yaml
nodes:
  Database:
    values:
      Port: 5432
      Count:
        u: 42
      LastRun:
        dt: 2026-04-15T12:00:00.000
      Payload:
        dab: SGVsbG8=
```

**Type encoding:**
`YAML::TConfig` follows the JSON/BSON conventions: `int`, `bool`, and `System::String` may be written in bare form by default, or as tagged single-entry mappings when `ExplicitTypes` is true. Reading accepts either form.

All other types are always written tagged, using the same tag names as the other backends. `unsigned long long`, `float`, `double`, and `Currency` are string-encoded under their tags to preserve precision and locale-independent formatting. `StringCont` is a YAML sequence, `TBytes` / `BytesCont` are Base-64 strings, and on `bcc64x`, `std::string` / `std::wstring` use the `str` / `wstr` tags.

Because fkYAML does not expose an erase operation on its node API, the backend rebuilds the YAML document from the in-memory `TConfigNode` tree on every flush. Deleted values and nodes are therefore removed by omission from the rebuilt document.

### XML::TConfig

Implements configuration storage in XML files.

```cpp
namespace XML {
class TConfig : public Anafestica::TConfig {
public:
    TConfig(String FileName, bool ReadOnly = false, bool FlushAllItems = false);
};
}
```

**Constructor Parameters:**
- `FileName`: Path to the XML file
- `ReadOnly`, `FlushAllItems`: Same as base class

**Storage layout:**
The XML backend builds a tree of `<node name="…">` elements mirroring the `TConfigPath` hierarchy. Each group of values lives under a `<values>` child, and each value is emitted as a `<value>` element whose `name` attribute carries the key and whose `type` attribute carries the tag:

```xml
<node name="Database">
  <values>
    <value name="Port"    type="i">5432</value>
    <value name="Host"    type="sz">localhost</value>
    <value name="LastRun" type="dt">2026-04-15T12:00:00.000</value>
    <value name="Items"   type="sv">one&#10;two&#10;three&#10;</value>
    <value name="Blob"    type="dab">SGVsbG8=</value>
  </values>
</node>
```

DTD declarations are rejected before parsing. Documents containing `<!DOCTYPE` or `<!ENTITY` are refused rather than handed to the XML parser.

**Type encoding:**
Every `<value>` element **must** carry a `type` attribute; there is no "bare" shorthand. The attribute value is exactly one of the tag strings from the [Shared Type Tags](#shared-type-tags) table (e.g. `i`, `u`, `sz`, `dt`, `flt`, `dbl`, `cur`, `sv`, `dab`, `vb`, `str`, `wstr`, …). A `<value>` element without a recognized `type` attribute is silently ignored on read.

The element's text content is the value, using these conventions:

- Integer, floating-point, and `Currency` text follows the conventions in the shared table; `unsigned long long` is emitted as a plain decimal string rather than a bounded number.
- `bool` text is `True` / `False` (produced by `BoolToStr(Val, true)`).
- `TDateTime` uses ISO-8601 (`DateToISO8601`).
- `StringCont` items are joined with `\n` line terminators (one item per line) in the text node.
- `TBytes` and `BytesCont` are Base-64 encoded text; an empty collection is stored as an empty text node.
- `std::string` / `std::wstring` (bcc64x only) carry `type="str"` / `type="wstr"` and are stored as plain text.

When hand-editing, you must keep the `type` attribute in sync with the text content. Changing only the text while leaving, say, `type="i"` in place will cause the reader to `std::stoi` the new content.

### INIFile::TConfig

Implements configuration storage in classic Windows INI files using `TMemIniFile`.

```cpp
namespace INIFile {
class TConfig : public Anafestica::TConfig {
public:
    TConfig(String FileName, bool ReadOnly = false, bool FlushAllItems = false);
};
}
```

**Constructor Parameters:**
- `FileName`: Path to the INI file (UTF-8 encoding)
- `ReadOnly`, `FlushAllItems`: Same as base class

**Storage layout:**
The INI file is structured around sections and key/value pairs. The root node maps to the section `[config]`; nested paths are flattened into section names using a backslash separator, e.g. `[config\Database\Primary]`. The file itself is written in UTF-8 via `TEncoding::UTF8`.

```ini
[config]
port::(i)=5432
host::(sz)=localhost
debug::(b)=1

[config\Database\Primary]
last_run::(dt)=2026-04-15T12:00:00.000
items::(sv)=one|two|three
```

Because backslash is the hierarchy separator, INI node names must not contain `\` or `/`. Those characters are rejected when a `TConfigPath` is converted to a section name.

**Type encoding:**
Because INI has no notion of value types (every value is plain text), the INI backend is the **only** backend in which *every* value must carry its tag — there is no canonical / bare form. The tag is appended to the key using the double-colon convention:

```text
Name::(TypeTag)=Value
```

Note the **double** colon and the parentheses around the tag — this is what distinguishes the INI convention from the registry's single-colon `Name:(TypeTag)` form. Keys without a valid `::(TypeTag)` suffix are silently ignored on read. The `Name` part may contain any character except a trailing `::(...)` sequence; the decoder scans from the right to find the last `::(` that closes at the end of the line with `)`.

Per-type text encoding:

- Integer, floating-point, and `Currency` text follows the conventions in the shared table; `unsigned long long` is emitted as a plain decimal string.
- `bool` is stored as `1` / `0` (not `True` / `False`).
- `TDateTime` uses ISO-8601.
- `StringCont` items are joined with `|`, with `\` → `\\` and `|` → `\|` backslash-escaping applied to each item. An empty string and a single-item `StringCont{""}` both round-trip to `StringCont{}`.
- `TBytes` and `BytesCont` are Base-64 encoded (same scheme as the XML and JSON backends).
- `std::string` (tag `str`, bcc64x only) is stored as UTF-8; `std::wstring` (tag `wstr`, bcc64x only) is UTF-16 transcoded to UTF-8 in the file.

When hand-editing, changing the tag is how you change the C++ type the loader will produce: for instance, rewriting `port::(i)=5432` as `port::(u)=5432` switches the variant alternative from `int` to `unsigned int` without touching the numeric text.

## Singleton Classes

For convenience, the library provides singleton classes that automatically determine the registry path from the application's version information.

### TConfigRegistrySingleton

```cpp
class TConfigRegistrySingleton {
public:
    static Anafestica::TConfig& GetConfig();
};
```

This singleton creates a registry-based configuration using the path: `HKCU\Software\CompanyName\ProductName\ProductVersion`

The CompanyName, ProductName, and ProductVersion are read from the application's version info resource.

### TConfigJSONSingleton

```cpp
class TConfigJSONSingleton {
public:
    static Anafestica::TConfig& GetConfig();
};
```

This singleton creates a JSON-based configuration. The file path is derived from the application's version info: `$(HOME)\CompanyName\ProductName\ProductVersion\AppName.json`.

### TConfigBSONSingleton

```cpp
class TConfigBSONSingleton {
public:
    static Anafestica::TConfig& GetConfig();
};
```

This singleton creates a BSON-based configuration. The file path is derived from the application's version info: `$(HOME)\CompanyName\ProductName\ProductVersion\AppName.bson`.

### TConfigYAMLSingleton

```cpp
class TConfigYAMLSingleton {
public:
    static Anafestica::TConfig& GetConfig();
};
```

This singleton creates a YAML-based configuration. The file path is derived from the application's version info: `$(HOME)\CompanyName\ProductName\ProductVersion\AppName.yaml`.

Include `<anafestica/CfgYAMLSingleton.h>` to use this singleton. The fkYAML header directory must be registered in RAD Studio's include search path for the Clang-based compiler platforms that build this header.

### TConfigXMLSingleton

```cpp
class TConfigXMLSingleton {
public:
    static Anafestica::TConfig& GetConfig();
};
```

This singleton creates an XML-based configuration. The file path is derived from the application's version info: `$(HOME)\CompanyName\ProductName\ProductVersion\AppName.xml`.

### TConfigINIFileSingleton

```cpp
class TConfigINIFileSingleton {
public:
    static Anafestica::TConfig& GetConfig();
};
```

This singleton creates an INI-file-based configuration. The file path is derived from the application's version info: `$(HOME)\CompanyName\ProductName\ProductVersion\AppName.ini`.

Include `<anafestica/CfgIniFileSingleton.h>` to use this singleton.

## Configuration migration

When an application's `ProductVersion` changes, the per-version path of the
file-based singletons changes too — so the new build starts with an empty
configuration unless the data is migrated forward.  The migration helpers
do this automatically.

### TVersion (`anafestica/Version.h`)

Strict version-string parser used by the discovery helpers.

```cpp
class TVersion {
public:
    TVersion() = default;                 // produces 0.0.0.0
    explicit TVersion(String Text);       // throws Exception on malformed input

    unsigned long long Major()    const noexcept;
    unsigned long long Minor()    const noexcept;
    String             MinorSuffix() const;
    unsigned long long Build()    const noexcept;
    unsigned long long Revision() const noexcept;

    bool operator< (TVersion const & Other) const;
    bool operator==(TVersion const & Other) const;
    // !=, >, <=, >= follow
};
```

Grammar:  `^\d+(?:\.\d+[a-zA-Z]*)(?:\.\d+){0,2}$`

- First component: digits only.
- Second component: digits plus an optional letter suffix
  (case-insensitive — lowercased at parse time).
- Up to two additional digits-only components (build, revision).

Accepted: `1.2`, `1.3a`, `2.0beta`, `1.0.2.3`, `1.10a.0.0`.
Rejected: `1` (too few components), `1a.3` (letter on major),
`1.0.0a` (letter on build/revision), `1.2.3.4.5` (too many), `1.0RC1`
(digits after letters inside one component).  Malformed input throws a
Delphi-style `Exception` whose message contains the offending string.

Ordering is lexicographic over (major, minor, suffix, build, revision)
with absent trailing components compared as zero — so `1.0.2.3 < 1.1`,
`1.1 < 1.1a`, and `1.1 == 1.1.0.0`.

### Migration constructor and `Migrate` factory

Every file-based backend (`JSON`, `BSON`, `XML`, `INIFile`, `YAML`)
exposes a migration constructor and a named `Migrate` factory:

```cpp
// One ctor variant per backend — parameters after the two file names
// vary slightly (Compact / ExplicitTypes / etc.).  See each backend's
// header for the precise signature.
namespace JSON {
class TConfig : public Anafestica::TConfig {
public:
    TConfig(String LoadFileName, String SaveFileName,
            bool ReadOnly = false, bool Compact = true,
            bool ExplicitTypes = false);

    static TConfig Migrate(String LoadFileName, String SaveFileName,
                           bool ReadOnly = false, bool Compact = true,
                           bool ExplicitTypes = false);
};
}
```

Semantics:

- Loads from `LoadFileName` on construction; persists to `SaveFileName`
  on destruction.
- If `SaveFileName` already exists, the **destination wins** — the load
  reads from `SaveFileName` and ignores `LoadFileName`.  This protects
  data already written for the current version.
- If `LoadFileName` does not exist either, the object behaves like a
  fresh-install single-arg ctor (defaults; no flush unless modified).
- `FlushAllItems` is forced `true` internally so that values loaded from
  the source (state `Operation::None`) are still written out to the
  destination when the destructor runs.
- After a successful load from the source, the migration ctor calls
  `MarkForFlush()` on the base class so that `ShouldFlushOnDestruction()`
  returns `true` even though nothing has been modified — ensuring the
  destination file gets populated.

> **Note on `FlushAllItems = true` in the migration ctor.**  The
> "always-write-everything" behaviour is **scoped to the migration
> constructor**, not the library at large.  All single-arg constructors
> (and the `GetConfigSingleton` helpers when no migration is needed)
> keep the default `FlushAllItems = false`, which means every backend's
> `DoSaveValueList` writes only items in state `Operation::Write` and
> deletes only items in state `Operation::Erase`; items in state
> `Operation::None` are untouched on disk.  The file backends layer a
> re-read of the existing destination on top of that, so a
> partial-modification flush ends up as "modified values overwritten,
> the rest passed through verbatim from the previous file."  The
> migration ctor cannot use that mode because the values came from a
> *different* file (the source) and would otherwise never reach the
> destination.

Prefer the named `Migrate` factory at call sites — it makes the load/save
direction unambiguous when both arguments are `String`.

**Example 1 — explicit source path.**  Use this when the application
already knows where the previous version's data lives (for example,
because it computed the path from a known schema or read it from a
command-line argument):

```cpp
#include <anafestica/CfgJSON.h>

void RunWithMigratedConfig()
{
    String const Source = _D( "C:\\Users\\Me\\AppData\\Roaming\\Acme\\MyApp\\1.5\\MyApp.json" );
    String const Dest   = _D( "C:\\Users\\Me\\AppData\\Roaming\\Acme\\MyApp\\2.0\\MyApp.json" );

    // Load 1.5's settings, persist to 2.0 on scope exit.  If Dest already
    // exists (e.g. the app was launched once already at 2.0), the load
    // silently reads from Dest and ignores Source.  If neither exists,
    // the object starts from defaults and the dtor does not create a file.
    auto Cfg = Anafestica::JSON::TConfig::Migrate( Source, Dest );

    // From here on, Cfg behaves like any other TConfig — read or modify
    // values, and the dtor flushes to Dest.
    Cfg.GetRootNode().PutItem( _D( "migrated_at" ), Now() );
}
```

**Example 2 — discovery + migration.**  When the previous version is not
known up front, combine the `FindPriorVersionFile` helper with `Migrate`.
This is what the file-based singleton helpers do internally; you only
need to write this code by hand if you are constructing `TConfig`
objects directly (without the singletons), or if you want to apply a
custom migration policy:

```cpp
#include <anafestica/CfgJSON.h>
#include <anafestica/CfgJSONSingleton.h>   // for Anafestica::JSON::GetFileName
#include <anafestica/Migration.h>

Anafestica::JSON::TConfig OpenAppConfigWithMigration( String ExePath = ParamStr( {} ) )
{
    auto const Dest = Anafestica::JSON::GetFileName( ExePath );

    if ( !TFile::Exists( Dest ) ) {
        // Look for the most recent strictly older version under
        // $HOME\<CompanyName>\<ProductName>\.
        if ( auto const Source =
                Anafestica::Migration::FindPriorVersionFile(
                    ExePath, _D( ".json" )
                ) )
        {
            return Anafestica::JSON::TConfig::Migrate( *Source, Dest );
        }
    }

    // Either Dest already exists, or no prior version was found — open
    // normally.  In the no-prior-version case the dtor will not create
    // a file unless something is modified.
    return Anafestica::JSON::TConfig( Dest );
}
```

The two `return` statements work even though `TConfig`'s copy/move are
deleted: in C++17 the prvalue produced by each `return` initialises the
caller's storage directly (mandatory copy elision).

For most applications, the **simpler path is to use the singleton
helper** — `Anafestica::JSON::GetConfigSingleton()` already performs
the discovery and migration internally, so no migration-specific code
is needed:

```cpp
#include <anafestica/CfgJSONSingleton.h>

void UseAppConfig()
{
    auto& Cfg = Anafestica::JSON::GetConfigSingleton();
    // First call after a version bump: loaded from the prior version's
    // file (if any), then written to the current-version path on
    // application exit.  No explicit Migrate call needed.
    auto const Lang = Cfg.GetRootNode().GetItem<String>( _D( "language" ) );
}
```

### Discovery: `FindPriorVersionFile`

`anafestica/Migration.h` provides two helpers that turn the file-version
info on disk into a candidate source path.

```cpp
namespace Anafestica { namespace Migration {

// Production entry point: reads CompanyName / ProductName /
// ProductVersion from FileName's VERSIONINFO and enumerates sibling
// version directories under $HOME\<Co>\<Prod>\.  Extension must include
// the leading dot.
std::optional<String>
FindPriorVersionFile(String FileName, String Extension);

// Lower-level variant — useful for tests and for applications that want
// to use a non-default product-root directory.
std::optional<String>
FindPriorVersionFileUnder(String ProductRoot, TVersion Current,
                          String LeafFileName, String Extension);

}}
```

For each sibling directory whose leaf name parses as `TVersion`, the
helper picks the largest strictly less than `Current`.  Subdirectories
that don't parse as versions (e.g. `backup`, `tmp`) are silently
skipped.  Returns `std::nullopt` when the product root is missing, when
no strictly older version is found, or when the matched sibling does not
contain a file with the requested extension.

### Automatic wiring in the file-based singletons

The `GetConfigSingleton` helper of every file backend calls
`FindPriorVersionFile` transparently when the per-version destination is
missing, so applications using the singletons get migration for free —
no code changes needed:

```cpp
// CfgJSONSingleton.h — equivalent of:
inline Anafestica::TConfig& GetConfigSingleton(String FileName = ParamStr({})) {
    static auto Cfg = [FileName]() -> TConfig {
        auto const Dest = GetFileName(FileName);
        if (!TFile::Exists(Dest)) {
            if (auto const Source =
                    Anafestica::Migration::FindPriorVersionFile(FileName, _D(".json")))
            {
                return TConfig::Migrate(*Source, Dest);
            }
        }
        return TConfig(Dest);
    }();
    return Cfg;
}
```

The same wiring applies in `CfgBSONSingleton.h`, `CfgXMLSingleton.h`,
`CfgIniFileSingleton.h`, and `CfgYAMLSingleton.h` with the appropriate
file extension.

## Form Persistence Classes

### TPersistFormVCL

A VCL form class that automatically persists form position, size, and state.

```cpp
template<typename CfgSingleton>
class TPersistFormVCL : public Vcl::Forms::TForm {
public:
    enum class StoreOpts {
        None, OnlySize, OnlyPos, PosAndSize,
        OnlyState, StateAndSize, StateAndPos, All
    };

    TPersistFormVCL(TComponent* Owner, StoreOpts StoreOptions = StoreOpts::All,
                   TConfigNode* RootNode = nullptr);
    TConfigNode& GetConfigNode() const;
    static TConfigNode& GetConfigRootNode();
    void ReadValues();
    void SaveValues();
};
```

**StoreOpts Enumeration:**
- `None`: No persistence
- `OnlySize`: Persist width and height
- `OnlyPos`: Persist left and top position
- `PosAndSize`: Persist position and size
- `OnlyState`: Persist window state (minimized/maximized)
- `StateAndSize`: Persist state and size
- `StateAndPos`: Persist state and position
- `All`: Persist everything

### TPersistFormFMX

Similar to TPersistFormVCL but for FireMonkey (FMX) forms.

```cpp
template<typename CfgSingleton>
class TPersistFormFMX : public Fmx::Forms::TForm {
    // Similar interface to TPersistFormVCL
};
```

## Persistence Macros

The library provides convenience macros for persisting properties and values. These macros are defined in `CfgItems.h` (general-purpose) and `PersistFormVCL.h`/`PersistFormFMX.h` (form-specific).

### RESTORE_LOCAL_PROPERTY(PROPERTY)

```cpp
#define RESTORE_LOCAL_PROPERTY( PROPERTY ) \
{ \
    std::remove_reference_t< decltype( PROPERTY )> Tmp{ PROPERTY }; \
    GetConfigNode().GetItem( #PROPERTY, Tmp ); \
    PROPERTY = Tmp; \
}
```

**Description:**  
Restores a property value from the configuration storage. The macro creates a temporary variable of the same type as the property, reads the value from the configuration using the property name as the key, and assigns it back to the property. This provides safe restoration with default value fallback.

**Parameters:**
- `PROPERTY`: The property variable to restore

**Example:**
```cpp
// In a form class that inherits from TPersistFormVCL
RESTORE_LOCAL_PROPERTY(FontSize);  // Restores FontSize from config
```

### SAVE_LOCAL_PROPERTY(PROPERTY)

```cpp
#define SAVE_LOCAL_PROPERTY( PROPERTY ) \
    GetConfigNode().PutItem( #PROPERTY, PROPERTY )
```

**Description:**  
Saves a property value to the configuration storage. The macro uses the property name as the configuration key and stores the current property value.

**Parameters:**
- `PROPERTY`: The property variable to save

**Example:**
```cpp
// In a form class that inherits from TPersistFormVCL
SAVE_LOCAL_PROPERTY(FontSize);  // Saves FontSize to config
```

**Note:** These macros automatically use the property name as the configuration key, making the code more readable and less error-prone than manually specifying string keys.

### RESTORE_PROPERTY(NODE, PROPERTY)

```cpp
#define RESTORE_PROPERTY( NODE, PROPERTY ) \
{ \
    std::remove_reference_t< decltype( PROPERTY )> Tmp{ PROPERTY }; \
    ( NODE ).GetItem( #PROPERTY, Tmp ); \
    PROPERTY = Tmp; \
}
```

**Description:**  
Restores a property value from a specified configuration node. The macro creates a temporary variable of the same type as the property, reads the value from the configuration node using the property name as the key, and assigns it back to the property. This provides safe restoration with default value fallback.

**Parameters:**
- `NODE`: The TConfigNode reference to read from
- `PROPERTY`: The property variable to restore

**Example:**
```cpp
auto& configNode = config.GetRootNode();
int fontSize = 12;  // Default value
RESTORE_PROPERTY(configNode, fontSize);  // Restores fontSize from config
```

### SAVE_PROPERTY(NODE, PROPERTY)

```cpp
#define SAVE_PROPERTY( NODE, PROPERTY ) \
    ( NODE ).PutItem( #PROPERTY, PROPERTY )
```

**Description:**  
Saves a property value to a specified configuration node. The macro uses the property name as the configuration key and stores the current property value.

**Parameters:**
- `NODE`: The TConfigNode reference to write to
- `PROPERTY`: The property variable to save

**Example:**
```cpp
auto& configNode = config.GetRootNode();
int fontSize = 14;
SAVE_PROPERTY(configNode, fontSize);  // Saves fontSize to config
```

### RESTORE_ID_PROPERTY(NODE, ID, PROPERTY)

```cpp
#define RESTORE_ID_PROPERTY( NODE, ID, PROPERTY ) \
{ \
    std::remove_reference_t< decltype( PROPERTY )> Tmp{ PROPERTY }; \
    ( NODE ).GetItem( #ID, Tmp ); \
    PROPERTY = Tmp; \
}
```

**Description:**  
Restores a property value from a specified configuration node using a custom identifier. Unlike `RESTORE_PROPERTY`, this macro allows you to specify a custom key name instead of using the property name. The macro creates a temporary variable of the same type as the property, reads the value from the configuration node using the specified ID as the key, and assigns it back to the property.

**Parameters:**
- `NODE`: The TConfigNode reference to read from
- `ID`: The custom identifier/key to use for storage
- `PROPERTY`: The property variable to restore

**Example:**
```cpp
auto& configNode = config.GetRootNode();
int fontSize = 12;  // Default value
RESTORE_ID_PROPERTY(configNode, "FontSize", fontSize);  // Restores using custom key
```

### SAVE_ID_PROPERTY(NODE, ID, PROPERTY)

```cpp
#define SAVE_ID_PROPERTY( NODE, ID, PROPERTY ) \
    ( NODE ).PutItem( #ID, PROPERTY )
```

**Description:**  
Saves a property value to a specified configuration node using a custom identifier. Unlike `SAVE_PROPERTY`, this macro allows you to specify a custom key name instead of using the property name.

**Parameters:**
- `NODE`: The TConfigNode reference to write to
- `ID`: The custom identifier/key to use for storage
- `PROPERTY`: The property variable to save

**Example:**
```cpp
auto& configNode = config.GetRootNode();
int fontSize = 14;
SAVE_ID_PROPERTY(configNode, "FontSize", fontSize);  // Saves using custom key
```

**Note:** The `*_ID_*` variants are useful when you need to use a different key name than the property name, or when working with properties that don't have meaningful names for storage purposes.

**Macro Categories:**
- **Local macros** (`RESTORE_LOCAL_PROPERTY`, `SAVE_LOCAL_PROPERTY`): Designed for use within form classes that inherit from `TPersistFormVCL` or `TPersistFormFMX`. They automatically use `GetConfigNode()` to access the form's configuration node.
- **General macros** (`RESTORE_PROPERTY`, `SAVE_PROPERTY`, `RESTORE_ID_PROPERTY`, `SAVE_ID_PROPERTY`): Can be used with any `TConfigNode` reference, providing flexibility for custom configuration scenarios.

## Usage Examples

### Basic Usage with Registry

```cpp
#include <anafestica/CfgRegistrySingleton.h>

// Get the singleton configuration
auto& config = Anafestica::TConfigRegistrySingleton::GetConfig();
auto& root = config.GetRootNode();

// Store a value
root.PutItem(_D("Setting1"), 42);
root.PutItem(_D("Setting2"), _D("Hello World"));

// Retrieve a value
int value = root.GetItem<int>(_D("Setting1"));

// Flush changes to registry
config.Flush();
```

### Using Sub-nodes

```cpp
// Create a sub-node
auto& subNode = root[_D("UserPreferences")];

// Store values in sub-node
subNode.PutItem(_D("Theme"), _D("Dark"));
subNode.PutItem(_D("FontSize"), 12);

// Access sub-node
String theme = root[_D("UserPreferences")].GetItem<String>(_D("Theme"));
```

### Form Persistence

```cpp
// In form header
class TMyForm : public Anafestica::TPersistFormVCL<Anafestica::TConfigRegistrySingleton> {
    // Form inherits persistence automatically
};

// In form implementation
TMyForm::TMyForm(TComponent* Owner)
    : TPersistFormVCL(Owner, StoreOpts::All)  // Persist position, size, and state
{
    // Additional initialization
}
```

### JSON Configuration

```cpp
#include <anafestica/CfgJSON.h>

Anafestica::JSON::TConfig config(_D("settings.json"));
auto& root = config.GetRootNode();

root.PutItem(_D("AppName"), _D("MyApplication"));
root.PutItem(_D("Version"), _D("1.0"));

// Configuration is automatically saved on destruction
```

### BSON Configuration

```cpp
#include <anafestica/CfgBSON.h>

Anafestica::BSON::TConfig config(_D("settings.bson"));
auto& root = config.GetRootNode();

root.PutItem(_D("AppName"), _D("MyApplication"));
root.PutItem(_D("Port"), 5432);

// Configuration is automatically saved on destruction
```

### YAML Configuration

```cpp
#include <anafestica/CfgYAML.h>

Anafestica::YAML::TConfig config(_D("settings.yaml"));
auto& root = config.GetRootNode();

root.PutItem(_D("AppName"), _D("MyApplication"));
root.PutItem(_D("Port"), 5432);

// Configuration is automatically saved on destruction
```

This requires fkYAML in header-only mode with its include directory registered in RAD Studio's include search path for the Clang-based compiler platforms used by the project.

### INI File Configuration

```cpp
#include <anafestica/CfgIniFile.h>

Anafestica::INIFile::TConfig config(_D("settings.ini"));
auto& root = config.GetRootNode();

root.PutItem(_D("AppName"), _D("MyApplication"));
root.PutItem(_D("Port"), 5432);

// Configuration is automatically saved on destruction
```

The resulting INI file will look like:

```ini
[config]
AppName::(sz)=MyApplication
Port::(i)=5432
```

### Using Collection Data Types

The library supports storing collections of strings and bytes. `StringCont` (defined as `std::vector<String>`) stores multiple strings and maps to `REG_MULTI_SZ` in the Windows Registry. `BytesCont` (defined as `std::vector<Byte>`) stores binary data.

```cpp
#include <anafestica/CfgRegistrySingleton.h>

// Get the singleton configuration
auto& config = Anafestica::TConfigRegistrySingleton::GetConfig();
auto& root = config.GetRootNode();

// Store a collection of strings (maps to REG_MULTI_SZ in registry)
StringCont recentFiles = { _D("file1.txt"), _D("file2.txt"), _D("file3.txt") };
root.PutItem(_D("RecentFiles"), recentFiles);

// Store binary data
BytesCont binaryData = { 0x01, 0x02, 0x03, 0x04 };
root.PutItem(_D("BinaryBlob"), binaryData);

// Retrieve the collections
StringCont loadedFiles = root.GetItem<StringCont>(_D("RecentFiles"));
BytesCont loadedData = root.GetItem<BytesCont>(_D("BinaryBlob"));

// Flush changes to registry
config.Flush();
```

### Working with TStrings-based Controls (TMemo)

The library provides specialized overloads of `GetItem` and `PutItem` that work directly with `TStrings` descendants (such as `TMemo::Lines`). These methods automatically convert between `TStrings` and the internal `StringCont` representation, making it convenient to persist multi-line text controls.

```cpp
#include <anafestica/CfgRegistrySingleton.h>
#include <vcl.h>

// Example: Saving and restoring TMemo lines
class TMyForm : public TForm {
private:
    TMemo* Memo1;
    
public:
    void SaveMemoContent() {
        auto& config = Anafestica::TConfigRegistrySingleton::GetConfig();
        auto& root = config.GetRootNode();
        
        // Save TMemo lines directly using the reference overload
        root.PutItem(_D("MemoLines"), Memo1->Lines);
        
        // Flush changes to registry (maps to REG_MULTI_SZ in Windows Registry)
        config.Flush();
    }
    
    void RestoreMemoContent() {
        auto& config = Anafestica::TConfigRegistrySingleton::GetConfig();
        auto& root = config.GetRootNode();
        
        // Restore TMemo lines directly using the reference overload
        // If the key doesn't exist or operation fails, Memo1->Lines remains unchanged
        root.GetItem(_D("MemoLines"), Memo1->Lines);
    }
};
```

The four `TStrings` specialized methods are:
- `void GetItem(String Id, TStrings& Val, Operation Op = Operation::None)` – Retrieve lines by reference
- `void GetItem(String Id, TStrings* const Val, Operation Op = Operation::None)` – Retrieve lines by pointer
- `bool PutItem(String Id, TStrings& Val, Operation Op = Operation::Write)` – Store lines by reference
- `bool PutItem(String Id, TStrings* const Val, Operation Op = Operation::Write)` – Store lines by pointer

Both pointer and reference overloads provide the same functionality; choose based on your coding style. The library internally converts between `TStrings` and `StringCont` (a `std::vector<String>`), allowing seamless integration with VCL controls.

### Service Application Configuration Example

Here is a real-world example showing how to use `TConfigJSON` to manage complex hierarchical configurations for a service application. This pattern demonstrates how to organize nested settings groups, track modifications, and persist configuration to JSON files.

**Header File (ServiceConfig.h):**

```cpp
#ifndef ServiceConfigH
#define ServiceConfigH

#include <anafestica/CfgItems.h>
#include <memory>

namespace ServiceApp::Config {

// Change tracking utility
template<typename T>
class ConfigValue {
public:
    ConfigValue() = default;
    ConfigValue(Anafestica::TConfigNode& Node, String KeyName, T DefaultValue)
        : value_(DefaultValue), oldValue_(DefaultValue) {
        try {
            Node.GetItem(KeyName, value_);
        } catch (...) {
            // Use default if key doesn't exist
        }
        oldValue_ = value_;
    }
    
    T Get() const noexcept { return value_; }
    void Set(T Val) noexcept { value_ = Val; }
    bool IsModified() const noexcept { return value_ != oldValue_; }
    void SetUnmodified() noexcept { oldValue_ = value_; }
    
private:
    T value_{};
    T oldValue_{};
};

// Database Configuration
class DatabaseSettings {
public:
    DatabaseSettings() = default;
    DatabaseSettings(Anafestica::TConfigNode& Cfg) 
        : server_(Cfg, _D("Server"), _D("localhost"))
        , port_(Cfg, _D("Port"), 5432)
        , database_(Cfg, _D("Database"), _D("myapp"))
        , username_(Cfg, _D("Username"), _D("admin"))
    {
    }
    
    void SaveTo(Anafestica::TConfigNode& Cfg) {
        Cfg.PutItem(_D("Server"), server_.Get());
        Cfg.PutItem(_D("Port"), port_.Get());
        Cfg.PutItem(_D("Database"), database_.Get());
        Cfg.PutItem(_D("Username"), username_.Get());
    }
    
    String GetServer() const noexcept { return server_.Get(); }
    void SetServer(String Val) noexcept { server_.Set(Val); }
    
    int GetPort() const noexcept { return port_.Get(); }
    void SetPort(int Val) noexcept { port_.Set(Val); }
    
    String GetDatabase() const noexcept { return database_.Get(); }
    void SetDatabase(String Val) noexcept { database_.Set(Val); }
    
    String GetUsername() const noexcept { return username_.Get(); }
    void SetUsername(String Val) noexcept { username_.Set(Val); }
    
    bool IsModified() const noexcept {
        return server_.IsModified() || port_.IsModified() 
            || database_.IsModified() || username_.IsModified();
    }
    
    void SetUnmodified() {
        server_.SetUnmodified();
        port_.SetUnmodified();
        database_.SetUnmodified();
        username_.SetUnmodified();
    }
    
private:
    ConfigValue<String> server_;
    ConfigValue<int> port_;
    ConfigValue<String> database_;
    ConfigValue<String> username_;
};

// Logging Configuration
class LoggingSettings {
public:
    LoggingSettings() = default;
    LoggingSettings(Anafestica::TConfigNode& Cfg)
        : enabled_(Cfg, _D("Enabled"), true)
        , logLevel_(Cfg, _D("LogLevel"), _D("Info"))
        , maxFileSize_(Cfg, _D("MaxFileSize"), 10485760)  // 10 MB
    {
    }
    
    void SaveTo(Anafestica::TConfigNode& Cfg) {
        Cfg.PutItem(_D("Enabled"), enabled_.Get());
        Cfg.PutItem(_D("LogLevel"), logLevel_.Get());
        Cfg.PutItem(_D("MaxFileSize"), maxFileSize_.Get());
    }
    
    bool GetEnabled() const noexcept { return enabled_.Get(); }
    void SetEnabled(bool Val) noexcept { enabled_.Set(Val); }
    
    String GetLogLevel() const noexcept { return logLevel_.Get(); }
    void SetLogLevel(String Val) noexcept { logLevel_.Set(Val); }
    
    int GetMaxFileSize() const noexcept { return maxFileSize_.Get(); }
    void SetMaxFileSize(int Val) noexcept { maxFileSize_.Set(Val); }
    
    bool IsModified() const noexcept {
        return enabled_.IsModified() || logLevel_.IsModified() 
            || maxFileSize_.IsModified();
    }
    
    void SetUnmodified() {
        enabled_.SetUnmodified();
        logLevel_.SetUnmodified();
        maxFileSize_.SetUnmodified();
    }
    
private:
    ConfigValue<bool> enabled_;
    ConfigValue<String> logLevel_;
    ConfigValue<int> maxFileSize_;
};

// Main Service Settings
class Settings {
public:
    Settings() = default;
    
    Settings(Anafestica::TConfigNode& Cfg)
        : database_(Cfg.GetSubNode(_D("Database")))
        , logging_(Cfg.GetSubNode(_D("Logging")))
    {
    }
    
    void LoadFrom(Anafestica::TConfigNode& Cfg) {
        database_ = DatabaseSettings(Cfg.GetSubNode(_D("Database")));
        logging_ = LoggingSettings(Cfg.GetSubNode(_D("Logging")));
    }
    
    void SaveTo(Anafestica::TConfigNode& Cfg) {
        database_.SaveTo(Cfg.GetSubNode(_D("Database")));
        logging_.SaveTo(Cfg.GetSubNode(_D("Logging")));
    }
    
    DatabaseSettings& GetDatabase() noexcept { return database_; }
    DatabaseSettings const& GetDatabase() const noexcept { return database_; }
    
    LoggingSettings& GetLogging() noexcept { return logging_; }
    LoggingSettings const& GetLogging() const noexcept { return logging_; }
    
    bool IsModified() const noexcept {
        return database_.IsModified() || logging_.IsModified();
    }
    
    void SetUnmodified() {
        database_.SetUnmodified();
        logging_.SetUnmodified();
    }
    
private:
    DatabaseSettings database_;
    LoggingSettings logging_;
};

}  // End of namespace ServiceApp::Config

#endif
```

**Implementation Example (Service Main Code):**

```cpp
#include <anafestica/CfgJSON.h>
#include "ServiceConfig.h"

using Anafestica::JSON::TConfig;

class ServiceApplication {
private:
    std::unique_ptr<TConfig> config_;
    ServiceApp::Config::Settings settings_;
    
public:
    ServiceApplication(String ConfigFileName) {
        // Load configuration from JSON file (read-only initially)
        config_ = std::make_unique<TConfig>(ConfigFileName, true);  // true = read-only
        auto& root = config_->GetRootNode();
        settings_ = ServiceApp::Config::Settings(root);
    }
    
    void SaveConfiguration() {
        if (settings_.IsModified()) {
            // Create a new writable config instance to save changes
            TConfig writableConfig(config file path, false);  // false = read-write
            settings_.SaveTo(writableConfig.GetRootNode());
            writableConfig.Flush();  // Write to file
            
            // Mark all settings as unmodified
            settings_.SetUnmodified();
        }
    }
    
    void ConfigureDatabase(String Server, int Port, String Database, String User) {
        auto& dbSettings = settings_.GetDatabase();
        dbSettings.SetServer(Server);
        dbSettings.SetPort(Port);
        dbSettings.SetDatabase(Database);
        dbSettings.SetUsername(User);
    }
    
    void ConfigureLogging(bool Enabled, String LogLevel, int MaxSize) {
        auto& logSettings = settings_.GetLogging();
        logSettings.SetEnabled(Enabled);
        logSettings.SetLogLevel(LogLevel);
        logSettings.SetMaxFileSize(MaxSize);
    }
    
    String GetDatabaseServer() const {
        return settings_.GetDatabase().GetServer();
    }
    
    bool HasPendingChanges() const {
        return settings_.IsModified();
    }
};

// Usage in application entry point
int main() {
    try {
        ServiceApplication app(_D("C:\\ProgramData\\MyService\\Config.json"));
        
        // Modify configuration as needed
        app.ConfigureDatabase(
            _D("db.example.com"), 5432, _D("production"), _D("svc_user")
        );
        app.ConfigureLogging(true, _D("Debug"), 50 * 1024 * 1024);  // 50 MB
        
        // Save only if changes were made
        if (app.HasPendingChanges()) {
            app.SaveConfiguration();
        }
        
        // Use configuration...
        String server = app.GetDatabaseServer();
        
    } catch (Exception& E) {
        MessageBox(NULL, E.Message.c_str(), _D("Error"), MB_OK | MB_ICONERROR);
        return 1;
    }
    
    return 0;
}
```

**Generated JSON File Structure:**

```json
{
  "Database": {
    "Server": "db.example.com",
    "Port": 5432,
    "Database": "production",
    "Username": "svc_user"
  },
  "Logging": {
    "Enabled": true,
    "LogLevel": "Debug",
    "MaxFileSize": 52428800
  }
}
```

This pattern demonstrates:
- **Hierarchical organization**: Settings grouped into logical sections using sub-nodes
- **Change tracking**: Each setting tracks its original value for detecting modifications
- **Default values**: Fallback defaults if configuration keys don't exist
- **Nested objects**: `Settings` class manages multiple configuration groups
- **Atomic saves**: Only write to file when there are actual changes
- **Type safety**: Strongly-typed configuration properties

## Dependencies

- **Variant back-end (auto-detected)**: `anafestica/CfgNodeValueType.h` selects the variant implementation automatically based on the toolchain's predefined macros — you do **not** need to define anything by hand. `bcc64x` (Clang ≥ 15, `_WIN64`) uses `std::variant` with 21 alternatives (including `std::string` / `std::wstring`); `bcc64` (Clang < 15, affected by RSP-27418) and `bcc32c` fall back to `boost::variant` with 19 alternatives. The internal flag `ANAFESTICA_USE_STD_VARIANT` is defined inside the header on the `bcc64x` path — it is not a user-facing switch, and overriding it manually is unsupported. Legacy non-Clang `BCC32` produces a hard `#error`.
- **Boost Libraries**: Required by the library on the `boost::variant` path only (i.e. `bcc32c` and `bcc64`). The `bcc64x` library build uses `std::variant` and does not need Boost for value storage. Separately, the bundled test projects still use Boost.Test on all three toolchains.
- **fkYAML**: Required only when using the YAML backend. `CfgYAML.h` includes `<fkYAML/node.hpp>`, so install fkYAML as a header-only dependency and add its include directory to RAD Studio's include search path for any Clang-based compiler platform that builds `CfgYAML.h` or `CfgYAMLSingleton.h`. The repository includes `register_fkYAML.bat` to automate that registration. The bundled test script does not enable YAML coverage by default; run `test_all.bat --with-yaml` to opt in. If that option is used but fkYAML is not visible, the YAML test block is compiled out.
- **Embarcadero C++ Compiler**: Only clang-based compilers (bcc32c, bcc64, bcc64x) are supported
- **RAD Studio**: Compatible with RAD Studio 10.3+ (earlier versions may work but are untested)

## Installation

1. Clone or download the library headers
2. Add the library path to your project's include directories, either manually or with `register_anafestica.bat`
3. For Registry usage, ensure your project has proper version info set (CompanyName, ProductName, ProductVersion)
4. Install Boost libraries via GetIt or manually when targeting `bcc64` / `bcc32c`, or whenever you want to build the bundled Boost.Test-based test suite
5. If you use `ConfigYAML` / `Anafestica::YAML::TConfig` or `ConfigYAMLSingleton` / `Anafestica::TConfigYAMLSingleton`, install fkYAML in header-only mode and register its include directory in RAD Studio's include search path for your Clang-based compiler platforms; `register_fkYAML.bat` can do this automatically when fkYAML is installed next to Anafestica under `$(BDSCOMMONDIR)`

## Thread Safety

The library performs no internal locking: there are no mutexes, critical sections, or atomics anywhere in `anafestica/`. Any `TConfig` or `TConfigNode` shared between threads must be externally synchronized.

**In practice this is rarely needed.** The library's intended use case is a standard VCL or FMX application in which configuration is handled exclusively on the **main (UI) thread** — the form-persistence classes (`TPersistFormVCL`, `TPersistFormFMX`) and the typical read-at-startup / write-at-shutdown pattern all run there. As long as your application follows that convention — no worker thread reads, writes, or even navigates the `TConfig` tree — the absence of internal locking is not a problem and you do not need to add any synchronization of your own. The rest of this section applies only when you deliberately choose to share a `TConfig` across threads.

**Why it is not thread-safe.** The unsafety is in the shared `TConfigNode` graph, not in the singleton mechanism. Singletons under C++11 guarantee thread-safe *construction* of the `static` instance (see [CfgRegistrySingleton.h:32](anafestica/CfgRegistrySingleton.h#L32)), so two threads calling `GetConfig()` concurrently will not double-construct. What singletons *do* is hand every thread a reference to the same `TConfig`, which routes all calls through the same root `TConfigNode` — but you would get the identical race by manually sharing a non-singleton `TConfig` instance.

The actual hazards are in `TConfigNode` itself:

- Operations that look like reads can mutate the node. `GetSubNode` lazily inserts a new child on miss, and `GetItem` goes through `GetItemFrom`, which inserts a default entry when the key is absent. So even "read-only" navigation writes to the underlying `std::map`s.
- `PutItem`, `DeleteItem`, `Clear`, `Read`, and `Write` all mutate `valueItems_` / `nodeItems_` or walk the subtree without locks.
- Backends hold non-thread-safe resources (`TRegistry`, `TMemIniFile`, `_di_IXMLDocument`, `TJSONObject`, `fkyaml::node`) and reuse them across calls.
- The RAII lifecycle flushes the *entire* tree in the owning `TConfig`'s destructor; this must not overlap with any other thread's access to any part of the tree.

**Working with disjoint subtrees.** Two `TConfigNode` objects that share no ancestor-path in the live graph can be driven by two threads concurrently, because each node owns its own `valueItems_` and `nodeItems_` maps — there is no hidden shared state inside `TConfigNode`. To use this safely:

1. Navigate from the root on a single thread and obtain the two `TConfigNode&` references you want to hand off.
2. After that point, no thread may touch any common ancestor (including the root) — remember that `GetSubNode` / `GetItem` on an ancestor can silently mutate it.
3. The owning `TConfig`'s construction (which reads the full tree from storage) and destruction (which flushes it back) must not overlap with any concurrent access, even to disjoint subtrees.

In practice, the simplest safe pattern is still to serialize all access to the `TConfig` with an external mutex; disjoint-subtree concurrency is an option when contention on the root becomes a measured problem.

## Error Handling

The library uses exceptions for error conditions:
- Registry access errors throw `ERegistryException`
- File I/O errors throw standard C++ exceptions
- JSON/BSON/YAML/XML parsing or serialization errors throw the underlying RTL/parser exceptions

## Best Practices

1. Use singletons for application-wide configuration
2. Group related settings in sub-nodes
3. Use meaningful names for configuration keys
4. Call `Flush()` explicitly or rely on destructor for automatic saving
5. Handle exceptions appropriately in production code
6. Use properties with persistence macros for cleaner code (as shown in examples)
7. Use the `_D()` macro around string literals for proper Unicode support in Embarcadero C++
8. **For Clang-based compilers (bcc32c, bcc64, bcc64x)**: Add form cleanup code in your project's main application file to ensure proper destruction order. Since the library uses singletons for serialization, you must manually destroy all forms before the application terminates to prevent access to destroyed singletons. Add the following code after `Application->Run()`:

   ```cpp
   while (auto const Cnt = Screen->FormCount) {
       delete Screen->Forms[Cnt - 1];
   }
   ```

   This ensures that form objects (which may hold references to configuration singletons) are properly destroyed before the singletons themselves are cleaned up by the runtime.

   Example: 
   
   ```cpp
   int WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
   {
       try
       {
           Application->Initialize();
           Application->MainFormOnTaskBar = true;
           Application->CreateForm(__classid(TForm1), &Form1);
           Application->Run();
           while ( auto const Cnt = Screen->FormCount ) {
               delete Screen->Forms[Cnt - 1];
           }
       }
       catch (Exception &exception) {
           Application->ShowException(&exception);
       }
       catch (...)
       {
           try {
               throw Exception("");
           }
           catch (Exception &exception) {
               Application->ShowException(&exception);
           }
       }
       return 0;
   }
   ```

## Limitations

- Limited to Embarcadero C++ compilers
- Windows Registry support is Windows-only
- No built-in encryption or security features
- No concurrent access protection (it's not thread safe)
- Persistence load/flush rejects paths deeper than `TConfigNode::MaxPersistenceDepth` to avoid stack exhaustion on maliciously deep trees
