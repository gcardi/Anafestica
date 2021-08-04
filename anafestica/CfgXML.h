//---------------------------------------------------------------------------

#ifndef CfgXMLH
#define CfgXMLH

#include <Xml.XMLIntf.hpp>
#include <Xml.XMLDoc.hpp>
#include <System.DateUtils.hpp>
#include <System.NetEncoding.hpp>

#include <utility>
#include <string>

#include <anafestica/Cfg.h>

#pragma comment( lib, "xmlrtl" )

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------
namespace XML {
//---------------------------------------------------------------------------

static constexpr LPCTSTR DocumentVersion = _D( "1.0" );
static constexpr LPCTSTR DocumentEncoding = _D( "UTF-8" );
static constexpr LPCTSTR DocumentStandAlone = _D( "yes" );

static constexpr LPCTSTR RootNodeName = _D( "application" );
static constexpr LPCTSTR ConfigNodeName = _D( "config" );
static constexpr LPCTSTR NodesNodeName = _D( "nodes" );
static constexpr LPCTSTR NodeNodeName = _D( "node" );
static constexpr LPCTSTR ValuesNodeName = _D( "values" );
static constexpr LPCTSTR ValueNodeName = _D( "value" );

static constexpr LPCTSTR NameAttrName = _D( "name" );
static constexpr LPCTSTR TypeAttrName = _D( "type" );

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
                    _D( "The document should be encoded using \'%s\' encoding" ),
                    ARRAYOFCONST(( DocumentEncoding ))
                )
            );
        }
    }


    String ToString( TConfigPath const & Path ) {
        auto SB = std::make_unique<TStringBuilder>();
        for ( auto const & Name : Path ) {
            SB->AppendFormat( _D( "/%s" ), ARRAYOFCONST(( Name )) );
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
                        ValueNode->Attributes[TypeAttrName] =
                            String( cnv_xstr( TT_I ) );
                        ValueNode->Text = Val;
                    },

                    [this, &ValueNode]( unsigned int Val ) {
                        ValueNode->Attributes[TypeAttrName] =
                            String( cnv_xstr( TT_U ) );
                        ValueNode->Text = static_cast<__int64>( Val );
                    },

                    [this, &ValueNode]( long Val ) {
                        ValueNode->Attributes[TypeAttrName] =
                            String( cnv_xstr( TT_L ) );
                        ValueNode->Text = Val;
                    },

                    [this, &ValueNode]( unsigned long Val ) {
                        ValueNode->Attributes[TypeAttrName] =
                            String( cnv_xstr( TT_UL ) );
                        ValueNode->Text = static_cast<__int64>( Val );
                    },

                    [this, &ValueNode]( char Val ) {
                        ValueNode->Attributes[TypeAttrName] =
                            String( cnv_xstr( TT_C ) );
                        ValueNode->Text = static_cast<int>( Val );
                    },

                    [this, &ValueNode]( unsigned char Val ) {
                        ValueNode->Attributes[TypeAttrName] =
                            String( cnv_xstr( TT_UC ) );
                        ValueNode->Text = static_cast<int>( Val );
                    },

                    [this, &ValueNode]( short Val ) {
                        ValueNode->Attributes[TypeAttrName] =
                            String( cnv_xstr( TT_S ) );
                        ValueNode->Text = static_cast<int>( Val );
                    },

                    [this, &ValueNode]( unsigned short Val ) {
                        ValueNode->Attributes[TypeAttrName] =
                            String( cnv_xstr( TT_US ) );
                        ValueNode->Text = static_cast<int>( Val );
                    },

                    [this, &ValueNode]( long long Val ) {
                        ValueNode->Attributes[TypeAttrName] =
                            String( cnv_xstr( TT_LL ) );
                        ValueNode->Text = static_cast<__int64>( Val );
                    },

                    [this, &ValueNode]( unsigned long long Val ) {
                        ValueNode->Attributes[TypeAttrName] =
                            String( cnv_xstr( TT_ULL ) );
                        ValueNode->Text =
#if defined( _UNICODE )
                            std::to_wstring( Val ).c_str();
#else
                            std::to_string( Val ).c_str();
#endif
                    },

                    [this, &ValueNode]( bool Val ) {
                        ValueNode->Attributes[TypeAttrName] =
                            String( cnv_xstr( TT_B ) );
                        ValueNode->Text = BoolToStr( Val, true );
                    },

                    [this, &ValueNode]( System::UnicodeString Val ) {
                        ValueNode->Attributes[TypeAttrName] =
                            String( cnv_xstr( TT_SZ ) );
                        ValueNode->Text = Val;
                    },

                    [this, &ValueNode]( System::TDateTime Val ) {
                        ValueNode->Attributes[TypeAttrName] =
                            String( cnv_xstr( TT_DT ) );
                        ValueNode->Text = DateToISO8601( Val, false );
                    },

                    [this, &ValueNode]( float Val ) {
                        ValueNode->Attributes[TypeAttrName] =
                            String( cnv_xstr( TT_FLT ) );
                        ValueNode->Text = Val;
                    },

                    [this, &ValueNode]( double Val ) {
                        ValueNode->Attributes[TypeAttrName] =
                            String( cnv_xstr( TT_DBL ) );
                        ValueNode->Text = Val;
                    },

                    [this, &ValueNode]( System::Currency Val ) {
                        ValueNode->Attributes[TypeAttrName] =
                            String( cnv_xstr( TT_CUR ) );
                        TFormatSettings FS;
                        FS.DecimalSeparator = _D( '.' );
                        ValueNode->Text = CurrToStr( Val, FS );
                    },

                    [this, &ValueNode]( StringCont const & Val ) {
                        ValueNode->Attributes[TypeAttrName] =
                            String( cnv_xstr( TT_SV ) );
                        auto SB = std::make_unique<TStringBuilder>();
                        for ( auto const & Item : Val ) {
                            SB->AppendLine( Item );
                        }
                        ValueNode->Text = SB->ToString();
                    },

                    [this, &ValueNode]( TBytes Val ) {
                        ValueNode->Attributes[TypeAttrName] =
                            String( cnv_xstr( TT_DAB ) );
                        ValueNode->Text =
                            TNetEncoding::Base64->EncodeBytesToString(
                                &Val[0], Val.High
                            );
                    },

                    [this, &ValueNode]( std::vector<Byte> const & Val ) {
                        ValueNode->Attributes[TypeAttrName] =
                            String( cnv_xstr( TT_VB ) );
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
        using Fn = std::function<TConfigNodeValueType(String)>;

        static std::array<Fn,TConfigNodeValueType::types::size::value> Builders {
            // TT_I
            []( String Value ) {
                return std::stoi( Value.c_str() );
            },

            // TT_U
            []( String Value ) {
                return static_cast<unsigned>( std::stoul( Value.c_str() ) );
            },

            // TT_L
            []( String Value ) {
                return std::stol( Value.c_str() );
            },

            // TT_UL
            []( String Value ) {
                return std::stoul( Value.c_str() );
            },

            // TT_C
            []( String Value ) {
                return static_cast<char>( std::stoi( Value.c_str() ) );
            },

            // TT_UC
            []( String Value ) {
                return static_cast<unsigned char>( std::stoul( Value.c_str() ) );
            },

            // TT_S
            []( String Value ) {
                return static_cast<short>( std::stoi( Value.c_str() ) );
            },

            // TT_US
            []( String Value ) {
                return static_cast<unsigned short>( std::stoul( Value.c_str() ) );
            },

            // TT_LL
            []( String Value ) {
                return std::stoll( Value.c_str() );
            },

            // TT_ULL
            []( String Value ) {
                return std::stoull( Value.c_str() );
            },

            // TT_B
            []( String Value ) {
                return StrToBool( Value.c_str() );
            },

            // TT_SZ
            []( String Value ) {
                return Value;
            },

            // TT_DT
            []( String Value ) {
                return ISO8601ToDate( Value, false );
            },

            // TT_FLT
            []( String Value ) {
                return std::stof( Value.c_str() );
            },

            // TT_DBL
            []( String Value ) {
                return std::stod( Value.c_str() );
            },

            // TT_CUR
            []( String Value ) {
                TFormatSettings FS;
                FS.DecimalSeparator = _D( '.' );
                return StrToCurr( Value, FS );
            },

            // TT_SV
            [this]( String Value ) {
                auto SL = std::make_unique<TStringList>();
                SL->Text = Value;
                StringCont Strings;
                Strings.reserve( SL->Count );
                for ( auto const & Line : SL.get() ) {
                    Strings.push_back( Line );
                }
                return Strings;
            },

            // TT_DAB
            []( String Value ) {
                return TNetEncoding::Base64->DecodeStringToBytes( Value );
            },

            // TT_VB
            []( String Value ) {
                auto Bytes = TNetEncoding::Base64->DecodeStringToBytes( Value );
                std::vector<Byte> VBytes;
                VBytes.reserve( Bytes.Length );
                std::copy(
                    System::begin( &Bytes ), System::end( &Bytes ),
                    std::back_inserter( VBytes )
                );
                return VBytes;
            },
        };

        ValueContType Values;

        if ( auto Node = OpenValues( Path ) ) {
            auto Childs = Node->ChildNodes;
            for ( int Idx = 0 ; Idx < Childs->Count ; ++Idx  ) {
                auto ValueNode = Childs->Get( Idx );
                if ( ValueNode->NodeName == ValueNodeName ) {
                    auto Attributes = ValueNode->AttributeNodes;
                    if ( auto NameNode = Attributes->FindNode( NameAttrName ) ) {
                        if ( auto TypeNode = Attributes->FindNode( TypeAttrName ) ) {
                            if ( auto Result = GetTypeTag( TypeNode->Text ) ) {
                                PutItemTo(
                                    Values, NameNode->Text,
                                    {
                                        Builders[static_cast<size_t>( Result.value() )](
                                            ValueNode->Text
                                        ),
                                        Operation::None
                                    }
                                );
                            }
                        }
                    }
                }
            }
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

