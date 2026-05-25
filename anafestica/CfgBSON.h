//---------------------------------------------------------------------------

#ifndef CfgBSONH
#define CfgBSONH

#include <System.JSON.hpp>
#include <System.JSON.BSON.hpp>
#include <System.JSON.Writers.hpp>
#include <System.JSON.Readers.hpp>
#include <System.IOUtils.hpp>
#include <System.DateUtils.hpp>
#include <System.NetEncoding.hpp>
#include <System.SysUtils.hpp>
#include <System.Classes.hpp>

#include <memory>
#include <vector>
#include <array>
#include <functional>
#include <iterator>

#include <anafestica/Cfg.h>
#include <anafestica/CfgCrypt.h>

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------
namespace BSON {
//---------------------------------------------------------------------------

class TConfig : public Anafestica::TConfig {
private:
    static constexpr LPCTSTR ValuesNodeName = _D( "values" );
    static constexpr LPCTSTR NodesNodeName = _D( "nodes" );
public:
    TConfig( String FileName, bool ReadOnly = false,
             bool FlushAllItems = false, bool ExplicitTypes = false,
             Crypt::TOptions CryptOptions = {} )
        : Anafestica::TConfig( ReadOnly, FlushAllItems )
        , fileName_{ FileName }, loadFileName_{ FileName }
        , explicitTypes_{ ExplicitTypes }
        , cryptOptions_{ CryptOptions }
    {
        if ( TFile::Exists( loadFileName_ ) ) {
            BSONObjRAII BSON{ *this };
            GetRootNode().Read( *this, TConfigPath{} );
        }
    }

    /// Migration constructor: loads from @p LoadFileName, persists to
    /// @p SaveFileName.  Destination wins when both exist.  FlushAllItems
    /// is forced @c true so that values in @c Operation::None state are
    /// still written to the destination.  See @ref Migrate for a named
    /// wrapper that makes the load/save direction unambiguous.
    TConfig( String LoadFileName, String SaveFileName,
             bool ReadOnly = false, bool ExplicitTypes = false,
             Crypt::TOptions CryptOptions = {} )
        : Anafestica::TConfig( ReadOnly, /*FlushAllItems*/ true )
        , fileName_{ SaveFileName }
        , loadFileName_{
            TFile::Exists( SaveFileName ) ? SaveFileName : LoadFileName
          }
        , explicitTypes_{ ExplicitTypes }
        , cryptOptions_{ CryptOptions }
    {
        if ( TFile::Exists( loadFileName_ ) ) {
            BSONObjRAII BSON{ *this };
            GetRootNode().Read( *this, TConfigPath{} );
            if ( loadFileName_ != fileName_ ) {
                MarkForFlush();
            }
        }
    }

    static TConfig Migrate( String LoadFileName, String SaveFileName,
                            bool ReadOnly = false, bool ExplicitTypes = false,
                            Crypt::TOptions CryptOptions = {} )
    {
        return TConfig(
            LoadFileName, SaveFileName, ReadOnly, ExplicitTypes, CryptOptions
        );
    }

    ~TConfig() {
        try {
            if ( ShouldFlushOnDestruction() ) {
                DoFlush();
            }
        }
        catch ( ... ) {
        }
    }

    TConfig( TConfig const & ) = delete;
    TConfig& operator=( TConfig const & ) = delete;
private:
    class BSONObjRAII {
    public:
        BSONObjRAII( TConfig& Cfg ) : cfg_{ Cfg } { Cfg.CreateBSONDocument(); }
        ~BSONObjRAII() {
            try { cfg_.DestroyAndCloseBSONDocument(); } catch ( ... ) {}
        }
        BSONObjRAII( BSONObjRAII const & ) = delete;
        BSONObjRAII& operator=( BSONObjRAII const & ) = delete;
    private:
        TConfig& cfg_;
    };

    std::unique_ptr<TJSONValue> document_;
    String fileName_;
    String loadFileName_;
    bool explicitTypes_;
    Crypt::TOptions cryptOptions_;

