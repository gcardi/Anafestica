//---------------------------------------------------------------------------

#ifndef CfgXMLH
#define CfgXMLH

#include <Xml.XMLIntf.hpp>
#include <Xml.XMLDoc.hpp>
#include <System.Win.ComObj.hpp>

#include <anafestica/Cfg.h>

#pragma comment( lib, "xmlrtl" )

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
        if ( FileExists( fileName_ ) ) {
            XMLObjRAII XML( *this );
            CheckDocument();
            GetRootNode().Read( *this, GetRootNodeName() );
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
    class XMLObjRAII {
    public:
        XMLObjRAII( TConfig& Cfg ) : cfg_{ Cfg } { cfg_.CreateXMLObject(); }
        ~XMLObjRAII() noexcept {
            try { cfg_.DestroyAndCloseXMLObject(); } catch ( ... ) {}
        }
        XMLObjRAII( XMLObjRAII const & ) = delete;
        XMLObjRAII& operator=( XMLObjRAII const & ) = delete;
    private:
        TConfig& cfg_;
    };

    _di_IXMLDocument XMLDoc_;
    String fileName_;

    static constexpr LPCTSTR DocumentVersion = _T( "1.0" );
    static constexpr LPCTSTR DocumentEncoding = _T( "UTF-8" );
    static constexpr LPCTSTR DocumentStandAlone = _T( "yes" );

    static constexpr LPCTSTR DocumentRootNodeName = _T( "application" );
    static constexpr LPCTSTR RootNodeName = _T( "config" );
    static constexpr LPCTSTR NodesNodeName = _T( "nodes" );
    static constexpr LPCTSTR NodeNodeName = _T( "node" );

    static constexpr LPCTSTR IdValueName = _T( "id" );

    void CreateXMLObject() {
        if ( TFile::Exists( fileName_ ) ) {
            XMLDoc_ = LoadXMLDocument( fileName_ );
            CheckDocument();
            XMLDoc_->Options = XMLDoc_->Options << doNodeAutoIndent;
        }
        else {
            XMLDoc_ = NewXMLDocument();
            XMLDoc_->Version = DocumentVersion;
            XMLDoc_->Encoding = DocumentEncoding;
            XMLDoc_->StandAlone = DocumentStandAlone;
            XMLDoc_->Options = XMLDoc_->Options << doNodeAutoIndent;
            XMLDoc_->AddChild( DocumentRootNodeName );
            XMLDoc_->DocumentElement->AddChild( RootNodeName );
        }
    }

    void DestroyAndCloseXMLObject() { XMLDoc_.Release(); }

    void CheckDocument() {
        if ( XMLDoc_->Encoding != DocumentEncoding ) {
            throw EXMLDocError(
                Format(
                    _T( "The document should be encoded using \'%s\' encoding" ),
                    ARRAYOFCONST(( DocumentEncoding ))
                )
            );
        }
    }

//    //config/nodes/node[@id="Form1"]/nodes/node[@id="Form2"]
    _di_IXMLNode OpenExistingPath( String Path ) {

        if ( _di_IXMLNode CurrentNode = XMLDoc_->DocumentElement ) {
        _di_IDOMNodeSelect DOMNode = CurrentNode->GetDOMNode();
            _di_IDOMNode ItemDOMNode;
            OleCheck( DOMNode->selectNode( Path, ItemDOMNode ) );
            if ( ItemDOMNode ) {
                return CurrentNode;
            }
        }
        return _di_IXMLNode{};
    }

/*
    config/Form1/Form2

    //config
    /nodes
    /node[@id="Form1"]
    /nodes
    /node[@id="Form2"]
*/
    _di_IXMLNode ForcePath( String Path ) {

    }



protected:
/*
    virtual String DoGetRootNodeName() const {
        return Format( _T( "//%s" ), ARRAYOFCONST(( RootNodeName )) );
    }

    virtual String DoGetNodePathSeparator() const override {
        return _T( "/" );
    }
*/

    virtual ValueContType DoCreateValueList( TConfigPath Path ) override {
        ValueContType Values;
        return Values;
    }

    virtual NodeContType DoCreateNodeList( TConfigPath Path ) override {
        NodeContType Nodes;
        return Nodes;
    }

    virtual void DoSaveValueList( TConfigPath Path, ValueContType const & Values ) override {
        ::OutputDebugString( KeyName.c_str() );
        if ( !Values.empty() ) {
            ::OutputDebugString( KeyName.c_str() );
            auto Node = ForcePath( KeyName );

            /*
            if ( auto Key = OpenPath( Format( _T( "%s\\%s" ), KeyName, ValuesNodeName ), true ) ) {
                for ( auto& v : Values ) {
                    auto const ValueState = v.second.second;
                    if ( GetAlwaysFlushNodeFlag() || ValueState == Operation::Write ) {
                        SaveValue( *Key, v );
                    }
                    else if ( v.second.second == Operation::Erase ) {
                        DeleteValue( *Key, v );
                    }
                }
            }
            */
        }
    }
/*
    virtual void Do1SaveNodeList( String KeyName, NodeContType const & Nodes ) override {
        for ( auto const & n : Nodes ) {
            if ( GetAlwaysFlushNodeFlag() || n.second->IsModified() ) {
                n.second->Write(
                    *this,
                    Format(
                        _T( "%s\\%s\\%s" ), KeyName, NodesNodeName, n.first
                    )
                );
            }
        }
    }
*/

    virtual void DoDeleteNode( TConfigPath Path ) override {
    }

    virtual void DoFlush() override {
        XMLObjRAII XML{ *this };
        GetRootNode().Write( *this, GetRootNodeName() );

        if ( !TFile::Exists( fileName_ ) ) {
            auto Path = TPath::GetFullPath( TPath::GetDirectoryName( fileName_ ) );
            if ( !TDirectory::Exists( Path ) ) {
                TDirectory::CreateDirectory( Path );
            }
        }
        XMLDoc_->SaveToFile( fileName_ );
    }

    virtual bool DoGetForcedWritesFlag() const {
        return false;
    }
};

//---------------------------------------------------------------------------
} // End namespace XML
//---------------------------------------------------------------------------
} // End of namespace Anafestica
//---------------------------------------------------------------------------

#endif

