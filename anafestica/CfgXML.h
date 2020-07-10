//---------------------------------------------------------------------------

#ifndef CfgXMLH
#define CfgXMLH

#include <Xml.XMLIntf.hpp>
#include <Xml.XMLDoc.hpp>
//#include <System.Win.ComObj.hpp>
#include <Clipbrd.hpp>
#include <System.DateUtils.hpp>
#include <System.NetEncoding.hpp>

#include <utility>

#include <anafestica/Cfg.h>

#pragma comment( lib, "xmlrtl" )

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------
namespace XML {
//---------------------------------------------------------------------------

static constexpr LPCTSTR DocumentVersion = _T( "1.0" );
static constexpr LPCTSTR DocumentEncoding = _T( "UTF-8" );
static constexpr LPCTSTR DocumentStandAlone = _T( "yes" );

static constexpr LPCTSTR RootNodeName = _T( "application" );
static constexpr LPCTSTR ConfigNodeName = _T( "config" );
static constexpr LPCTSTR NodesNodeName = _T( "nodes" );
static constexpr LPCTSTR NodeNodeName = _T( "node" );
static constexpr LPCTSTR ValuesNodeName = _T( "values" );
static constexpr LPCTSTR ValueNodeName = _T( "value" );

static constexpr LPCTSTR NameAttrName = _T( "name" );
static constexpr LPCTSTR TypeAttrName = _T( "type" );

template<typename T>
class TD;

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
            GetRootNode().Read( *this, TConfigPath{} );
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
            XMLDoc_->AddChild( RootNodeName );
            XMLDoc_->DocumentElement->AddChild( ConfigNodeName );
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


    String ToString( TConfigPath const & Path ) {
        auto SB = std::make_unique<TStringBuilder>();
        for ( auto const & Name : Path ) {
            SB->AppendFormat( _T( "/%s" ), ARRAYOFCONST(( Name )) );
        }
        return SB->ToString();
    }

    _di_IXMLNode FindNodeByNameAndAttrValue( _di_IXMLNode Node, String NodeName,
                                             String AttrName, String AttrValue )
    {
        auto NodeList = Node->ChildNodes;
        for ( int Idx = 0 ; Idx < NodeList->Count ; ++Idx ) {
            if ( auto Node = NodeList->Nodes[Idx] ) {
                if ( Node->NodeName == NodeName ) {
                    auto const ElementValue = Node->GetAttribute( AttrName );
                    if ( !ElementValue.IsNull() && ElementValue == AttrValue ) {
                        return Node;
                    }
                }
            }
        }
        return _di_IXMLNode();
    }

    _di_IXMLNode OpenPath( TConfigPath Path ) {
        if ( _di_IXMLNode RootNode = XMLDoc_->DocumentElement ) {
            if ( _di_IXMLNode Current = RootNode->ChildNodes->FindNode( ConfigNodeName ) ) {
                for ( auto const & AttrName : Path ) {
                    if ( Current = Current->ChildNodes->FindNode( NodesNodeName ) ) {
                        if ( Current = FindNodeByNameAndAttrValue( Current, NodeNodeName, NameAttrName, AttrName ) ) {
                            return Current;
                        }
                    }
                }
                return Current;
            }
        }
        return _di_IXMLNode{};
    }

    _di_IXMLNode OpenValues( TConfigPath const & Path ) {
        if ( auto Node = OpenPath( Path ) ) {
            return Node->ChildNodes->FindNode( ValuesNodeName );
        }
        return _di_IXMLNode{};
    }

    _di_IXMLNode OpenNodes( TConfigPath const & Path ) {
        if ( auto Node = OpenPath( Path ) ) {
            return Node->ChildNodes->FindNode( NodesNodeName );
        }
        return _di_IXMLNode{};
    }

    _di_IXMLNode OpenOrForceNode( _di_IXMLNode Node, String NodeName )
    {
        if ( auto ResultNode = Node->ChildNodes->FindNode( NodeName ) ) {
            return ResultNode;
        }
        else {
            return Node->AddChild( NodeName );
        }
    }

