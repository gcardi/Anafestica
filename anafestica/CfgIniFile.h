//---------------------------------------------------------------------------

#ifndef CfgIniFileH
#define CfgIniFileH

// NOTE: TMemIniFile (System.IniFiles) is used here, NOT TRegIniFile.
// TRegIniFile stores data in the Windows Registry using an INI-like API –
// that role is already covered by Anafestica::Registry::TConfig.
// TMemIniFile reads the whole file into memory on construction and writes
// it back atomically via UpdateFile(), matching the RAII lifecycle used by
// the XML and JSON backends.

#include <System.IniFiles.hpp>
#include <System.IOUtils.hpp>
#include <System.NetEncoding.hpp>
#include <System.DateUtils.hpp>
#include <System.SysUtils.hpp>
#include <System.Classes.hpp>

#include <memory>
#include <vector>
#include <array>
#include <functional>
#include <string>

#include <anafestica/Cfg.h>
#include <anafestica/CfgCrypt.h>

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------
namespace INIFile {
//---------------------------------------------------------------------------

// Storage layout
// --------------
// Each TConfigPath maps to an INI section whose name is the path components
// joined by '\':
//
//   TConfigPath{}          -> [config]
//   TConfigPath{"A"}       -> [config\A]
//   TConfigPath{"A","B"}   -> [config\A\B]
//
// Every value key is encoded as  Name::(TypeTag)  so the type can be
// recovered unambiguously on read (all INI values are plain strings):
//
//   [config]
//   port::(i)=5432
//   host::(sz)=localhost
//   debug::(b)=True
//
// StringCont is encoded as items joined by '|', with '\' → '\\' and
// '|' → '\|' escaping.  Binary types (TBytes, BytesCont) are base-64.
// The file is written in UTF-8 encoding.

class TConfig : public Anafestica::TConfig {
public:
    TConfig( String FileName, bool ReadOnly = false, bool FlushAllItems = false,
             Crypt::TOptions CryptOptions = {} )
        : Anafestica::TConfig( ReadOnly, FlushAllItems )
        , fileName_( FileName ), loadFileName_( FileName )
        , cryptOptions_( CryptOptions )
    {
        if ( TFile::Exists( loadFileName_ ) ) {
            IniFileRAII Ini( *this, loadFileName_ );
            GetRootNode().Read( *this, TConfigPath{} );
        }
    }

    /// Migration constructor: loads from @p LoadFileName, persists to
    /// @p SaveFileName.  Destination wins when both exist.  FlushAllItems
    /// is forced @c true so that values in @c Operation::None state are
    /// still written to the destination.  See @ref Migrate for a named
    /// wrapper that makes the load/save direction unambiguous.
    TConfig( String LoadFileName, String SaveFileName, bool ReadOnly = false,
             Crypt::TOptions CryptOptions = {} )
        : Anafestica::TConfig( ReadOnly, /*FlushAllItems*/ true )
        , fileName_( SaveFileName )
        , loadFileName_(
            TFile::Exists( SaveFileName ) ? SaveFileName : LoadFileName
          )
        , cryptOptions_( CryptOptions )
    {
        if ( TFile::Exists( loadFileName_ ) ) {
            IniFileRAII Ini( *this, loadFileName_ );
            GetRootNode().Read( *this, TConfigPath{} );
            if ( loadFileName_ != fileName_ ) {
                MarkForFlush();
            }
        }
    }

    static TConfig Migrate( String LoadFileName, String SaveFileName,
                            bool ReadOnly = false,
                            Crypt::TOptions CryptOptions = {} )
    {
        return TConfig( LoadFileName, SaveFileName, ReadOnly, CryptOptions );
    }

    ~TConfig() {
        try {
            if ( ShouldFlushOnDestruction() ) {
                DoFlush();
            }
        }
        catch ( ... ) {}
    }

    TConfig( TConfig const & ) = delete;
    TConfig& operator=( TConfig const & ) = delete;

private:
    // -----------------------------------------------------------------------
    // RAII wrapper that opens/closes the TMemIniFile around each operation
    // -----------------------------------------------------------------------
    class IniFileRAII {
    public:
        IniFileRAII( TConfig& Cfg, String FilePath )
            : cfg_( Cfg )
        {
            Cfg.CreateIniObject( FilePath );
        }
        ~IniFileRAII() { try { cfg_.DestroyIniObject(); } catch ( ... ) {} }
        IniFileRAII( IniFileRAII const & ) = delete;
        IniFileRAII& operator=( IniFileRAII const & ) = delete;
    private:
        TConfig& cfg_;
    };