    std::unique_ptr<TStream> OpenReadStream( String const & FileName ) const {
        if ( cryptOptions_.Enabled ) {
            auto Bytes = Crypt::LoadBytes( FileName, cryptOptions_ );
            TBytes BytesArray;
            BytesArray.Length = static_cast<int>( Bytes.size() );
            if ( !Bytes.empty() ) {
                std::copy( Bytes.begin(), Bytes.end(), std::begin( BytesArray ) );
            }
            return std::make_unique<TBytesStream>( BytesArray );
        }
        return std::make_unique<TFileStream>(
            FileName, fmOpenRead | fmShareDenyWrite
        );
    }

    void SaveBSONStream( std::unique_ptr<TStream> Stream ) const {
        if ( cryptOptions_.Enabled ) {
            auto MemoryStream = dynamic_cast<TMemoryStream*>( Stream.get() );
            Crypt::Bytes Bytes( static_cast<size_t>( MemoryStream->Size ) );
            MemoryStream->Position = 0;
            if ( !Bytes.empty() ) {
                MemoryStream->ReadBuffer(
                    Bytes.data(), static_cast<NativeInt>( Bytes.size() )
                );
            }
            Crypt::SaveBytes( fileName_, Bytes, cryptOptions_ );
        }
    }

    void CreateBSONDocument() {
        document_.reset();
        if ( TFile::Exists( loadFileName_ ) ) {
            auto Stream = OpenReadStream( loadFileName_ );
            auto Reader =
                std::make_unique<System::Json::Bson::TBsonReader>(
                    Stream.get(), false
                );
            auto StringWriter = std::make_unique<TStringWriter>();
            auto Writer =
                std::make_unique<System::Json::Writers::TJsonTextWriter>(
                    StringWriter.get(), false
                );
            if ( Reader->Read() ) {
                Writer->WriteToken( Reader.get(), true );
                Writer->Flush();
                document_.reset(
                    TJSONObject::ParseJSONValue( StringWriter->ToString() )
                );
            }
        }
        if ( !document_ ) {
            document_.reset( new TJSONObject{} );
        }
    }

    void DestroyAndCloseBSONDocument() {
        document_.reset();
    }

    TJSONObject* OpenPath( TConfigPath const & Path ) {
        if ( auto Node = dynamic_cast<TJSONObject*>( document_.get() ) ) {
            for ( auto const & NodeName : Path ) {
                if ( auto NodesInner = Node->FindValue( NodesNodeName ) ) {
                    if ( auto Inner = NodesInner->FindValue( NodeName ) ) {
                        if ( auto NodeObj = dynamic_cast<TJSONObject*>( Inner ) ) {
                            Node = NodeObj;
                            continue;
                        }
                    }
                }
                return nullptr;
            }
            return Node;
        }
        return nullptr;
    }

    TJSONObject* OpenNodes( TConfigPath const & Path ) {
        if ( auto Node = OpenPath( Path ) ) {
            if ( auto Inner = Node->FindValue( NodesNodeName ) ) {
                return dynamic_cast<TJSONObject*>( Inner );
            }
        }
        return nullptr;
    }

    TJSONObject* OpenValues( TConfigPath const & Path ) {
        if ( auto Node = OpenPath( Path ) ) {
            if ( auto Inner = Node->FindValue( ValuesNodeName ) ) {
                return dynamic_cast<TJSONObject*>( Inner );
            }
        }
        return nullptr;
    }

    TJSONObject* ForcePath( TConfigPath const & Path ) {
        if ( auto Node = dynamic_cast<TJSONObject*>( document_.get() ) ) {
            TConfigPath FPath;
            FPath.reserve( Path.size() * 2 );
            for ( auto const & NodeName : Path ) {
                FPath.push_back( NodesNodeName );
                FPath.push_back( NodeName );
            }
            for ( auto const & NodeName : FPath ) {
                if ( auto Inner = Node->FindValue( NodeName ) ) {
                    if ( auto ANode = dynamic_cast<TJSONObject*>( Inner ) ) {
                        Node = ANode;
                    }
                }
                else {
                    auto InnerObj = std::make_unique<TJSONObject>();
                    Node->AddPair( NodeName, InnerObj.get() );
                    Node = InnerObj.release();
                }
            }
            return Node;
        }
        return nullptr;
    }

