//---------------------------------------------------------------------------
//
// YAML backend for Anafestica, built on top of the header-only fkYAML library
// (https://github.com/fktn-k/fkYAML). The library must be reachable via the
// compiler include path; fkYAML is NOT redistributed with Anafestica and
// nothing else in the project depends on it. Users that do not include this
// header pay no compile- or link-time cost.
//
// The on-disk schema mirrors CfgJSON.h (nodes: / values: with optional
// explicit type tags) so that a YAML file can be inspected and round-tripped
// against the same TConfigNode tree.
//
// fkYAML does not expose an erase operation on its node API. To support
// deletion, this backend rebuilds the YAML document from the in-memory
// TConfigNode tree on every flush; for that reason the FlushAllItems flag
// is forced to true regardless of what the caller passes (the parameter is
// kept for API parity with the other file-based backends).
//
//---------------------------------------------------------------------------

#ifndef CfgYAMLH
#define CfgYAMLH

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
#include <string>
#include <utility>
#include <cstdio>

#include <fkYAML/node.hpp>

#include <anafestica/Cfg.h>
#include <anafestica/CfgCrypt.h>

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------
namespace YAML {
//---------------------------------------------------------------------------

class TConfig : public Anafestica::TConfig {
private:
    using YamlNode = ::fkyaml::node;

    static constexpr char const * NodesKeyU8  = "nodes";
    static constexpr char const * ValuesKeyU8 = "values";

public:
    TConfig( String FileName, bool ReadOnly = false,
             bool /*FlushAllItems*/ = false, bool ExplicitTypes = false,
             Crypt::TOptions CryptOptions = {} )
        : Anafestica::TConfig( ReadOnly, /*FlushAllItems*/ true )
        , fileName_{ FileName }, loadFileName_{ FileName }
        , explicitTypes_{ ExplicitTypes }
        , cryptOptions_{ CryptOptions }
    {
        if ( TFile::Exists( loadFileName_ ) ) {
            YAMLObjRAII YAML{ *this, /*Load*/true };
            GetRootNode().Read( *this, TConfigPath{} );
        }
    }

    /// Migration constructor: loads from @p LoadFileName, persists to
    /// @p SaveFileName.  Destination wins when both exist.  FlushAllItems
    /// is already always-on for YAML (see header note).  See @ref Migrate
    /// for a named wrapper that makes the load/save direction unambiguous.
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
            YAMLObjRAII YAML{ *this, /*Load*/true };
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
    class YAMLObjRAII {
    public:
        YAMLObjRAII( TConfig& Cfg, bool Load )
            : cfg_{ Cfg }
        {
            Cfg.CreateYAMLDocument( Load );
        }
        ~YAMLObjRAII() {
            try { cfg_.DestroyAndCloseYAMLDocument(); } catch ( ... ) {}
        }
        YAMLObjRAII( YAMLObjRAII const & ) = delete;
        YAMLObjRAII& operator=( YAMLObjRAII const & ) = delete;
    private:
        TConfig& cfg_;
    };

    std::unique_ptr<YamlNode>          document_;
    String                             fileName_;
    String                             loadFileName_;
    bool                               explicitTypes_;
    Crypt::TOptions                    cryptOptions_;
    std::unique_ptr<TBase64Encoding>   base64_ { new TBase64Encoding{ 0 } };

    //-----------------------------------------------------------------------
    // Encoding helpers — fkYAML uses UTF-8 std::string internally; the
    // Embarcadero RTL uses UTF-16 System::String.
    //-----------------------------------------------------------------------
    static std::string ToUtf8( String const & S ) {
        AnsiString Tmp = UTF8Encode( S );
        return std::string( Tmp.c_str(), static_cast<size_t>( Tmp.Length() ) );
    }

    static String FromUtf8( std::string const & S ) {
        return S.empty() ? String() : UTF8ToString( S.c_str() );
    }

    std::string ReadFileBytes( String const & FileName ) const {
        if ( cryptOptions_.Enabled ) {
            auto Bytes = Crypt::LoadBytes( FileName, cryptOptions_ );
            return std::string( Bytes.begin(), Bytes.end() );
        }

        std::string Content;
        std::unique_ptr<TFileStream> Stream(
            new TFileStream(
                FileName, fmOpenRead | fmShareDenyWrite
            )
        );
        auto const Size = static_cast<size_t>( Stream->Size );
        if ( Size > 0 ) {
            Content.resize( Size );
            Stream->ReadBuffer( &Content[0], static_cast<NativeInt>( Size ) );
        }
        return Content;
    }