    std::unique_ptr<TMemIniFile> ini_;
    String fileName_;
    String loadFileName_;
    Crypt::TOptions cryptOptions_;

    static void ValidatePathComponent( String const & Component ) {
        if ( Component.Pos( _D( "\\" ) ) > 0 ||
             Component.Pos( _D( "/" ) ) > 0 )
        {
            throw Exception(
                Format(
                    _D( "Invalid INI path component: '%s'" ),
                    ARRAYOFCONST(( Component ))
                )
            );
        }
    }

    // Opens (and implicitly loads) the underlying TMemIniFile against the
    // given path.  Constructors pass loadFileName_; DoFlush passes
    // fileName_ — TMemIniFile::UpdateFile writes back to its construction
    // path, so this is how the load/save split is realised.
    void CreateIniObject( String FilePath ) {
        // Specify UTF-8 so that Unicode strings survive the disk roundtrip.
        if ( cryptOptions_.Enabled ) {
            ini_ = std::make_unique<TMemIniFile>( String{}, TEncoding::UTF8 );
            if ( TFile::Exists( FilePath ) ) {
                auto SL = std::make_unique<TStringList>();
                SL->Text = Crypt::LoadText( FilePath, TEncoding::UTF8, cryptOptions_ );
                ini_->SetStrings( SL.get() );
            }
        }
        else {
            ini_ = std::make_unique<TMemIniFile>( FilePath, TEncoding::UTF8 );
        }
    }

    void DestroyIniObject() {
        ini_.reset();
    }

    // -----------------------------------------------------------------------
    // Section / key encoding helpers
    // -----------------------------------------------------------------------

    // Build the INI section name from a TConfigPath.
    // {}           -> "config"
    // {"A","B"}    -> "config\A\B"
    String GetSectionName( TConfigPath const & Path ) const {
        String Result( _D("config") );
        for ( auto const & Component : Path ) {
            ValidatePathComponent( Component );
            Result += _D("\\");
            Result += Component;
        }
        return Result;
    }

    // Encode a value key as  "Name::(TypeTag)"
    static String EncodeKey( String const & Name, String const & Tag ) {
        return Name + _D("::(") + Tag + _D(")");
    }

    // Find the 1-based position of the first ':' in the LAST "::(" pattern
    // in Key, searching from the right.  Returns 0 if not found.
    static int FindTypeSepPos( String const & Key ) {
        for ( int i = Key.Length() - 2; i >= 1; --i ) {
            if ( Key[i] == _D(':') && Key[i + 1] == _D(':') && Key[i + 2] == _D('(') ) {
                return i;
            }
        }
        return 0;
    }

    // Decode "Name::(TypeTag)" → Name and Tag strings.
    // Returns false when no valid type suffix is found.
    static bool DecodeKey( String const & Key, String & Name, String & Tag ) {
        if ( Key.IsEmpty() || Key[Key.Length()] != _D(')') ) {
            return false;
        }
        int const SepPos = FindTypeSepPos( Key );
        if ( SepPos == 0 ) {
            return false;
        }
        Name = Key.SubString( 1, SepPos - 1 );
        Tag  = Key.SubString( SepPos + 3, Key.Length() - SepPos - 3 );
        return !Name.IsEmpty() && !Tag.IsEmpty();
    }

    // -----------------------------------------------------------------------
    // StringCont codec  (items joined by '|', with '\' and '|' escaped)
    // -----------------------------------------------------------------------

    static String EncodeStringCont( StringCont const & Val ) {
        String Result;
        bool First = true;
        for ( auto const & Item : Val ) {
            if ( !First ) { Result += _D('|'); }
            First = false;
            for ( int i = 1; i <= Item.Length(); ++i ) {
                if      ( Item[i] == _D('\\') ) { Result += _D("\\\\"); }
                else if ( Item[i] == _D('|')  ) { Result += _D("\\|");  }
                else                            { Result += Item[i];    }
            }
        }
        return Result;
    }