    _di_IXMLNode OpenOrForceNode( _di_IXMLNode Node, String NodeName,
                                  String AttrName, String AttrValue )
    {
        if ( auto ResultNode = FindNodeByNameAndAttrValue( Node, NodeName, AttrName, AttrValue ) ) {
            return ResultNode;
        }
        else {
            ResultNode = Node->AddChild( NodeName );
            ResultNode->Attributes[AttrName] = AttrValue;
            return ResultNode;
        }
    }

    _di_IXMLNode ForcePath( TConfigPath const & Path ) {
        if ( auto RootNode = XMLDoc_->DocumentElement ) {
            if ( auto Current = OpenOrForceNode( RootNode, ConfigNodeName ) ) {
                for ( auto const & AttrName : Path ) {
                    if ( Current = OpenOrForceNode( Current, NodesNodeName ) ) {
                        Current =
                            OpenOrForceNode(
                                Current, NodeNodeName, NameAttrName, AttrName
                            );
                        if ( !Current ) {
                            break;
                        }
                    }
                    else {
                        break;
                    }
                }
                return Current;
            }
        }
        return _di_IXMLNode{};
    }

    _di_IXMLNode ForceValues( TConfigPath const & Path ) {
        if ( _di_IXMLNode Node = ForcePath( Path ) ) {
            return OpenOrForceNode( Node, ValuesNodeName );
        }
        return _di_IXMLNode{};
    }

    // https://www.bfilipek.com/2019/02/2lines3featuresoverload.html
    template<class... Ts> struct overload : Ts... { using Ts::operator()...; };
    template<class... Ts> overload( Ts... ) -> overload<Ts...>;

    void SaveValue( _di_IXMLNode Node, ValueContType::value_type const & v ) {
        if ( auto ValueNode =
                OpenOrForceNode( Node, ValueNodeName, NameAttrName, v.first ) )
        {
            boost::apply_visitor(
                overload {
                    [this, &ValueNode]( int Val ) {
                        ValueNode->Attributes[TypeAttrName] = String( _T( "i" ) );
                        ValueNode->Text = Val;
                    },

                    [this, &ValueNode]( unsigned int Val ) {
                        ValueNode->Attributes[TypeAttrName] = String( _T( "u" ) );
                        ValueNode->Text = static_cast<__int64>( Val );
                    },

                    [this, &ValueNode]( long Val ) {
                        ValueNode->Attributes[TypeAttrName] = String( _T( "l" ) );
                        ValueNode->Text = Val;
                    },

                    [this, &ValueNode]( unsigned long Val ) {
                        ValueNode->Attributes[TypeAttrName] = String( _T( "ul" ) );
                        ValueNode->Text = static_cast<__int64>( Val );
                    },

                    [this, &ValueNode]( char Val ) {
                        ValueNode->Attributes[TypeAttrName] = String( _T( "c" ) );
                        ValueNode->Text = static_cast<int>( Val );
                    },

                    [this, &ValueNode]( unsigned char Val ) {
                        ValueNode->Attributes[TypeAttrName] = String( _T( "uc" ) );
                        ValueNode->Text = static_cast<int>( Val );
                    },

                    [this, &ValueNode]( short Val ) {
                        ValueNode->Attributes[TypeAttrName] = String( _T( "s" ) );
                        ValueNode->Text = static_cast<int>( Val );
                    },

                    [this, &ValueNode]( unsigned short Val ) {
                        ValueNode->Attributes[TypeAttrName] = String( _T( "us" ) );
                        ValueNode->Text = static_cast<int>( Val );
                    },

                    [this, &ValueNode]( long long Val ) {
                        ValueNode->Attributes[TypeAttrName] = String( _T( "ll" ) );
                        ValueNode->Text = static_cast<__int64>( Val );
                    },

                    [this, &ValueNode]( unsigned long long Val ) {
                        ValueNode->Attributes[TypeAttrName] = String( _T( "ull" ) );
                        ValueNode->Text =
#if defined( _UNICODE )
                            std::to_wstring( Val ).c_str();
#else
                            std::to_string( Val ).c_str();
#endif
                    },

                    [this, &ValueNode]( bool Val ) {
                        ValueNode->Attributes[TypeAttrName] = String( _T( "b" ) );
                        ValueNode->Text = BoolToStr( Val, true );
                    },

                    [this, &ValueNode]( System::UnicodeString Val ) {
                        ValueNode->Attributes[TypeAttrName] = String( _T( "sz" ) );
                        ValueNode->Text = Val;
                    },

                    [this, &ValueNode]( System::TDateTime Val ) {
                        ValueNode->Attributes[TypeAttrName] = String( _T( "dt" ) );
                        ValueNode->Text = DateToISO8601( Val, false );
                    },

                    [this, &ValueNode]( float Val ) {
                        ValueNode->Attributes[TypeAttrName] = String( _T( "flt" ) );
                        ValueNode->Text = Val;
                    },

                    [this, &ValueNode]( double Val ) {
                        ValueNode->Attributes[TypeAttrName] = String( _T( "dbl" ) );
                        ValueNode->Text = Val;
                    },

                    [this, &ValueNode]( System::Currency Val ) {
                        ValueNode->Attributes[TypeAttrName] = String( _T( "cur" ) );
                        TFormatSettings FS;
                        FS.DecimalSeparator = _T( '.' );
                        ValueNode->Text = CurrToStr( Val, FS );
                    },

                    [this, &ValueNode]( StringCont const & Val ) {
                        ValueNode->Attributes[TypeAttrName] = String( _T( "sv" ) );
                        auto SB = std::make_unique<TStringBuilder>();
                        for ( auto const & Item : Val ) {
                            SB->AppendLine( Item );
                        }
                        ValueNode->Text = SB->ToString();
                    },

                    [this, &ValueNode]( TBytes Val ) {
                        ValueNode->Attributes[TypeAttrName] = String( _T( "dab" ) );
                        ValueNode->Text =
                            TNetEncoding::Base64->EncodeBytesToString(
                                &Val[0], Val.High
                            );
                    },

                    [this, &ValueNode]( std::vector<Byte> const & Val ) {
                        ValueNode->Attributes[TypeAttrName] = String( _T( "vb" ) );
                        ValueNode->Text =
                            Val.empty() ?
                              String()
                            :
                              TNetEncoding::Base64->EncodeBytesToString(
                                  Val.data(), Val.size() - 1
                              );
                    }
                },
                v.second.first
            );
        }
    }

protected:
    virtual ValueContType DoCreateValueList( TConfigPath const & Path ) override {
        ValueContType Values;

        if ( auto Node = OpenValues( Path ) ) {
::OutputDebugString( ToString( Path ).c_str() );
Clipboard()->AsText = Node->XML;
Clipboard()->AsText = Node->XML;
        }

        return Values;
    }