    void WriteFileBytes( String const & FileName, std::string const & Content ) const {
        Crypt::Bytes Bytes( Content.begin(), Content.end() );
        if ( cryptOptions_.Enabled ) {
            Crypt::SaveBytes( FileName, Bytes, cryptOptions_ );
        }
        else {
            std::unique_ptr<TFileStream> Stream(
                new TFileStream( FileName, fmCreate )
            );
            if ( !Bytes.empty() ) {
                Stream->WriteBuffer(
                    Bytes.data(), static_cast<NativeInt>( Bytes.size() )
                );
            }
        }
    }

    //-----------------------------------------------------------------------
    // Document lifecycle.
    //-----------------------------------------------------------------------
    void CreateYAMLDocument( bool Load ) {
        document_.reset();
        if ( Load && TFile::Exists( loadFileName_ ) ) {
            std::string Content = ReadFileBytes( loadFileName_ );
            try {
                if ( !Content.empty() ) {
                    document_.reset(
                        new YamlNode( YamlNode::deserialize( Content ) )
                    );
                }
            }
            catch ( ... ) {
                // Malformed YAML — fall through and start with an empty doc.
                document_.reset();
            }
        }
        if ( !document_ || !document_->is_mapping() ) {
            document_.reset( new YamlNode( ::fkyaml::node_type::MAPPING ) );
        }
    }

    void DestroyAndCloseYAMLDocument() {
        document_.reset();
    }

    //-----------------------------------------------------------------------
    // Path traversal helpers — analogous to OpenPath/ForcePath in CfgJSON.
    // Read paths return nullptr on a missing/wrong-typed branch; force paths
    // create the necessary intermediate mappings.
    //-----------------------------------------------------------------------
    YamlNode* OpenPath( TConfigPath const & Path ) {
        if ( !document_ || !document_->is_mapping() ) return nullptr;
        YamlNode* Cur = document_.get();
        for ( auto const & NodeName : Path ) {
            if ( !Cur->contains( NodesKeyU8 ) ) return nullptr;
            YamlNode& Inner = (*Cur)[NodesKeyU8];
            if ( !Inner.is_mapping() ) return nullptr;
            std::string const SubKey = ToUtf8( NodeName );
            if ( !Inner.contains( SubKey ) ) return nullptr;
            YamlNode& SubNode = Inner[SubKey];
            if ( !SubNode.is_mapping() ) return nullptr;
            Cur = &SubNode;
        }
        return Cur;
    }

    YamlNode* OpenSection( TConfigPath const & Path, char const * Section ) {
        if ( auto Node = OpenPath( Path ) ) {
            if ( Node->contains( Section ) ) {
                YamlNode& Sect = (*Node)[Section];
                if ( Sect.is_mapping() ) return &Sect;
            }
        }
        return nullptr;
    }

    YamlNode* OpenNodes( TConfigPath const & Path ) {
        return OpenSection( Path, NodesKeyU8 );
    }

    YamlNode* OpenValues( TConfigPath const & Path ) {
        return OpenSection( Path, ValuesKeyU8 );
    }

    YamlNode* ForcePath( TConfigPath const & Path ) {
        if ( !document_ ) {
            document_.reset( new YamlNode( ::fkyaml::node_type::MAPPING ) );
        }
        if ( !document_->is_mapping() ) {
            *document_ = YamlNode( ::fkyaml::node_type::MAPPING );
        }
        YamlNode* Cur = document_.get();
        for ( auto const & NodeName : Path ) {
            if ( !Cur->contains( NodesKeyU8 ) || !(*Cur)[NodesKeyU8].is_mapping() ) {
                (*Cur)[NodesKeyU8] = YamlNode( ::fkyaml::node_type::MAPPING );
            }
            YamlNode& Inner = (*Cur)[NodesKeyU8];
            std::string const SubKey = ToUtf8( NodeName );
            if ( !Inner.contains( SubKey ) || !Inner[SubKey].is_mapping() ) {
                Inner[SubKey] = YamlNode( ::fkyaml::node_type::MAPPING );
            }
            Cur = &Inner[SubKey];
        }
        return Cur;
    }

