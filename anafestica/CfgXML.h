//---------------------------------------------------------------------------

#ifndef CfgXMLH
#define CfgXMLH

#include <anafestica/Cfg.h>

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------
namespace XML {
//---------------------------------------------------------------------------

class TConfig : public Anafestica::TConfig {
public:
    TConfig( String FileName, bool ReadOnly = false,
             bool FlushAllItems = false )
        : Anafestica::TConfig( ReadOnly, FlushAllItems )
        , fileName_( FileName )
    {
        GetRootNode().Read( *this, String() );
        if ( FileExists( fileName_ ) ) {
            XMLDoc_ = LoadXMLDocument( fileName_ );
            TRrid<TConfigXml,&TConfigXml::ReleaseXMLDoc> const XMLDocMngr( *this );
            CheckEncodingAndVersion( XMLDoc_ );
            GetRootNode().Read( *this, GetMainNodeName() );
        }
    }

    ~TConfig() {
        try {
            if ( !GetReadOnlyFlag() ) {
                DoFlush();
            }
        }
        catch ( ... ) {
        }
    }

    TConfig( TConfig const & ) = delete;
    TConfig& operator=( TConfig const & ) = delete;
private:
protected:
    virtual TConfigNode::ValueContType DoCreateValueList( String KeyName ) override {
        TConfigNode::ValueContType Values;

        return Values;
    }

    virtual TConfigNode::NodeContType DoCreateNodeList( String KeyName ) override {
        TConfigNode::NodeContType Nodes;
        return Nodes;
    }

    virtual void DoSaveValueList( String KeyName, TConfigNode::ValueContType const & Values ) override {
        if ( !Values.empty() ) {

        }
    }

    virtual void DoSaveNodeList( String KeyName, TConfigNode::NodeContType const & Nodes ) override {
    }

    virtual void DoDeleteNode( String KeyName ) override {
    }

    virtual void DoFlush() override {
    }

    virtual bool DoGetForcedWritesFlag() const {
        return false;
    }
private:
    String fileName_;
};

//---------------------------------------------------------------------------
} // End namespace XML
//---------------------------------------------------------------------------
} // End of namespace Anafestica
//---------------------------------------------------------------------------

#endif

