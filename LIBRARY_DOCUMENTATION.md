# Anafestica Library Documentation

## Overview

Anafestica is a header-only C++ library designed for the persistence of application settings in various storage media, including the Windows Registry, JSON files, and XML files. It provides a hierarchical, heterogeneous container that mimics the Windows Registry structure but operates in application memory, allowing seamless saving and loading of configuration data.

The library is particularly well-suited for GUI applications (FMX, VCL, and others) where it simplifies the management of persistent attributes such as form positions, sizes, states, and custom application settings. It requires minimal code changes to existing applications and supports multiple storage formats through a policy-based design.

## Key Features

- **Header-only library**: No compilation required, just include the necessary headers
- **Multiple storage backends**: Windows Registry, JSON files, XML files
- **Hierarchical data structure**: Tree-like organization similar to Windows Registry
- **Type-safe operations**: Supports various data types including primitives, strings, dates, and collections
- **Singleton pattern support**: Easy access through singleton classes
- **Form persistence helpers**: Specialized classes for VCL and FMX form persistence
- **Cross-platform compatibility**: Works with Embarcadero C++ compilers (bcc32c, bcc64)

## Architecture

The library consists of two main parts:

1. **Container Part**: A generalized hierarchical container (`TConfigNode`) that holds configuration data in memory
2. **Serialization Part**: Policy-based serializers that handle reading from and writing to different storage media

The container uses a tree structure where each node can contain:
- Named values of various types
- Named sub-nodes (child nodes)

## Core Classes

### TConfig (Abstract Base Class)

The base class for all configuration implementations. It provides the interface for configuration operations.

#### Public Members

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

### TConfigNode

The main class representing a node in the configuration tree. Each node can contain values and sub-nodes.

#### Public Members

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
- `EnumerateNodes()`: Lists all sub-node names
- `EnumerateValueNames()`: Lists all value names
- `EnumeratePairs()`: Lists all name-value pairs

**Supported Data Types:**
The library supports the following data types through template specialization:
- `int`, `unsigned int`, `long`, `unsigned long`
- `char`, `unsigned char`, `short`, `unsigned short`
- `long long`, `unsigned long long`
- `bool`
- `String` (System::String)
- `TDateTime`
- `float`, `double`, `Currency`
- `StringCont` (std::vector<String>)
- `TBytes`, `BytesCont` (std::vector<Byte>)

**Special Handling for Enums:**
Enumerated types are automatically handled. If the enum has RTTI information, it's stored as a string; otherwise, as an integer.

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

### JSON::TConfig

Implements configuration storage in JSON files.

```cpp
namespace JSON {
class TConfig : public Anafestica::TConfig {
public:
    TConfig(String FileName, bool ReadOnly = false, bool Compact = true,
            bool FlushAllItems = false, bool ExplicitTypes = false);
};
}
```

**Constructor Parameters:**
- `FileName`: Path to the JSON file
- `Compact`: If true, outputs compact JSON; if false, pretty-printed
- `ExplicitTypes`: If true, includes type information in JSON
- `ReadOnly`, `FlushAllItems`: Same as base class

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

## Usage Examples

### Basic Usage with Registry

```cpp
#include <anafestica/CfgRegistrySingleton.h>

// Get the singleton configuration
auto& config = Anafestica::TConfigRegistrySingleton::GetConfig();
auto& root = config.GetRootNode();

// Store a value
root.PutItem("Setting1", 42);
root.PutItem("Setting2", "Hello World");

// Retrieve a value
int value = root.GetItem<int>("Setting1");

// Flush changes to registry
config.Flush();
```

### Using Sub-nodes

```cpp
// Create a sub-node
auto& subNode = root["UserPreferences"];

// Store values in sub-node
subNode.PutItem("Theme", "Dark");
subNode.PutItem("FontSize", 12);

// Access sub-node
String theme = root["UserPreferences"].GetItem<String>("Theme");
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

Anafestica::JSON::TConfig config("settings.json");
auto& root = config.GetRootNode();

root.PutItem("AppName", "MyApplication");
root.PutItem("Version", 1.0);

// Configuration is automatically saved on destruction
```

## Dependencies

- **Boost Libraries**: Required for `boost::variant` (unless `ANAFESTICA_USE_STD_VARIANT` is defined)
- **Embarcadero C++ Compiler**: Only clang-based compilers (bcc32c, bcc64) are supported
- **RAD Studio**: Compatible with RAD Studio 10.3+ (earlier versions may work but are untested)

## Installation

1. Clone or download the library headers
2. Add the library path to your project's include directories
3. For Registry usage, ensure your project has proper version info set (CompanyName, ProductName, ProductVersion)
4. Install Boost libraries via GetIt or manually

## Thread Safety

The library is not thread-safe. Access to configuration objects should be serialized if used from multiple threads.

## Error Handling

The library uses exceptions for error conditions:
- Registry access errors throw `ERegistryException`
- File I/O errors throw standard C++ exceptions
- JSON/XML parsing errors throw appropriate parser exceptions

## Best Practices

1. Use singletons for application-wide configuration
2. Group related settings in sub-nodes
3. Use meaningful names for configuration keys
4. Call `Flush()` explicitly or rely on destructor for automatic saving
5. Handle exceptions appropriately in production code
6. Use properties with persistence macros for cleaner code (as shown in examples)

## Limitations

- Limited to Embarcadero C++ compilers
- Windows Registry support is Windows-only
- No built-in encryption or security features
- No concurrent access protection</content>
<parameter name="filePath">c:\Users\Public\Documents\Embarcadero\Studio\37.0\Anafestica\LIBRARY_DOCUMENTATION.md