    // Note: an empty string and a StringCont with a single empty item both
    // round-trip to StringCont{}.  This edge case is unlikely in practice.
    static StringCont DecodeStringCont( String const & Val ) {
        if ( Val.IsEmpty() ) {
            return StringCont{};
        }
        StringCont Result;
        String     Current;
        bool       Escape = false;
        for ( int i = 1; i <= Val.Length(); ++i ) {
            auto const Ch = Val[i];
            if ( Escape ) {
                Current += Ch;
                Escape = false;
            }
            else if ( Ch == _D('\\') ) {
                Escape = true;
            }
            else if ( Ch == _D('|') ) {
                Result.push_back( Current );
                Current = String{};
            }
            else {
                Current += Ch;
            }
        }
        Result.push_back( Current );
        return Result;
    }

    // -----------------------------------------------------------------------
    // Visitor helpers (same pattern as CfgXML.h)
    // -----------------------------------------------------------------------

    template<class...>
    static constexpr bool always_false_v = false;

    template<class... Ts>
    struct overload : Ts...
    {
        using Ts::operator()...;
        template<typename T>
        constexpr void operator()( T ) const {
            static_assert( always_false_v<T>, "Unsupported type" );
        }
    };

    template<class... Ts>
    overload( Ts... ) -> overload<Ts...>;