    TJSONObject* ForceValues( TConfigPath const & Path ) {
        if ( auto Node = ForcePath( Path ) ) {
            if ( auto Inner = Node->FindValue( ValuesNodeName ) ) {
                return dynamic_cast<TJSONObject*>( Inner );
            }
            else {
                auto InnerObj = std::make_unique<TJSONObject>();
                Node->AddPair( ValuesNodeName, InnerObj.get() );
                return InnerObj.release();
            }
        }
        return nullptr;
    }

    template<typename J, typename T>
    static void Write( J& Obj, String Name, std::unique_ptr<T> Val ) {
        if ( auto Pair = Obj.Get( Name ) ) {
            Pair->JsonValue = Val.release();
        }
        else {
            Obj.AddPair( Name, Val.release() );
        }
    }

    template<typename J, typename T>
    static void Write( J& Obj, String Type, String Name, std::unique_ptr<T> Val ) {
        auto ValueObj = std::make_unique<TJSONObject>();
        ValueObj->AddPair( Type, Val.get() );
        Val.release();
        if ( auto Pair = Obj.Get( Name ) ) {
            Pair->JsonValue = ValueObj.release();
        }
        else {
            Obj.AddPair( Name, ValueObj.release() );
        }
    }

    template<typename C, typename J, typename T>
    static void WriteNumber( J& Obj, String Type, String Name, T Val ) {
        Write(
            Obj, Type, Name,
            std::make_unique<TJSONNumber>( static_cast<C>( Val ) )
        );
    }

    template<typename J, typename T>
    static void WriteStrings( J& Obj, String Type, String Name, T&& Val ) {
        auto Array = std::make_unique<TJSONArray>();
        for ( auto Item : Val ) {
            Array->Add( Item );
        }
        auto ValueObj = std::make_unique<TJSONObject>();
        ValueObj->AddPair( Type, Array.get() );
        Array.release();
        if ( auto Pair = Obj.Get( Name ) ) {
            Pair->JsonValue = ValueObj.release();
        }
        else {
            Obj.AddPair( Name, ValueObj.release() );
        }
    }

    template<class...>
    static constexpr bool always_false_v = false;

    template<class... Ts>
    struct overload : Ts...
    {
      using Ts::operator()...;

      template<typename T>
      constexpr void operator()(T) const
      {
        static_assert(always_false_v<T>, "Unsupported type");
      }
    };

    template<class... Ts>
    overload(Ts...) -> overload<Ts...>;

    std::unique_ptr<TBase64Encoding> base64_ { new TBase64Encoding{ 0 } };