    YamlNode* ForceValues( TConfigPath const & Path ) {
        if ( auto Node = ForcePath( Path ) ) {
            if ( !Node->contains( ValuesKeyU8 ) || !(*Node)[ValuesKeyU8].is_mapping() ) {
                (*Node)[ValuesKeyU8] = YamlNode( ::fkyaml::node_type::MAPPING );
            }
            return &(*Node)[ValuesKeyU8];
        }
        return nullptr;
    }

    //-----------------------------------------------------------------------
    // Tagged-value helper: produces a single-pair mapping {Type: Inner}.
    //-----------------------------------------------------------------------
    static YamlNode WrapTagged( char const * Type, YamlNode Inner ) {
        YamlNode Wrapper( ::fkyaml::node_type::MAPPING );
        Wrapper[Type] = std::move( Inner );
        return Wrapper;
    }

    //-----------------------------------------------------------------------
    // Variant overload visitor (same pattern as CfgJSON / CfgBSON).
    //-----------------------------------------------------------------------
    template<class...>
    static constexpr bool always_false_v = false;

    template<class... Ts>
    struct overload : Ts...
    {
        using Ts::operator()...;
        template<typename T>
        constexpr void operator()(T) const {
            static_assert( always_false_v<T>, "Unsupported type" );
        }
    };
    template<class... Ts> overload(Ts...) -> overload<Ts...>;

