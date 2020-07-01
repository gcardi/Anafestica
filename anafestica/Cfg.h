//---------------------------------------------------------------------------

#ifndef CfgH
#define CfgH

#include <memory>

#include <anafestica/CfgItems.h>

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------

class TConfig {
public:
    TConfig( bool ReadOnly, bool FlushAllItems )
      : readOnly_{ ReadOnly }
      , flushAllItems_{ FlushAllItems }
      , root_( new TConfigNode )
    {}
    TConfigNode& GetRootNode() { return DoGetRootNode(); }
    void Flush() { DoFlush(); }
    ValueContType CreateValueList( TConfigPath Path ) {
        return DoCreateValueList( Path );
    }

    NodeContType CreateNodeList( TConfigPath Path ) {
        return DoCreateNodeList( Path );
    }

    void SaveValueList( TConfigPath Path, ValueContType const & Values ) {
        DoSaveValueList( Path, Values );
    }

    void DeleteNode( TConfigPath Path ) { DoDeleteNode( Path ); }

    bool GetReadOnlyFlag() const noexcept { return readOnly_; }
    bool GetAlwaysFlushNodeFlag() const noexcept { return flushAllItems_; }

protected:
    virtual ValueContType DoCreateValueList( TConfigPath Path ) = 0;
    virtual NodeContType DoCreateNodeList( TConfigPath Path ) = 0;
    virtual void DoSaveValueList( TConfigPath Path, ValueContType const & Values ) = 0;
    virtual TConfigNode& DoGetRootNode() { return *root_; }
    virtual void DoDeleteNode( TConfigPath Path ) = 0;
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