    virtual NodeContType DoCreateNodeList( TConfigPath const & Path ) override {
        NodeContType Nodes;
        if ( auto Node = OpenNodes( Path ) ) {
            auto Childs = Node->ChildNodes;
            for ( int Idx = 0 ; Idx < Childs->Count ; ++Idx  ) {
                auto ANode = Childs->Get( Idx );
                if ( ANode->NodeName == NodeNodeName ) {
                    auto Attributes = ANode->AttributeNodes;
                    if ( auto NameNode = Attributes->FindNode( NameAttrName ) ) {
                        Nodes[NameNode->Text] =
                            std::move( std::make_unique<TConfigNode>() );
                    }
                }
            }
        }
        return Nodes;
    }

    virtual void DoSaveValueList( TConfigPath const & Path, ValueContType const & Values ) override {
        if ( !Values.empty() ) {
            if ( auto Node = ForceValues( Path ) ) {
                for ( auto& v : Values ) {
                    auto const ValueState = v.second.second;
                    if ( GetAlwaysFlushNodeFlag() || ValueState == Operation::Write ) {
                        SaveValue( Node, v );
                    }
                    else if ( v.second.second == Operation::Erase ) {
                        Node->ChildNodes->Delete( v.first );
                    }
                }
            }
        }
    }

    virtual void DoDeleteNode( TConfigPath const & Path ) override {
        if ( auto Node = OpenPath( Path ) ) {
            auto Parent = Node->ParentNode;
            Parent->ChildNodes->Remove( Node );
        }
    }

    virtual void DoFlush() override {
        XMLObjRAII XML{ *this };
        GetRootNode().Write( *this, TConfigPath{} );

        if ( !TFile::Exists( fileName_ ) ) {
            auto DirPath = TPath::GetFullPath( TPath::GetDirectoryName( fileName_ ) );
            if ( !TDirectory::Exists( DirPath ) ) {
                TDirectory::CreateDirectory( DirPath );
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