    void SaveValue( YamlNode& Obj, ValueContType::value_type const & v ) {
        std::string const Key = ToUtf8( v.first );
#if defined( ANAFESTICA_USE_STD_VARIANT )
        std::visit(
#else
        boost::apply_visitor(
#endif
            overload {
                [this, &Obj, &Key]( int Val ) {
                    if ( explicitTypes_ ) {
                        Obj[Key] = WrapTagged(
                            ana_cnv_xstr( ANA_TT_I ),
                            YamlNode( static_cast<std::int64_t>( Val ) )
                        );
                    }
                    else {
                        Obj[Key] = YamlNode( static_cast<std::int64_t>( Val ) );
                    }
                },
                [&Obj, &Key]( unsigned int Val ) {
                    Obj[Key] = WrapTagged(
                        ana_cnv_xstr( ANA_TT_U ),
                        YamlNode( static_cast<std::int64_t>( Val ) )
                    );
                },
                [&Obj, &Key]( long Val ) {
                    Obj[Key] = WrapTagged(
                        ana_cnv_xstr( ANA_TT_L ),
                        YamlNode( static_cast<std::int64_t>( Val ) )
                    );
                },
                [&Obj, &Key]( unsigned long Val ) {
                    Obj[Key] = WrapTagged(
                        ana_cnv_xstr( ANA_TT_UL ),
                        YamlNode( static_cast<std::int64_t>( Val ) )
                    );
                },
                [&Obj, &Key]( char Val ) {
                    Obj[Key] = WrapTagged(
                        ana_cnv_xstr( ANA_TT_C ),
                        YamlNode( static_cast<std::int64_t>( Val ) )
                    );
                },
                [&Obj, &Key]( unsigned char Val ) {
                    Obj[Key] = WrapTagged(
                        ana_cnv_xstr( ANA_TT_UC ),
                        YamlNode( static_cast<std::int64_t>( Val ) )
                    );
                },
                [&Obj, &Key]( short Val ) {
                    Obj[Key] = WrapTagged(
                        ana_cnv_xstr( ANA_TT_S ),
                        YamlNode( static_cast<std::int64_t>( Val ) )
                    );
                },
                [&Obj, &Key]( unsigned short Val ) {
                    Obj[Key] = WrapTagged(
                        ana_cnv_xstr( ANA_TT_US ),
                        YamlNode( static_cast<std::int64_t>( Val ) )
                    );
                },
                [&Obj, &Key]( long long Val ) {
                    Obj[Key] = WrapTagged(
                        ana_cnv_xstr( ANA_TT_LL ),
                        YamlNode( static_cast<std::int64_t>( Val ) )
                    );
                },
                [&Obj, &Key]( unsigned long long Val ) {
                    // Stored as decimal string to avoid 64-bit precision loss.
                    Obj[Key] = WrapTagged(
                        ana_cnv_xstr( ANA_TT_ULL ),
                        YamlNode( std::to_string( Val ) )
                    );
                },
                [this, &Obj, &Key]( bool Val ) {
                    if ( explicitTypes_ ) {
                        Obj[Key] = WrapTagged(
                            ana_cnv_xstr( ANA_TT_B ),
                            YamlNode( Val )
                        );
                    }
                    else {
                        Obj[Key] = YamlNode( Val );
                    }
                },
                [this, &Obj, &Key]( System::UnicodeString Val ) {
                    if ( explicitTypes_ ) {
                        Obj[Key] = WrapTagged(
                            ana_cnv_xstr( ANA_TT_SZ ),
                            YamlNode( ToUtf8( Val ) )
                        );
                    }
                    else {
                        Obj[Key] = YamlNode( ToUtf8( Val ) );
                    }
                },
                [&Obj, &Key]( System::TDateTime Val ) {
                    Obj[Key] = WrapTagged(
                        ana_cnv_xstr( ANA_TT_DT ),
                        YamlNode( ToUtf8( DateToISO8601( Val, false ) ) )
                    );
                },
                [&Obj, &Key]( float Val ) {
                    // Serialised as a string: fkYAML's float emitter uses
                    // ostringstream default precision (6 digits) which is
                    // lossy. A string preserves the round-trip.
                    char Buf[64];
                    std::snprintf( Buf, sizeof Buf, "%.9g", static_cast<double>( Val ) );
                    Obj[Key] = WrapTagged(
                        ana_cnv_xstr( ANA_TT_FLT ),
                        YamlNode( std::string( Buf ) )
                    );
                },
                [&Obj, &Key]( double Val ) {
                    // %.17g is the round-trip-safe printf format for double.
                    char Buf[64];
                    std::snprintf( Buf, sizeof Buf, "%.17g", Val );
                    Obj[Key] = WrapTagged(
                        ana_cnv_xstr( ANA_TT_DBL ),
                        YamlNode( std::string( Buf ) )
                    );
                },
                [&Obj, &Key]( System::Currency Val ) {
                    TFormatSettings FS;
                    FS.DecimalSeparator = _D( '.' );
                    Obj[Key] = WrapTagged(
                        ana_cnv_xstr( ANA_TT_CUR ),
                        YamlNode( ToUtf8( CurrToStr( Val, FS ) ) )
                    );
                },
                [&Obj, &Key]( StringCont const & Val ) {
                    YamlNode::sequence_type Seq;
                    Seq.reserve( Val.size() );
                    for ( auto const & S : Val ) {
                        Seq.emplace_back( ToUtf8( S ) );
                    }
                    Obj[Key] = WrapTagged(
                        ana_cnv_xstr( ANA_TT_SV ),
                        YamlNode::sequence( std::move( Seq ) )
                    );
                },
                [this, &Obj, &Key]( System::Sysutils::TBytes Val ) {
                    String Encoded = Val.Length == 0
                        ? String()
                        : base64_->EncodeBytesToString( &Val[0], Val.High );
                    Obj[Key] = WrapTagged(
                        ana_cnv_xstr( ANA_TT_DAB ),
                        YamlNode( ToUtf8( Encoded ) )
                    );
                },
                [this, &Obj, &Key]( std::vector<Byte> const & Val ) {
                    String Encoded = Val.empty()
                        ? String()
                        : base64_->EncodeBytesToString(
                              Val.data(), Val.size() - 1
                          );
                    Obj[Key] = WrapTagged(
                        ana_cnv_xstr( ANA_TT_VB ),
                        YamlNode( ToUtf8( Encoded ) )
                    );
                }
#if defined( ANAFESTICA_USE_STD_VARIANT )
                ,
                [&Obj, &Key]( std::string const & Val ) {
                    Obj[Key] = WrapTagged(
                        ana_cnv_xstr( ANA_TT_STR ),
                        YamlNode( Val )
                    );
                },
                [&Obj, &Key]( std::wstring const & Val ) {
                    Obj[Key] = WrapTagged(
                        ana_cnv_xstr( ANA_TT_WSTR ),
                        YamlNode( ToUtf8( String( Val.c_str() ) ) )
                    );
                }
#endif
            },
            v.second.first
        );
    }

protected:
    virtual ValueContType DoCreateValueList( TConfigPath const & Path ) override {

        using Fn = std::function<TConfigNodeValueType(YamlNode const &)>;

        static std::array<Fn,
#if defined( ANAFESTICA_USE_STD_VARIANT )
            std::variant_size<TConfigNodeValueType>::value
#else
            TConfigNodeValueType::types::size::value
#endif
        > Builders {
            // TT_I
            []( YamlNode const & V ) {
                return static_cast<int>( V.get_value<std::int64_t>() );
            },
            // TT_U
            []( YamlNode const & V ) {
                return static_cast<unsigned int>( V.get_value<std::int64_t>() );
            },
            // TT_L
            []( YamlNode const & V ) {
                return static_cast<long>( V.get_value<std::int64_t>() );
            },
            // TT_UL
            []( YamlNode const & V ) {
                return static_cast<unsigned long>( V.get_value<std::int64_t>() );
            },
            // TT_C
            []( YamlNode const & V ) {
                return static_cast<char>( V.get_value<std::int64_t>() );
            },
            // TT_UC
            []( YamlNode const & V ) {
                return static_cast<unsigned char>( V.get_value<std::int64_t>() );
            },
            // TT_S
            []( YamlNode const & V ) {
                return static_cast<short>( V.get_value<std::int64_t>() );
            },
            // TT_US
            []( YamlNode const & V ) {
                return static_cast<unsigned short>( V.get_value<std::int64_t>() );
            },
            // TT_LL
            []( YamlNode const & V ) {
                return static_cast<long long>( V.get_value<std::int64_t>() );
            },
            // TT_ULL — decimal string
            []( YamlNode const & V ) {
                return std::stoull( V.get_value<std::string>() );
            },
            // TT_B
            []( YamlNode const & V ) {
                return V.get_value<bool>();
            },
            // TT_SZ
            []( YamlNode const & V ) {
                return FromUtf8( V.get_value<std::string>() );
            },
            // TT_DT
            []( YamlNode const & V ) {
                return ISO8601ToDate( FromUtf8( V.get_value<std::string>() ), false );
            },
            // TT_FLT  (string-encoded for full precision)
            []( YamlNode const & V ) {
                return std::stof( V.get_value<std::string>() );
            },
            // TT_DBL  (string-encoded for full precision)
            []( YamlNode const & V ) {
                return std::stod( V.get_value<std::string>() );
            },
            // TT_CUR
            []( YamlNode const & V ) {
                TFormatSettings FS;
                FS.DecimalSeparator = _D( '.' );
                return StrToCurr( FromUtf8( V.get_value<std::string>() ), FS );
            },
            // TT_SV
            []( YamlNode const & V ) {
                StringCont Strings;
                if ( V.is_sequence() ) {
                    Strings.reserve( V.size() );
                    for ( auto It = V.begin(); It != V.end(); ++It ) {
                        Strings.push_back( FromUtf8( It->get_value<std::string>() ) );
                    }
                }
                return Strings;
            },
            // TT_DAB
            []( YamlNode const & V ) {
                return TNetEncoding::Base64->DecodeStringToBytes(
                    FromUtf8( V.get_value<std::string>() )
                );
            },
            // TT_VB
            []( YamlNode const & V ) {
                auto Bytes = TNetEncoding::Base64->DecodeStringToBytes(
                    FromUtf8( V.get_value<std::string>() )
                );
                std::vector<Byte> VBytes;
                VBytes.reserve( Bytes.Length );
                std::copy(
                    std::begin( Bytes ), std::end( Bytes ),
                    std::back_inserter( VBytes )
                );
                return VBytes;
            },
#if defined( ANAFESTICA_USE_STD_VARIANT )
            // TT_STR
            []( YamlNode const & V ) {
                return V.get_value<std::string>();
            },
            // TT_WSTR
            []( YamlNode const & V ) {
                String Tmp = FromUtf8( V.get_value<std::string>() );
                return std::wstring( Tmp.c_str() );
            },
#endif
        };

        ValueContType Values;

        if ( auto Section = OpenValues( Path ) ) {
            for ( auto It = Section->begin() ; It != Section->end() ; ++It ) {
                String const Name = FromUtf8( It.key().get_value<std::string>() );
                YamlNode const & Val = It.value();

                bool Handled = false;

                // Tagged form: { type: data }  (single-key mapping)
                if ( Val.is_mapping() && Val.size() == 1 ) {
                    auto Inner = Val.begin();
                    String TypeName = FromUtf8( Inner.key().get_value<std::string>() );
                    if ( auto Tag = GetTypeTag( TypeName ) ) {
                        try {
                            PutItemTo(
                                Values, Name,
                                {
                                    Builders[static_cast<size_t>( Tag.value() )](
                                        Inner.value()
                                    ),
                                    Operation::None
                                }
                            );
                            Handled = true;
                        }
                        catch ( ... ) {
                            // Builder failed for this entry — leave it out.
                            Handled = true;
                        }
                    }
                }

                if ( Handled ) continue;

                // Implicit form: scalar value with no type tag.
                try {
                    if ( Val.is_boolean() ) {
                        PutItemTo(
                            Values, Name,
                            {
                                TConfigNodeValueType{ Val.get_value<bool>() },
                                Operation::None
                            }
                        );
                    }
                    else if ( Val.is_integer() ) {
                        PutItemTo(
                            Values, Name,
                            {
                                TConfigNodeValueType{
                                    static_cast<int>( Val.get_value<std::int64_t>() )
                                },
                                Operation::None
                            }
                        );
                    }
                    else if ( Val.is_string() ) {
                        PutItemTo(
                            Values, Name,
                            {
                                TConfigNodeValueType{
                                    FromUtf8( Val.get_value<std::string>() )
                                },
                                Operation::None
                            }
                        );
                    }
                }
                catch ( ... ) {
                    // Skip malformed entry.
                }
            }
        }
        return Values;
    }

    virtual NodeContType DoCreateNodeList( TConfigPath const & Path ) override {
        NodeContType Nodes;
        if ( auto NodesMap = OpenNodes( Path ) ) {
            for ( auto It = NodesMap->begin() ; It != NodesMap->end() ; ++It ) {
                if ( It.value().is_mapping() ) {
                    String const NodeName =
                        FromUtf8( It.key().get_value<std::string>() );
                    Nodes[NodeName] = std::make_unique<TConfigNode>();
                }
            }
        }
        return Nodes;
    }

    virtual void DoSaveValueList( TConfigPath const & Path,
                                  ValueContType const & Values ) override {
        bool HasAny = false;
        for ( auto const & v : Values ) {
            if ( v.second.second != Operation::Erase ) { HasAny = true; break; }
        }
        if ( !HasAny ) return;

        if ( auto Node = ForceValues( Path ) ) {
            for ( auto const & v : Values ) {
                if ( v.second.second != Operation::Erase ) {
                    SaveValue( *Node, v );
                }
            }
        }
    }

    // Deletion is implicit: DoFlush rebuilds the document from scratch, so
    // any TConfigNode that has been Cleared is simply not re-emitted.
    virtual void DoDeleteNode( TConfigPath const & /*Path*/ ) override {
    }

    virtual void DoFlush() override {
        // Build a fresh YAML document from the current in-memory tree.
        YAMLObjRAII YAML{ *this, /*Load*/false };
        GetRootNode().Write( *this, TConfigPath{} );

        if ( !TFile::Exists( fileName_ ) ) {
            auto Path = TPath::GetFullPath( TPath::GetDirectoryName( fileName_ ) );
            if ( !TDirectory::Exists( Path ) ) {
                TDirectory::CreateDirectory( Path );
            }
        }

        std::string Yaml = YamlNode::serialize( *document_ );
        WriteFileBytes( fileName_, Yaml );
    }

    virtual bool DoGetForcedWritesFlag() const {
        return false;
    }
};

//---------------------------------------------------------------------------
} // End of namespace YAML
//---------------------------------------------------------------------------
} // End of namespace Anafestica
//---------------------------------------------------------------------------

#endif