    void SaveValue( TJSONObject& Obj, ValueContType::value_type const & v ) {
#if defined( ANAFESTICA_USE_STD_VARIANT )
        std::visit(
#else
        boost::apply_visitor(
#endif
            overload {
                [this, &Obj, &v]( int Val ) {
                    if ( explicitTypes_ ) {
                        WriteNumber<int>( Obj, ana_cnv_xstr( ANA_TT_I ), v.first, Val );
                    }
                    else {
                        Write(
                            Obj, v.first,
                            std::make_unique<TJSONNumber>( Val )
                        );
                    }
                },

                [&Obj, &v]( unsigned int Val ) {
                    WriteNumber<__int64>( Obj, ana_cnv_xstr( ANA_TT_U ), v.first, Val );
                },

                [&Obj, &v]( long Val ) {
                    WriteNumber<__int64>( Obj, ana_cnv_xstr( ANA_TT_L ), v.first, Val );
                },

                [&Obj, &v]( unsigned long Val ) {
                    WriteNumber<__int64>( Obj, ana_cnv_xstr( ANA_TT_UL ), v.first, Val );
                },

                [&Obj, &v]( char Val ) {
                    WriteNumber<int>( Obj, ana_cnv_xstr( ANA_TT_C ), v.first, Val );
                },

                [&Obj, &v]( unsigned char Val ) {
                    WriteNumber<int>( Obj, ana_cnv_xstr( ANA_TT_UC ), v.first, Val );
                },

                [&Obj, &v]( short Val ) {
                    WriteNumber<int>( Obj, ana_cnv_xstr( ANA_TT_S ), v.first, Val );
                },

                [&Obj, &v]( unsigned short Val ) {
                    WriteNumber<int>( Obj, ana_cnv_xstr( ANA_TT_US ), v.first, Val );
                },

                [&Obj, &v]( long long Val ) {
                    WriteNumber<__int64>( Obj, ana_cnv_xstr( ANA_TT_LL ), v.first, Val );
                },

                [&Obj, &v]( unsigned long long Val ) {
                    Write(
                        Obj, ana_cnv_xstr( ANA_TT_ULL ), v.first,
                        std::make_unique<TJSONString>(
#if defined( _UNICODE )
                            std::to_wstring( Val ).c_str()
#else
                            std::to_string( Val ).c_str()
#endif
                        )
                    );
                },

                [this, &Obj, &v]( bool Val ) {
                    if ( explicitTypes_ ) {
                        Write(
                            Obj, ana_cnv_xstr( ANA_TT_B ), v.first,
                            std::make_unique<TJSONBool>( Val )
                        );
                    }
                    else {
                        Write(
                            Obj, v.first,
                            std::make_unique<TJSONBool>( Val )
                        );
                    }
                },

                [this, &Obj, &v]( System::UnicodeString Val ) {
                    if ( explicitTypes_ ) {
                        Write(
                            Obj, ana_cnv_xstr( ANA_TT_SZ ), v.first,
                            std::make_unique<TJSONString>( Val )
                        );
                    }
                    else {
                        Write(
                            Obj, v.first,
                            std::make_unique<TJSONString>( Val )
                        );
                    }
                },

                [&Obj, &v]( System::TDateTime Val ) {
                    Write(
                        Obj, ana_cnv_xstr( ANA_TT_DT ), v.first,
                        std::make_unique<TJSONString>(
                            DateToISO8601( Val, false )
                        )
                    );
                },

                [&Obj, &v]( float Val ) {
                    WriteNumber<double>( Obj, ana_cnv_xstr( ANA_TT_FLT ), v.first, Val );
                },

                [&Obj, &v]( double Val ) {
                    WriteNumber<double>( Obj, ana_cnv_xstr( ANA_TT_DBL ), v.first, Val );
                },

                [&Obj, &v]( System::Currency Val ) {
                    TFormatSettings FS;
                    FS.DecimalSeparator = _D( '.' );
                    Write(
                        Obj, ana_cnv_xstr( ANA_TT_CUR ), v.first,
                        std::make_unique<TJSONString>( CurrToStr( Val, FS ) )
                    );
                },

                [&Obj, &v]( StringCont const & Val ) {
                    WriteStrings( Obj, ana_cnv_xstr( ANA_TT_SV ), v.first, Val );
                },

                [this, &Obj, &v]( TBytes Val ) {
                    Write(
                        Obj, ana_cnv_xstr( ANA_TT_DAB ), v.first,
                        std::make_unique<TJSONString>(
                            Val.Length == 0 ?
                              String()
                            :
                              base64_->EncodeBytesToString(
                                  &Val[0], Val.High
                              )
                        )
                    );
                },

                [this, &Obj, &v]( std::vector<Byte> const & Val ) {
                    Write(
                        Obj, ana_cnv_xstr( ANA_TT_VB ), v.first,
                        std::make_unique<TJSONString>(
                            Val.empty() ?
                              String()
                            :
                              base64_->EncodeBytesToString(
                                  Val.data(), Val.size() - 1
                              )
                        )
                    );
                },

#if defined( ANAFESTICA_USE_STD_VARIANT )
                [&Obj, &v]( std::string const & Val ) {
                    Write(
                        Obj, ana_cnv_xstr( ANA_TT_STR ), v.first,
                        std::make_unique<TJSONString>(
                            UTF8ToString( Val.c_str() )
                        )
                    );
                },

                [&Obj, &v]( std::wstring const & Val ) {
                    Write(
                        Obj, ana_cnv_xstr( ANA_TT_WSTR ), v.first,
                        std::make_unique<TJSONString>(
                            String( Val.c_str() )
                        )
                    );
                }
#endif
            },
            v.second.first
        );
    }

protected:
    virtual ValueContType DoCreateValueList( TConfigPath const & Path ) override {

        using Fn = std::function<TConfigNodeValueType(TJSONValue&)>;

        static std::array<Fn,
#if defined( ANAFESTICA_USE_STD_VARIANT )
            std::variant_size<TConfigNodeValueType>::value
#else
            TConfigNodeValueType::types::size::value
#endif
        > Builders {
            []( TJSONValue& Value ) { return Value.GetValue<int>(); },
            []( TJSONValue& Value ) { return Value.GetValue<unsigned>(); },
            []( TJSONValue& Value ) { return static_cast<long>( Value.GetValue<__int64>() ); },
            []( TJSONValue& Value ) { return static_cast<unsigned long>( Value.GetValue<__int64>() ); },
            []( TJSONValue& Value ) { return static_cast<char>( Value.GetValue<int>() ); },
            []( TJSONValue& Value ) { return Value.GetValue<unsigned char>(); },
            []( TJSONValue& Value ) { return Value.GetValue<short>(); },
            []( TJSONValue& Value ) { return Value.GetValue<unsigned short>(); },
            []( TJSONValue& Value ) { return Value.GetValue<__int64>(); },
            []( TJSONValue& Value ) { return std::stoull( Value.GetValue<String>().c_str() ); },
            []( TJSONValue& Value ) { return Value.GetValue<bool>(); },
            []( TJSONValue& Value ) { return Value.GetValue<String>(); },
            []( TJSONValue& Value ) { return ISO8601ToDate( Value.GetValue<String>(), false ); },
            []( TJSONValue& Value ) { return Value.GetValue<float>(); },
            []( TJSONValue& Value ) { return Value.GetValue<double>(); },
            []( TJSONValue& Value ) {
                TFormatSettings FS;
                FS.DecimalSeparator = _D( '.' );
                return StrToCurr( Value.GetValue<String>(), FS );
            },
            []( TJSONValue& Value ) {
                StringCont Strings;
                if ( auto JSONArr = dynamic_cast<TJSONArray*>( &Value ) ) {
                    Strings.reserve( JSONArr->Count );
                    std::transform(
                        System::begin( JSONArr ), System::end( JSONArr ),
                        std::back_inserter( Strings ),
                        []( auto Val ){ return Val->template GetValue<String>(); }
                    );
                }
                return Strings;
            },
            []( TJSONValue& Value ) {
                return
                    TNetEncoding::Base64->DecodeStringToBytes(
                        Value.GetValue<String>()
                    );
            },
            []( TJSONValue& Value ) {
                auto Bytes = TNetEncoding::Base64->DecodeStringToBytes(
                    Value.GetValue<String>()
                );
                std::vector<Byte> VBytes;
                VBytes.reserve( Bytes.Length );
                std::copy(
                    std::begin( Bytes ),
                    std::end( Bytes ),
                    std::back_inserter( VBytes )
                );
                return VBytes;
            },
#if defined( ANAFESTICA_USE_STD_VARIANT )
            []( TJSONValue& Value ) {
                return std::string(
                    UTF8Encode( Value.GetValue<String>() ).c_str()
                );
            },
            []( TJSONValue& Value ) {
                auto s = Value.GetValue<String>();
                return std::wstring( s.c_str() );
            },
#endif
        };

        ValueContType Values;

        if ( auto Key = OpenValues( Path ) ) {
            for ( int Idx = 0 ; Idx < Key->Count ; ++Idx ) {
                auto const & Pair = Key->Pairs[Idx];
                if ( dynamic_cast<TJSONObject*>( Pair->JsonValue ) ) {
                    if ( auto InnerObj = dynamic_cast<TJSONObject*>( Pair->JsonValue ) ) {
                        if ( InnerObj->Count == 1 ) {
                            auto const & InnerPair = InnerObj->Pairs[0];
                            auto TypeName = InnerPair->JsonString->Value();
                            if ( auto Result = GetTypeTag( TypeName ) ) {
                                PutItemTo(
                                    Values, Pair->JsonString->Value(),
                                    {
                                        Builders[static_cast<size_t>( Result.value() )](
                                            *InnerPair->JsonValue
                                        ),
                                        Operation::None
                                    }
                                );
                            }
                        }
                    }
                }
                else if ( auto Ptr = dynamic_cast<TJSONNumber*>( Pair->JsonValue ) ) {
                    PutItemTo(
                        Values, Pair->JsonString->Value(),
                        {
                            TConfigNodeValueType{ Ptr->AsInt },
                            Operation::None
                        }
                    );
                }
                else if ( auto Ptr = dynamic_cast<TJSONString*>( Pair->JsonValue ) ) {
                    PutItemTo(
                        Values, Pair->JsonString->Value(),
                        {
                            TConfigNodeValueType{ Ptr->Value() },
                            Operation::None
                        }
                    );
                }
                else if ( auto Ptr = dynamic_cast<TJSONBool*>( Pair->JsonValue ) ) {
                    PutItemTo(
                        Values, Pair->JsonString->Value(),
                        {
                            TConfigNodeValueType{ Ptr->AsBoolean },
                            Operation::None
                        }
                    );
                }
            }
        }
        return Values;
    }

    virtual NodeContType DoCreateNodeList( TConfigPath const & Path ) override {
        NodeContType Nodes;

        if ( auto Node = OpenNodes( Path ) ) {
            for ( int Idx = 0 ; Idx < Node->Count ; ++Idx ) {
                auto const & Pair = Node->Pairs[Idx];
                if ( dynamic_cast<TJSONObject*>( Pair->JsonValue ) ) {
                    auto NodeName = Pair->JsonString->Value();
                    Nodes[NodeName] = std::move( std::make_unique<TConfigNode>() );
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
                        SaveValue( *Node, v );
                    }
                    else if ( v.second.second == Operation::Erase ) {
                        Node->RemovePair( v.first );
                    }
                }
            }
        }
    }