    // Return the type-tag string for a variant value (used when erasing keys).
    static String GetTagStr( TConfigNodeValueType const & Val ) {
#if defined( ANAFESTICA_USE_STD_VARIANT )
        return std::visit(
#else
        return boost::apply_visitor(
#endif
            overload {
                []( int                          ) -> String { return String( ana_cnv_xstr( ANA_TT_I    ) ); },
                []( unsigned int                 ) -> String { return String( ana_cnv_xstr( ANA_TT_U    ) ); },
                []( long                         ) -> String { return String( ana_cnv_xstr( ANA_TT_L    ) ); },
                []( unsigned long                ) -> String { return String( ana_cnv_xstr( ANA_TT_UL   ) ); },
                []( char                         ) -> String { return String( ana_cnv_xstr( ANA_TT_C    ) ); },
                []( unsigned char                ) -> String { return String( ana_cnv_xstr( ANA_TT_UC   ) ); },
                []( short                        ) -> String { return String( ana_cnv_xstr( ANA_TT_S    ) ); },
                []( unsigned short               ) -> String { return String( ana_cnv_xstr( ANA_TT_US   ) ); },
                []( long long                    ) -> String { return String( ana_cnv_xstr( ANA_TT_LL   ) ); },
                []( unsigned long long           ) -> String { return String( ana_cnv_xstr( ANA_TT_ULL  ) ); },
                []( bool                         ) -> String { return String( ana_cnv_xstr( ANA_TT_B    ) ); },
                []( System::UnicodeString const& ) -> String { return String( ana_cnv_xstr( ANA_TT_SZ   ) ); },
                []( System::TDateTime            ) -> String { return String( ana_cnv_xstr( ANA_TT_DT   ) ); },
                []( float                        ) -> String { return String( ana_cnv_xstr( ANA_TT_FLT  ) ); },
                []( double                       ) -> String { return String( ana_cnv_xstr( ANA_TT_DBL  ) ); },
                []( System::Currency             ) -> String { return String( ana_cnv_xstr( ANA_TT_CUR  ) ); },
                []( StringCont const&            ) -> String { return String( ana_cnv_xstr( ANA_TT_SV   ) ); },
                []( TBytes                       ) -> String { return String( ana_cnv_xstr( ANA_TT_DAB  ) ); },
                []( std::vector<Byte> const&     ) -> String { return String( ana_cnv_xstr( ANA_TT_VB   ) ); },
#if defined( ANAFESTICA_USE_STD_VARIANT )
                []( std::string const&           ) -> String { return String( ana_cnv_xstr( ANA_TT_STR  ) ); },
                []( std::wstring const&          ) -> String { return String( ana_cnv_xstr( ANA_TT_WSTR ) ); },
#endif
            },
            Val
        );
    }

    // -----------------------------------------------------------------------
    // Write a single key=value pair into the INI section
    // -----------------------------------------------------------------------
    void SaveValue( String const & Section, ValueContType::value_type const & v ) {
        auto const Key = EncodeKey( v.first, GetTagStr( v.second.first ) );
#if defined( ANAFESTICA_USE_STD_VARIANT )
        std::visit(
#else
        boost::apply_visitor(
#endif
            overload {
                [&]( int Val ) {
                    ini_->WriteString( Section, Key, IntToStr( Val ) );
                },
                [&]( unsigned int Val ) {
                    ini_->WriteString( Section, Key,
                        IntToStr( static_cast<__int64>( Val ) ) );
                },
                [&]( long Val ) {
                    ini_->WriteString( Section, Key,
                        IntToStr( static_cast<__int64>( Val ) ) );
                },
                [&]( unsigned long Val ) {
                    ini_->WriteString( Section, Key,
                        IntToStr( static_cast<__int64>( Val ) ) );
                },
                [&]( char Val ) {
                    ini_->WriteString( Section, Key,
                        IntToStr( static_cast<int>( Val ) ) );
                },
                [&]( unsigned char Val ) {
                    ini_->WriteString( Section, Key,
                        IntToStr( static_cast<int>( Val ) ) );
                },
                [&]( short Val ) {
                    ini_->WriteString( Section, Key,
                        IntToStr( static_cast<int>( Val ) ) );
                },
                [&]( unsigned short Val ) {
                    ini_->WriteString( Section, Key,
                        IntToStr( static_cast<int>( Val ) ) );
                },
                [&]( long long Val ) {
                    ini_->WriteString( Section, Key,
                        IntToStr( static_cast<__int64>( Val ) ) );
                },
                [&]( unsigned long long Val ) {
                    ini_->WriteString( Section, Key,
#if defined( _UNICODE )
                        String( std::to_wstring( Val ).c_str() )
#else
                        String( std::to_string( Val ).c_str() )
#endif
                    );
                },
                [&]( bool Val ) {
                    ini_->WriteString( Section, Key, BoolToStr( Val, true ) );
                },
                [&]( System::UnicodeString Val ) {
                    ini_->WriteString( Section, Key, Val );
                },
                [&]( System::TDateTime Val ) {
                    ini_->WriteString( Section, Key, DateToISO8601( Val, false ) );
                },
                [&]( float Val ) {
                    TFormatSettings FS;
                    FS.DecimalSeparator = _D( '.' );
                    ini_->WriteString( Section, Key,
                        FloatToStrF(
                            static_cast<double>( Val ),
                            TFloatFormat::ffGeneral, 9, 0, FS
                        )
                    );
                },
                [&]( double Val ) {
                    TFormatSettings FS;
                    FS.DecimalSeparator = _D( '.' );
                    ini_->WriteString( Section, Key,
                        FloatToStrF( Val, TFloatFormat::ffGeneral, 17, 0, FS )
                    );
                },
                [&]( System::Currency Val ) {
                    TFormatSettings FS;
                    FS.DecimalSeparator = _D( '.' );
                    ini_->WriteString( Section, Key, CurrToStr( Val, FS ) );
                },
                [&]( StringCont const & Val ) {
                    ini_->WriteString( Section, Key, EncodeStringCont( Val ) );
                },
                [&]( TBytes Val ) {
                    ini_->WriteString( Section, Key,
                        Val.Length == 0 ?
                            String{}
                        :
                            TNetEncoding::Base64->EncodeBytesToString(
                                &Val[0], Val.High
                            )
                    );
                },
                [&]( std::vector<Byte> const & Val ) {
                    ini_->WriteString( Section, Key,
                        Val.empty() ?
                            String{}
                        :
                            TNetEncoding::Base64->EncodeBytesToString(
                                Val.data(), Val.size() - 1
                            )
                    );
                },
#if defined( ANAFESTICA_USE_STD_VARIANT )
                [&]( std::string const & Val ) {
                    ini_->WriteString( Section, Key, UTF8ToString( Val.c_str() ) );
                },
                [&]( std::wstring const & Val ) {
                    ini_->WriteString( Section, Key, String( Val.c_str() ) );
                },
#endif
            },
            v.second.first
        );
    }

protected:
    // -----------------------------------------------------------------------
    // DoCreateValueList – read all typed key=value pairs for the given path
    // -----------------------------------------------------------------------
    virtual ValueContType DoCreateValueList( TConfigPath const & Path ) override {
        using Fn = std::function<TConfigNodeValueType( String )>;

        static std::array<Fn,
#if defined( ANAFESTICA_USE_STD_VARIANT )
            std::variant_size<TConfigNodeValueType>::value
#else
            TConfigNodeValueType::types::size::value
#endif
        > Builders {
            // TT_I
            []( String Value ) -> TConfigNodeValueType {
                return std::stoi( Value.c_str() );
            },
            // TT_U
            []( String Value ) -> TConfigNodeValueType {
                return static_cast<unsigned>( std::stoul( Value.c_str() ) );
            },
            // TT_L
            []( String Value ) -> TConfigNodeValueType {
                return std::stol( Value.c_str() );
            },
            // TT_UL
            []( String Value ) -> TConfigNodeValueType {
                return std::stoul( Value.c_str() );
            },
            // TT_C
            []( String Value ) -> TConfigNodeValueType {
                return static_cast<char>( std::stoi( Value.c_str() ) );
            },
            // TT_UC
            []( String Value ) -> TConfigNodeValueType {
                return static_cast<unsigned char>( std::stoul( Value.c_str() ) );
            },
            // TT_S
            []( String Value ) -> TConfigNodeValueType {
                return static_cast<short>( std::stoi( Value.c_str() ) );
            },
            // TT_US
            []( String Value ) -> TConfigNodeValueType {
                return static_cast<unsigned short>( std::stoul( Value.c_str() ) );
            },
            // TT_LL
            []( String Value ) -> TConfigNodeValueType {
                return std::stoll( Value.c_str() );
            },
            // TT_ULL
            []( String Value ) -> TConfigNodeValueType {
                return std::stoull( Value.c_str() );
            },
            // TT_B
            []( String Value ) -> TConfigNodeValueType {
                return StrToBool( Value );
            },
            // TT_SZ
            []( String Value ) -> TConfigNodeValueType {
                return Value;
            },
            // TT_DT
            []( String Value ) -> TConfigNodeValueType {
                return ISO8601ToDate( Value, false );
            },
            // TT_FLT
            []( String Value ) -> TConfigNodeValueType {
                return std::stof( Value.c_str() );
            },
            // TT_DBL
            []( String Value ) -> TConfigNodeValueType {
                return std::stod( Value.c_str() );
            },
            // TT_CUR
            []( String Value ) -> TConfigNodeValueType {
                TFormatSettings FS;
                FS.DecimalSeparator = _D( '.' );
                return StrToCurr( Value, FS );
            },
            // TT_SV
            []( String Value ) -> TConfigNodeValueType {
                return TConfig::DecodeStringCont( Value );
            },
            // TT_DAB
            []( String Value ) -> TConfigNodeValueType {
                return TNetEncoding::Base64->DecodeStringToBytes( Value );
            },
            // TT_VB
            []( String Value ) -> TConfigNodeValueType {
                auto Bytes = TNetEncoding::Base64->DecodeStringToBytes( Value );
                std::vector<Byte> VBytes;
                VBytes.reserve( Bytes.Length );
                std::copy(
                    std::begin( Bytes ), std::end( Bytes ),
                    std::back_inserter( VBytes )
                );
                return VBytes;
            },
#if defined( ANAFESTICA_USE_STD_VARIANT )
            // TT_STR  (UTF-8 std::string)  — bcc64x only
            []( String Value ) -> TConfigNodeValueType {
                return std::string( UTF8Encode( Value ).c_str() );
            },
            // TT_WSTR (UTF-16 std::wstring) — bcc64x only
            []( String Value ) -> TConfigNodeValueType {
                return std::wstring( Value.c_str() );
            },
#endif
        };

        ValueContType Values;
        auto const Section = GetSectionName( Path );
        auto SL = std::make_unique<TStringList>();
        ini_->ReadSection( Section, SL.get() );
        for ( int i = 0; i < SL->Count; ++i ) {
            String const & Key = SL->Strings[i];
            String Name, Tag;
            if ( DecodeKey( Key, Name, Tag ) ) {
                if ( auto const TypeOpt = GetTypeTag( Tag ) ) {
                    String const Val = ini_->ReadString( Section, Key, String{} );
                    PutItemTo(
                        Values, Name,
                        {
                            Builders[static_cast<size_t>( TypeOpt.value() )]( Val ),
                            Operation::None
                        }
                    );
                }
            }
            // Keys without a recognised ::(TypeTag) suffix are silently ignored;
            // they may have been placed there manually and are not managed by
            // Anafestica.
        }
        return Values;
    }

    // -----------------------------------------------------------------------
    // DoCreateNodeList – enumerate direct child sections for the given path
    // -----------------------------------------------------------------------
    virtual NodeContType DoCreateNodeList( TConfigPath const & Path ) override {
        NodeContType Nodes;
        auto const Section = GetSectionName( Path );
        auto const Prefix  = Section + _D("\\");
        auto SL = std::make_unique<TStringList>();
        ini_->ReadSections( SL.get() );
        for ( int i = 0; i < SL->Count; ++i ) {
            String const & S = SL->Strings[i];
            if ( S.Length() > Prefix.Length() &&
                 S.SubString( 1, Prefix.Length() ) == Prefix )
            {
                String const Remainder = S.SubString(
                    Prefix.Length() + 1,
                    S.Length() - Prefix.Length()
                );
                // Extract the first path component (before any '\').
                // This handles both direct children ("Child") and deeper
                // sections ("Child\GrandChild") whose intermediate ancestor
                // sections may not have been written (e.g. when Child has
                // no values of its own).
                int const Sep = Remainder.Pos( _D("\\") );
                String const DirectChild =
                    Sep > 0 ? Remainder.SubString( 1, Sep - 1 ) : Remainder;
                if ( !DirectChild.IsEmpty() &&
                     Nodes.find( DirectChild ) == std::end( Nodes ) ) {
                    Nodes[DirectChild] = std::make_unique<TConfigNode>();
                }
            }
        }
        return Nodes;
    }

    // -----------------------------------------------------------------------
    // DoSaveValueList – write / erase values in the given section
    // -----------------------------------------------------------------------
    virtual void DoSaveValueList( TConfigPath const & Path,
                                  ValueContType const & Values ) override {
        auto const Section = GetSectionName( Path );
        for ( auto const & v : Values ) {
            auto const ValueState = v.second.second;
            if ( GetAlwaysFlushNodeFlag() || ValueState == Operation::Write ) {
                SaveValue( Section, v );
            }
            else if ( ValueState == Operation::Erase ) {
                ini_->DeleteKey(
                    Section,
                    EncodeKey( v.first, GetTagStr( v.second.first ) )
                );
            }
        }
    }

    // -----------------------------------------------------------------------
    // DoDeleteNode – erase a section and all its descendant sections
    // -----------------------------------------------------------------------
    virtual void DoDeleteNode( TConfigPath const & Path ) override {
        auto const Section = GetSectionName( Path );
        ini_->EraseSection( Section );
        // Also erase all descendants (subsections whose name starts with
        // "Section\").
        auto const Prefix = Section + _D("\\");
        auto SL = std::make_unique<TStringList>();
        ini_->ReadSections( SL.get() );
        for ( int i = 0; i < SL->Count; ++i ) {
            if ( SL->Strings[i].SubString( 1, Prefix.Length() ) == Prefix ) {
                ini_->EraseSection( SL->Strings[i] );
            }
        }
    }

    // -----------------------------------------------------------------------
    // DoFlush – write the in-memory tree back to the INI file
    // -----------------------------------------------------------------------
    virtual void DoFlush() override {
        // Open the TMemIniFile against fileName_ so that UpdateFile writes
        // to the destination, regardless of where the initial load came from.
        IniFileRAII Ini{ *this, fileName_ };
        GetRootNode().Write( *this, TConfigPath{} );

        if ( !TFile::Exists( fileName_ ) ) {
            auto const DirPath =
                TPath::GetFullPath( TPath::GetDirectoryName( fileName_ ) );
            if ( !TDirectory::Exists( DirPath ) ) {
                TDirectory::CreateDirectory( DirPath );
            }
        }
        if ( cryptOptions_.Enabled ) {
            auto SL = std::make_unique<TStringList>();
            ini_->GetStrings( SL.get() );
            Crypt::SaveText( fileName_, SL->Text, TEncoding::UTF8, cryptOptions_ );
        }
        else {
            ini_->UpdateFile();
        }
    }
};

//---------------------------------------------------------------------------
} // End namespace INIFile
//---------------------------------------------------------------------------
} // End namespace Anafestica
//---------------------------------------------------------------------------

#endif
