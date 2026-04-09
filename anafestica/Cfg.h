//---------------------------------------------------------------------------

#ifndef CfgH
#define CfgH

#include <memory>

#include <anafestica/CfgItems.h>

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------

/// Abstract base for all configuration backends.
///
/// Owns a root @ref TConfigNode and delegates storage I/O to pure-virtual
/// hooks that concrete backends (Registry, JSON, XML, INI) override.
///
/// @par Non-Virtual Interface (NVI) idiom
/// The public API consists entirely of non-virtual methods (@c GetRootNode,
/// @c Flush, @c CreateValueList, etc.) that forward to @c protected virtual
/// @c Do* counterparts.  This guarantees that every call to a concrete
/// backend passes through the base-class interface first — the base class
/// owns the call protocol, and subclasses can only customise the
/// implementation, never bypass the entry point.  Client code therefore
/// interacts exclusively with the stable public surface of @c TConfig,
/// regardless of which backend is in use.
///
/// @par RAII lifecycle
/// Concrete constructors call @c GetRootNode().Read(*this, ...) to
/// populate the in-memory tree from storage.  Destructors call
/// @c DoFlush() to write dirty nodes back.  If the process crashes
/// before the destructor runs, the storage medium is left unchanged.
///
/// @par Backend contract
/// Subclasses must implement the following @c protected virtual hooks:
/// - @c DoCreateValueList — deserialise one node's values from storage.
/// - @c DoCreateNodeList  — enumerate child nodes at a path.
/// - @c DoSaveValueList   — serialise one node's values to storage.
/// - @c DoDeleteNode      — remove a node and its children.
/// - @c DoFlush           — commit all pending changes.
class TConfig {
public:
    TConfig( bool ReadOnly, bool FlushAllItems )
      : readOnly_{ ReadOnly }
      , flushAllItems_{ FlushAllItems }
      , root_{ new TConfigNode{} }
    {}
    TConfigNode& GetRootNode() { return DoGetRootNode(); }
    void Flush() { DoFlush(); }
    ValueContType CreateValueList( TConfigPath const & Path ) {
        return DoCreateValueList( Path );
    }

    NodeContType CreateNodeList( TConfigPath const & Path ) {
        return DoCreateNodeList( Path );
    }

    void SaveValueList( TConfigPath const & Path, ValueContType const & Values ) {
        DoSaveValueList( Path, Values );
    }

    void DeleteNode( TConfigPath const & Path ) { DoDeleteNode( Path ); }

    [[nodiscard]] bool GetReadOnlyFlag() const noexcept { return readOnly_; }

    /// When true, @ref TConfigNode::Write flushes every node regardless
    /// of its dirty state.  File-based backends (JSON, XML, INI) set this
    /// because they rewrite the entire file atomically.
    [[nodiscard]] bool GetAlwaysFlushNodeFlag() const noexcept { return flushAllItems_; }

protected:
    /// Deserialises the values stored at @p Path into a value map.
    ///
    /// Each backend parses its own format, resolves the type tag, and
    /// constructs the correct @ref TConfigNodeValueType alternative via
    /// a builder array indexed by @ref TypeTag.
    virtual ValueContType DoCreateValueList( TConfigPath const & Path ) = 0;
    virtual NodeContType DoCreateNodeList( TConfigPath const & Path ) = 0;
    virtual void DoSaveValueList( TConfigPath const & Path, ValueContType const & Values ) = 0;
    virtual TConfigNode& DoGetRootNode() { return *root_; }
    virtual void DoDeleteNode( TConfigPath const & Path ) = 0;
    virtual void DoFlush() = 0;
private:
    using TConfigNodePtr = std::unique_ptr<TConfigNode>;

    bool readOnly_ {};
    bool flushAllItems_ {};
    TConfigNodePtr root_;
};

//---------------------------------------------------------------------------
} // End of namespace Anafestica
//---------------------------------------------------------------------------

#endif