    virtual void DoDeleteNode( TConfigPath const & Path ) override {
        if ( !Path.empty() ) {
            auto Last = std::end( Path );
            std::advance( Last, -1 );
            TConfigPath TmpPath( std::begin( Path ), Last );
            if ( auto Node = OpenNodes( TmpPath ) ) {
                Node->RemovePair( Path.back() );
            }
        }
    }

    virtual void DoFlush() override {
        BSONObjRAII BSON{ *this };
        GetRootNode().Write( *this, TConfigPath{} );

        if ( !TFile::Exists( fileName_ ) ) {
            auto Path = TPath::GetFullPath( TPath::GetDirectoryName( fileName_ ) );
            if ( !TDirectory::Exists( Path ) ) {
                TDirectory::CreateDirectory( Path );
            }
        }

        std::unique_ptr<TStream> Stream =
            cryptOptions_.Enabled
                ? std::unique_ptr<TStream>( new TMemoryStream() )
                : std::unique_ptr<TStream>( new TFileStream( fileName_, fmCreate ) );
        auto StringReader =
            std::make_unique<TStringReader>( document_->ToJSON() );
        auto Writer =
            std::make_unique<System::Json::Bson::TBsonWriter>( Stream.get() );
        auto Reader =
            std::make_unique<System::Json::Readers::TJsonTextReader>(
                StringReader.get()
            );
        if ( Reader->Read() ) {
            Writer->WriteToken( Reader.get(), true );
        }
        Writer->Flush();
        SaveBSONStream( std::move( Stream ) );
    }
};

//---------------------------------------------------------------------------
} // End namespace BSON
//---------------------------------------------------------------------------
} // End of namespace Anafestica
//---------------------------------------------------------------------------

#endif
