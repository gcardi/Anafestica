//---------------------------------------------------------------------------

#ifndef CfgRegistryH
#define CfgRegistryH

#include <memory>
#include <vector>
#include <algorithm>
#include <iterator>
#include <regex>
#include <functional>
#include <type_traits>
#include <array>
#include <tuple>
#include <utility>

#include <System.Classes.hpp>
#include <System.SysUtils.hpp>
#include <System.Win.Registry.hpp>
#include <System.RTLConsts.hpp>

#include <anafestica/Cfg.h>
#include <anafestica/FileVersionInfo.h>

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------
namespace Registry {
//---------------------------------------------------------------------------

template <typename IT>
struct ItValueType {
    typedef typename std::iterator_traits<IT>::value_type elem;
};

template <typename Container>
struct ItValueType<std::back_insert_iterator<Container>>
{
    typedef typename Container::value_type elem;
};

template <typename Container>
struct ItValueType<std::front_insert_iterator<Container>>
{
    typedef typename Container::value_type elem;
};

template <typename Container>
struct ItValueType<std::insert_iterator<Container>>
{
    typedef typename Container::value_type elem;
};

struct BuildString {
    template<typename FwIt>
    String operator ()( FwIt Begin, FwIt End ) const {
        return String(
            reinterpret_cast<TCHAR*>( &*Begin ),
            std::distance( Begin, End )
        );
    }
};

enum class TExRegDataType {
    Binary,     // REG_BINARY
                // Binary data in any form.

    Dword,      // REG_DWORD
                // A 32-bit number. (LE on x86-x64)

    ExpandSz,   // REG_EXPAND_SZ
                // A null-terminated string that contains unexpanded
                // references to environment variables (for example,
                // "%PATH%"). To expand the environment variable
                // references, use the ExpandEnvironmentStrings function.

    Link,       // REG_LINK
                // A null-terminated Unicode string that contains
                // the target path of a symbolic link that was
                // created by calling the RegCreateKeyEx function
                // with REG_OPTION_CREATE_LINK.

    MultiSz,    // REG_MULTI_SZ
                // A sequence of null-terminated strings, terminated
                // by an empty string (\0).

    None,       // REG_NONE
                // No defined value type.

    Qword,      // REG_QWORD
                // A 64-bit number.

    Sz          // REG_SZ
                // A null-terminated string.
};

class TRegistry : public System::Win::Registry::TRegistry {
public:

    template<typename...A>
    TRegistry( A...Args )
      : System::Win::Registry::TRegistry( std::forward<A...>( Args )... ) {}

    template<typename T>
    typename std::enable_if_t<std::is_integral_v<T> && sizeof( T ) == 8,T>
    ReadQWORD( String Name );

    size_t ReadStrings( String Name, TStrings& Result ) {
        return ReadStringsTo( Name, System::back_inserter( &Result ) );
    }

    template<typename OutIt, typename BT = BuildString>
	size_t ReadStringsTo( String Name, OutIt it );

    template<typename OutIt>
    typename
        std::enable_if_t<
            std::is_integral_v<typename ItValueType<OutIt>::elem> &&
              sizeof( typename ItValueType<OutIt>::elem ) == 1,
            size_t
        >
    ReadBinaryDataTo( String Name, OutIt It );

    TBytes ReadBinaryData( String Name ) {
        DWORD Type {};
        DWORD Size {};

        if ( !CheckResult(
               ::RegQueryValueEx(
                 CurrentKey, Name.c_str(), nullptr, &Type, nullptr, &Size
               )
             )
        )
        {
            throw ERegistryException(
                &_SRegGetDataFailed, ARRAYOFCONST(( Name ))
            );
        }

        if ( Type != REG_BINARY ) {
            throw ERegistryException(
                &_SInvalidRegType, ARRAYOFCONST(( Name ))
            );
        }

        TBytes Data;
        Data.Length = Size;

        if ( !CheckResult(
               ::RegQueryValueEx(
                 CurrentKey, Name.c_str(), nullptr, &Type,&Data[0], &Size
               )
             )
        )
        {
            throw ERegistryException(
                &_SRegGetDataFailed, ARRAYOFCONST(( Name ))
            );
        }

        // check again if the type has changed in the meantime
        if ( Type != REG_BINARY ) {
            throw ERegistryException(
                &_SInvalidRegType, ARRAYOFCONST(( Name ))
            );
        }

        return Data;
    }

    void WriteQWORD( String Name, unsigned long long Value ) {
        if ( !CheckResult(
               ::RegSetValueEx(
                 CurrentKey, Name.c_str(), 0, REG_QWORD,
                 reinterpret_cast<BYTE*>( &Value ), sizeof Value
               )
             )
        )
        {
            throw ERegistryException(
                &_SRegSetDataFailed, ARRAYOFCONST(( Name ))
            );
        }
    }

    void WriteQWORD( String Name, long long Value ) {
        WriteQWORD( Name, static_cast<unsigned long long>( Value ) );
    }

    void WriteStrings( String Name, TStrings& Value ) {
        WriteStrings( Name, System::begin( &Value ), System::end( &Value ) );
    }

    void WriteStrings( String Name, std::vector<String> const & Value ) {
        WriteStrings( Name, std::begin( Value ), std::end( Value ) );
    }

    template<typename InIt>
    void WriteStrings( String Name, InIt Begin, InIt End );

    void WriteBinaryData( String Name, TBytes Value ) {
        if ( !CheckResult(
               ::RegSetValueEx(
                 CurrentKey, Name.c_str(), 0, REG_BINARY,
                 &Value[0], Value.Length
               )
             )
        )
        {
            throw ERegistryException(
                &_SRegSetDataFailed, ARRAYOFCONST(( Name ))
            );
        }
    }

    void WriteBinaryData( String Name, std::vector<Byte> const & Value ) {
        if ( !CheckResult(
               ::RegSetValueEx(
                 CurrentKey, Name.c_str(), 0, REG_BINARY,
                 Value.data(), Value.size()
               )
             )
        )
        {
            throw ERegistryException(
                &_SRegSetDataFailed, ARRAYOFCONST(( Name ))
            );
        }
    }

    std::tuple<TExRegDataType,size_t> GetExDataType( String Name ) {
        DWORD Type {};
        DWORD Size {};
        if ( !CheckResult(
               ::RegQueryValueEx(
                 CurrentKey, Name.c_str(), nullptr, &Type, nullptr, &Size
               )
             )
        )
        {
            throw ERegistryException(
                &_SRegGetDataFailed, ARRAYOFCONST(( Name ))
            );
        }
        switch ( Type ) {
            case REG_BINARY:
                return std::make_tuple( TExRegDataType::Binary, Size );
            case REG_DWORD:
                return std::make_tuple( TExRegDataType::Dword, Size );
            case REG_EXPAND_SZ:
                return std::make_tuple( TExRegDataType::ExpandSz, Size );
            case REG_LINK:
                return std::make_tuple( TExRegDataType::Link, Size );
            case REG_MULTI_SZ:
                return std::make_tuple( TExRegDataType::MultiSz, Size );
            case REG_NONE:
                return std::make_tuple( TExRegDataType::None, Size );
            case REG_QWORD:
                return std::make_tuple( TExRegDataType::Qword, Size );
            case REG_SZ:
                return std::make_tuple( TExRegDataType::Sz, Size );
            default:
                throw ERegistryException(
                    &_SInvalidRegType, ARRAYOFCONST(( Name ))
                );
        }
    }

    String ReadExpandString( String Name ) {
        std::array<TCHAR,32767> Buffer;

        auto const Ret = ::ExpandEnvironmentStrings(
            ReadString( Name ).c_str(),
            Buffer.data(), Buffer.size()
        );

        if ( Ret ) {
            return String( Buffer.data(), Ret );
        }

        throw ERegistryException(
            &_SRegGetDataFailed, ARRAYOFCONST(( Name ))
        );
    }
};
//---------------------------------------------------------------------------

template<typename T>
typename std::enable_if_t<std::is_integral_v<T> && sizeof( T ) == 8,T>
TRegistry::ReadQWORD( String Name )
{
    unsigned long long Data;
    DWORD Type {};
    DWORD Size { sizeof Data };
    if ( !CheckResult(
           ::RegQueryValueEx(
             CurrentKey, Name.c_str(), nullptr, &Type,
             reinterpret_cast<BYTE*>( &Data ),
             &Size
           )
         )
    )
    {
        throw ERegistryException(
            &_SRegGetDataFailed, ARRAYOFCONST(( Name ))
        );
    }
    if ( Type != REG_QWORD ) {
        throw ERegistryException(
            &_SInvalidRegType, ARRAYOFCONST(( Name ))
        );
    }

    return static_cast<T>( Data );
}
//---------------------------------------------------------------------------

template<typename OutIt, typename BT>
size_t TRegistry::ReadStringsTo( String Name, OutIt It )
{
    DWORD Type {};
    DWORD Size {};
    if ( !CheckResult(
           ::RegQueryValueEx(
             CurrentKey, Name.c_str(), nullptr, &Type, nullptr, &Size
           )
         )
    )
    {
        throw ERegistryException( &_SRegGetDataFailed, ARRAYOFCONST(( Name )) );
    }
    if ( Type != REG_MULTI_SZ ) {
        throw ERegistryException( &_SInvalidRegType, ARRAYOFCONST(( Name )) );
    }
    std::vector<TCHAR> Data( Size / sizeof( TCHAR ) );
    if ( !CheckResult(
           ::RegQueryValueEx(
             CurrentKey, Name.c_str(), nullptr, &Type,
             reinterpret_cast<LPBYTE>( Data.data() ), &Size
           )
         )
    )
    {
        throw ERegistryException( &_SRegGetDataFailed, ARRAYOFCONST(( Name )) );
    }
    // check again if the type has changed in the meantime
    if ( Type != REG_MULTI_SZ ) {
        throw ERegistryException( &_SInvalidRegType, ARRAYOFCONST(( Name )) );
    }

	size_t Cnt {};

	auto Start = std::begin( Data );
    auto End = std::end( Data );
    for ( auto Begin = Start ; Begin != End ; ) {
        auto OldBegin = Begin++;
        if ( !*OldBegin ) {
            if ( Begin != End ) {
                *It++ = BT{}( Start, OldBegin );
                Start = Begin;
                ++Cnt;
            }
        }
    }
    return Cnt;
}
//---------------------------------------------------------------------------

template<typename OutIt>
typename
    std::enable_if_t<
        std::is_integral_v<typename ItValueType<OutIt>::elem> &&
          sizeof( typename ItValueType<OutIt>::elem ) == 1,
        size_t
    >
TRegistry::ReadBinaryDataTo( String Name, OutIt It )
{
    DWORD Type {};
    DWORD Size {};
    if ( !CheckResult(
           ::RegQueryValueEx(
             CurrentKey, Name.c_str(), nullptr, &Type, nullptr, &Size
           )
         )
    )
    {
        throw ERegistryException( &_SRegGetDataFailed, ARRAYOFCONST(( Name )) );
    }
    if ( Type != REG_BINARY ) {
        throw ERegistryException( &_SInvalidRegType, ARRAYOFCONST(( Name )) );
    }
    std::vector<Byte> Data( Size );
    if ( !CheckResult(
           ::RegQueryValueEx(
             CurrentKey, Name.c_str(), nullptr, &Type, Data.data(), &Size
           )
         )
    )
    {
        throw ERegistryException( &_SRegGetDataFailed, ARRAYOFCONST(( Name )) );
    }
    // check again if the type has changed in the meantime
    if ( Type != REG_BINARY ) {
        throw ERegistryException( &_SInvalidRegType, ARRAYOFCONST(( Name )) );
    }

    std::copy( std::begin( Data ), std::end( Data ), It );

    return Data.size();
}
//---------------------------------------------------------------------------

template<typename InIt>
void TRegistry::WriteStrings( String Name, InIt Begin, InIt End )
{
    auto SB = std::make_unique<TStringBuilder>();
    while ( Begin != End ) {
        SB->Append( *Begin++ );
        SB->Append( _D( '\0' ) );
    }
    SB->Append( _D( '\0' ) );
    if ( !CheckResult(
           ::RegSetValueEx(
             CurrentKey, Name.c_str(), 0, REG_MULTI_SZ,
             reinterpret_cast<BYTE*>( SB->ToString().c_str() ),
             SB->Length * sizeof _D( '\0' )
           )
         )
    )
    {
        throw ERegistryException( &_SRegSetDataFailed, ARRAYOFCONST(( Name )) );
    }
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------


class TConfig : public Anafestica::TConfig {
public:
    TConfig( HKEY HKey, String RootPath, bool ReadOnly = false,
             bool FlushAllItems = false )
        : Anafestica::TConfig( ReadOnly, FlushAllItems )
        , rootPath_( RootPath ) , hKey_( HKey )
    {
        RegObjRAII Reg{ *this };
        GetRootNode().Read( *this, TConfigPath{} );
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
    #if !defined(_UNICODE)
      using regex_type = std::regex;
      using cmatch_type = std::cmatch;
      using std_string = std::string;
    #else
      using regex_type = std::wregex;
      using cmatch_type = std::wcmatch;
      using std_string = std::wstring;
    #endif

    template<typename PairType>
    void DeleteValue( PairType const & v ) { registry_->DeleteValue( v.first ); }

    void DeleteKey( TConfigPath const & Path ) {
        auto const Key = GetKeyName( Path, rootPath_ );
        if ( !registry_->CurrentPath.IsEmpty() ) {
            if ( registry_->CurrentPath == Key ) {
                registry_->CloseKey();
            }
        }
        registry_->DeleteKey( Key );
    }
protected:
    virtual ValueContType DoCreateValueList( TConfigPath const & Path ) override {
        regex_type re(
            _D( "" )
            "^(.*\?)(\?::(\\((\?:"
                "(" cnv_xstr( TT_I )   ")|(" cnv_xstr( TT_U )   ")|"
                "(" cnv_xstr( TT_L )   ")|(" cnv_xstr( TT_UL )  ")|"
                "(" cnv_xstr( TT_C )   ")|(" cnv_xstr( TT_UC )  ")|"
                "(" cnv_xstr( TT_S )   ")|(" cnv_xstr( TT_US )  ")|"
                "(" cnv_xstr( TT_LL )  ")|(" cnv_xstr( TT_ULL ) ")|"
                "(" cnv_xstr( TT_B )   ")|(" cnv_xstr( TT_SZ )  ")|"
                "(" cnv_xstr( TT_DT )  ")|(" cnv_xstr( TT_FLT ) ")|"
                "(" cnv_xstr( TT_DBL ) ")|(" cnv_xstr( TT_CUR ) ")|"
                "(" cnv_xstr( TT_SV )  ")|(" cnv_xstr( TT_DAB ) ")|"
                "(" cnv_xstr( TT_VB )  ")"
            "))\\))\?$"
        );

        using RegObjType = std::remove_reference_t<decltype( *registry_ )>;

        using ValueBuilder =
            std::function<
                TConfigNodeValueType (
                    RegObjType &, // Registry Object
                    String        // Key Name
                )
            >;

        static std::array<ValueBuilder,TConfigNodeValueType::types::size::value> Builders {

            // CLASS  TAG         REG_DYPE      API
            // -----  -------     ------------  ----------------

            // i32    (TT_I)      REG_DWORD     ReadInteger
            []( RegObjType& Reg, String KeyName ) {
                return Reg.ReadInteger( KeyName );
            },

            // u32    (TT_U)      REG_DWORD     ReadInteger
            []( RegObjType& Reg, String KeyName ) {
                return static_cast<unsigned>( Reg.ReadInteger( KeyName ) );
            },

            // i32    (TT_L)      REG_DWORD     ReadInteger
            []( RegObjType& Reg, String KeyName ) {
                return static_cast<long>( Reg.ReadInteger( KeyName ) );
            },

            // u32    (TT_UL)     REG_DWORD     ReadInteger
            []( RegObjType& Reg, String KeyName ) {
                return static_cast<unsigned long>( Reg.ReadInteger( KeyName ) );
            },

            // i8     (TT_C)      REG_DWORD     ReadInteger
            []( RegObjType& Reg, String KeyName ) {
                return static_cast<char>( Reg.ReadInteger( KeyName ) );
            },

            // u8     (TT_UC)     REG_DWORD     ReadInteger
            []( RegObjType& Reg, String KeyName ) {
                return static_cast<unsigned char>( Reg.ReadInteger( KeyName ) );
            },

            // i16    (TT_S)      REG_DWORD     ReadInteger
            []( RegObjType& Reg, String KeyName ) {
                return static_cast<short>( Reg.ReadInteger( KeyName ) );
            },

            // u16    (TT_US)     REG_DWORD     ReadInteger
            []( RegObjType& Reg, String KeyName ) {
                return static_cast<unsigned short>( Reg.ReadInteger( KeyName ) );
            },

            // i64    (TT_LL)     REG_QWORD     ReadQWORD
            []( RegObjType& Reg, String KeyName ) {
                return Reg.ReadQWORD<long long>( KeyName );
            },

            // u64    (TT_ULL)    REG_QWORD     ReadQWORD
            []( RegObjType& Reg, String KeyName ) {
                return Reg.ReadQWORD<unsigned long long>( KeyName );
            },

            // u8     (TT_B)      REG_DWORD     ReadInteger
            []( RegObjType& Reg, String KeyName ) {
                return static_cast<bool>( Reg.ReadInteger( KeyName ) );
            },

            // sz     (TT_SZ)     REG_SZ        ReadString
            []( RegObjType& Reg, String KeyName ) {
                return Reg.ReadString( KeyName );
            },

            // b8     (TT_DT)     REG_BINARY    ReadDateTime
            []( RegObjType& Reg, String KeyName ) {
                return Reg.ReadDateTime( KeyName );
            },

            // b8     (TT_FLT)    REG_BINARY    ReadFloat
            []( RegObjType& Reg, String KeyName ) {
                return static_cast<float>( Reg.ReadFloat( KeyName ) );
            },

            // b8     (TT_DBL)    REG_BINARY    ReadFloat
            []( RegObjType& Reg, String KeyName ) {
                return Reg.ReadFloat( KeyName );
            },

            // b8     (TT_CUR)    REG_BINARY    ReadCurrency
            []( RegObjType& Reg, String KeyName ) {
                return Reg.ReadCurrency( KeyName );
            },

            // sv     (TT_SV)     REG_MULTI_SZ  ReadStringsTo
            []( RegObjType& Reg, String KeyName ) {
                StringCont Strings;
                Reg.ReadStringsTo( KeyName, std::back_inserter( Strings ) );
                return std::move( Strings );
            },

            // dab    (TT_DAB)    REG_BINARY    ReadBinaryData
            []( RegObjType& Reg, String KeyName ) {
                return Reg.ReadBinaryData( KeyName );
            },

            // vb     (TT_VB)     REG_BINARY    ReadBinaryDataTo
            []( RegObjType& Reg, String KeyName ) {
                auto Size = Reg.GetDataSize( KeyName );
                if ( Size < 0 ) {
                    throw Exception(
                        _D( "Registry Key %s\\%s has invalid data size" ),
                        ARRAYOFCONST((
                            Reg.CurrentPath,
                            KeyName
                        ))
                    );
                }
                auto Bytes = std::vector<Byte>( Size );
                Reg.ReadBinaryDataTo( KeyName, std::back_inserter( Bytes ) );
                return std::move( Bytes );
            },
        };

        struct PutItem {
            void operator()(
                ValueContType& Values,
                String Name,
                ValueType const & Value
            ) const {
                PutItemTo(
                    Values, Name, { Value, Operation::None }
                );
            }
        };

        ValueContType Values;
        if ( OpenKeyReadOnly( GetKeyName( Path ) ) ) {
            auto RegValues = std::make_unique<TStringList>();
            registry_->GetValueNames( RegValues.get() );
            cmatch_type ms;
            for ( auto ValueName : RegValues.get() ) {
                if ( regex_match( ValueName.c_str(), ms, re ) ) {
                    auto It = std::begin( ms );
                    std::advance( It, 1 );
                    String const Name = It->str().c_str();
                    std::advance( It, 2 );
                    It =
                        find_if(
                            It, std::end( ms ),
                            []( auto const & m ) { return m.matched; }
                        );
                    if ( It != std::end( ms ) ) {
                        auto const Idx = distance( std::begin( ms ), It ) - 3;
                        auto Val = Builders[Idx]( *registry_, ValueName );
                        PutItem{}( Values, Name, Val );
                    }
                    else {
                        auto [Type,Size] = registry_->GetExDataType( ValueName );
                        switch ( Type ) {
                            case TExRegDataType::Binary:
                                PutItem{}(
                                    Values, Name,
                                    registry_->ReadBinaryData( ValueName )
                                );
                                break;
                            case TExRegDataType::Dword:
                                PutItem{}(
                                    Values, Name,
                                    registry_->ReadInteger( ValueName )
                                );
                                break;
                            case TExRegDataType::MultiSz: {
                                    StringCont Strings;
                                    registry_->ReadStringsTo(
                                        ValueName,
                                        std::back_inserter( Strings )
                                    );
                                    PutItem{}( Values, Name, Strings );
                                }
                                break;
                            case TExRegDataType::Qword:
                                PutItem{}(
                                    Values, Name,
                                    registry_->ReadQWORD<long long>( ValueName )
                                );
                                break;
                            case TExRegDataType::Sz:
                                PutItem{}(
                                    Values, Name,
                                    registry_->ReadString( ValueName )
                                );
                                break;
                            case TExRegDataType::ExpandSz:
                                PutItem{}(
                                    Values, Name,
                                    registry_->ReadExpandString( ValueName )
                                );
                                break;
                            default:
                                throw Exception(
                                    _D( "Registry Key %s\\%s has an invalid data type" ),
                                    ARRAYOFCONST((
                                        registry_->CurrentPath,
                                        ValueName
                                    ))
                                );
                        }
                    }
                }
            }
        }
        return Values;
    }

    virtual NodeContType DoCreateNodeList( TConfigPath const & Path ) override {
        NodeContType Nodes;

        if ( OpenKeyReadOnly( GetKeyName( Path ) ) ) {
            auto RegKeys = std::make_unique<TStringList>();
            registry_->GetKeyNames( RegKeys.get() );
            for ( auto const & Key : RegKeys.get() ) {
                Nodes[Key] = std::move( std::make_unique<TConfigNode>() );
            }
        }
        return Nodes;
    }

    virtual void DoSaveValueList( TConfigPath const & Path, ValueContType const & Values ) override {
        if ( !Values.empty() ) {
            if ( OpenKey( GetKeyName( Path ), true ) ) {
                for ( auto& v : Values ) {
                    auto const ValueState = v.second.second;
                    if ( GetAlwaysFlushNodeFlag() || ValueState == Operation::Write ) {
                        SaveValue( *registry_, v );
                    }
                    else if ( v.second.second == Operation::Erase ) {
                        DeleteValue( v );
                    }
                }
            }
            else {
                throw ERegistryException(
                    _D( "Error in TConfigRegistry::DoSaveValueList: can't open key for writing" )
                );
            }
        }
    }

    virtual void DoDeleteNode( TConfigPath const & Path ) override {
        DeleteKey( std::move( Path ) );
    }

    virtual void DoFlush() override {
        RegObjRAII Reg{ *this };
        GetRootNode().Write( *this, TConfigPath{} );
    }

    virtual bool DoGetForcedWritesFlag() const { return false; }
private:
    class RegObjRAII {
    public:
        RegObjRAII( TConfig& Cfg ) : cfg_{ Cfg } { Cfg.CreateRegistryObject(); }
        ~RegObjRAII() noexcept {
            try { cfg_.DestroyAndCloseRegistryObject(); } catch ( ... ) {}
        }
        RegObjRAII( RegObjRAII const & ) = delete;
        RegObjRAII& operator=( RegObjRAII const & ) = delete;
    private:
        TConfig& cfg_;
    };

    String rootPath_;
    HKEY hKey_;
    std::unique_ptr<TRegistry> registry_;

    static String GetKeyName( TConfigPath const & Path, String Prefix = String{} ) {
        auto SB = std::make_unique<TStringBuilder>( Prefix );
        for ( auto const & Item : Path ) {
            SB->AppendFormat( _D( "\\%s" ), ARRAYOFCONST(( Item )) );
        }
        return SB->ToString();
    }

    void CreateRegistryObject() {
        registry_ =
            std::move( std::make_unique<Anafestica::Registry::TRegistry>() );
        registry_->RootKey = hKey_;
    }

    void DestroyAndCloseRegistryObject() {
        if ( !registry_->CurrentPath.IsEmpty() ) {
            registry_->CloseKey();
        }
        registry_.reset();
    }

    bool OpenKey( String Path, bool CanCreate = false ) {
        String const Key = ExcludeTrailingBackslash( rootPath_ + Path );
        if ( registry_->CurrentPath != Key ) {
            if ( !registry_->CurrentPath.IsEmpty() ) {
                registry_->CloseKey();
            }
            return registry_->OpenKey( Key, CanCreate );
        }
        return true;
    }

    bool OpenKeyReadOnly( String Path ) {
        String const Key = ExcludeTrailingBackslash( rootPath_ + Path );
        if ( registry_->CurrentPath != Key ) {
            if ( !registry_->CurrentPath.IsEmpty() ) {
                registry_->CloseKey();
            }
            return registry_->OpenKeyReadOnly( Key );
        }
        return true;
    }

    // https://andreasfertig.blog/2023/07/visiting-a-stdvariant-safely/
	template<class...>
	constexpr bool always_false_v = false;

	template<class... Ts>
	struct overload : Ts...
	{
	  using Ts::operator()...;

	  // Prevent implicit type conversions
	  template<typename T>
	  constexpr void operator()(T) const
	  {
		static_assert(always_false_v<T>, "Unsupported type");
	  }
	};

	template<class... Ts>
	overload(Ts...) -> overload<Ts...>;

	// https://www.bfilipek.com/2019/02/2lines3featuresoverload.html
	// template<class... Ts> struct overload : Ts... { using Ts::operator()...; };
	// template<class... Ts> overload( Ts... ) -> overload<Ts...>;

    void SaveValue( TRegistry& Reg, ValueContType::value_type const & v ) {
        boost::apply_visitor(
            overload {
                // REG_BINARY    - Binary data in any form.
                // REG_DWORD     - 32-bit number.
                // REG_QWORD     - 64-bit number.
                // REG_EXPAND_SZ - Null-terminated string that contains
                //                 unexpanded references to environment
                //                 variables
                // REG_MULTI_SZ  - Array of null-terminated strings that
                //                 are terminated by two null characters.
                // REG_SZ        - Null-terminated string.

                // CLASS  TAG        REG_DYPE      API
                // -----  ---------  ------------  ----------------
                // i32               REG_DWORD     WriteInteger
                [&Reg, &v]( int Val ) {
                    Reg.WriteInteger( v.first, Val );
                },

                // u32    (TT_U)     REG_DWORD     WriteInteger
                [&Reg, &v]( unsigned int Val ) {
                    Reg.WriteInteger(
                        Format(
                            _D( "%s:(" ) cnv_xstr( TT_U ) ")",
                            ARRAYOFCONST(( v.first ))
                        ),
                        Val
                    );
                },

                // i32    (TT_L)     REG_DWORD     WriteInteger
                [&Reg, &v]( long Val ) {
                    Reg.WriteInteger(
                        Format(
                            _D( "%s:(" ) cnv_xstr( TT_L ) ")",
                            ARRAYOFCONST(( v.first ))
                        ),
                        Val
                    );
                },

                // u32    (TT_UL)    REG_DWORD     WriteInteger
                [&Reg, &v]( unsigned long Val ) {
                    Reg.WriteInteger(
                        Format(
                            _D( "%s:(" ) cnv_xstr( TT_UL ) ")",
                            ARRAYOFCONST(( v.first ))
                        ),
                        Val
                    );
                },

                // i8     (TT_C)     REG_DWORD     WriteInteger
                [&Reg, &v]( char Val ) {
                    Reg.WriteInteger(
                        Format(
                            _D( "%s:(" ) cnv_xstr( TT_C ) ")",
                            ARRAYOFCONST(( v.first ))
                        ),
                        Val
                    );
                },

                // u8     (TT_UC)    REG_DWORD     WriteInteger
                [&Reg, &v]( unsigned char Val ) {
                    Reg.WriteInteger(
                        Format(
                            _D( "%s:(" ) cnv_xstr( TT_UC ) ")",
                            ARRAYOFCONST(( v.first ))
                        ),
                        Val
                    );
                },

                // i16    (TT_S)     REG_DWORD     WriteInteger
                [&Reg, &v]( short Val ) {
                    Reg.WriteInteger(
                        Format(
                            _D( "%s:(" ) cnv_xstr( TT_S ) ")",
                            ARRAYOFCONST(( v.first ))
                        ),
                        Val
                    );
                },

                // u16    (TT_US)    REG_DWORD     WriteInteger
                [&Reg, &v]( unsigned short Val ) {
                    Reg.WriteInteger(
                        Format(
                            _D( "%s:(" ) cnv_xstr( TT_US ) ")",
                            ARRAYOFCONST(( v.first ))
                        ),
                        Val
                    );
                },

                // i64               REG_QWORD     WriteQWORD
                [&Reg, &v]( long long Val ) {
                    Reg.WriteQWORD( v.first, Val );
                },

                // u64    (TT_ULL)   REG_QWORD     WriteQWORD
                [&Reg, &v]( unsigned long long Val ) {
                    Reg.WriteQWORD(
                        Format(
                            _D( "%s:(" ) cnv_xstr( TT_ULL ) ")",
                            ARRAYOFCONST(( v.first ))
                        ),
                        Val
                    );
                },

                // u8     (TT_B)     REG_DWORD     WriteInteger
                [&Reg, &v]( bool Val ) {
                    Reg.WriteInteger(
                        Format(
                            _D( "%s:(" ) cnv_xstr( TT_B ) ")",
                            ARRAYOFCONST(( v.first ))
                        ), Val
                    );
                },

                // sz                REG_SZ        WriteString
                [&Reg, &v]( System::UnicodeString Val ) {
                    Reg.WriteString( v.first, Val );
                },

                // b8     (TT_DT)    REG_BINARY    WriteDateTime
                [&Reg, &v]( System::TDateTime Val ) {
                    Reg.WriteDateTime(
                        Format(
                            _D( "%s:(" ) cnv_xstr( TT_DT ) ")",
                            ARRAYOFCONST(( v.first ))
                        ),
                        Val
                    );
                },

                // b8     (TT_FLT)   REG_BINARY    WriteFloat
                [&Reg, &v]( float Val ) {
                    Reg.WriteFloat(
                        Format(
                            _D( "%s:(" ) cnv_xstr( TT_FLT ) ")",
                            ARRAYOFCONST(( v.first ))
                        ),
                        Val
                    );
                },

                // b8     (TT_DBL)   REG_BINARY    WriteFloat
                [&Reg, &v]( double Val ) {
                    Reg.WriteFloat(
                        Format(
                            _D( "%s:(" ) cnv_xstr( TT_DBL ) ")",
                            ARRAYOFCONST(( v.first ))
                        ),
                        Val
                    );
                },

                // b8     (TT_CUR)   REG_BINARY    WriteCurrency
                [&Reg, &v]( System::Currency Val ) {
                    Reg.WriteCurrency(
                        Format(
                            _D( "%s:(" ) cnv_xstr( TT_CUR ) ")",
                            ARRAYOFCONST(( v.first ))
                        ),
                        Val
                    );
                },

                // sv     (TT_SV)    REG_MULTI_SZ  WriteStrings
                [&Reg, &v]( StringCont const & Val ) {
                    Reg.WriteStrings( v.first, Val );
                },

                // dab               REG_BINARY    WriteBinaryData
                [&Reg, &v]( TBytes Val ) {
                    Reg.WriteBinaryData( v.first, Val );
                },

                // vb     (TT_VB)    REG_BINARY    WriteBinaryData
                [&Reg, &v]( std::vector<Byte> const & Val ) {
                    Reg.WriteBinaryData(
                        Format(
                            _D( "%s:(" ) cnv_xstr( TT_VB ) ")",
                            ARRAYOFCONST(( v.first ))
                        ),
                        Val
                    );
                }
            },
            v.second.first
        );
    }
};

//---------------------------------------------------------------------------
} // End namespace Registry
//---------------------------------------------------------------------------
} // End of namespace Anafestica
//---------------------------------------------------------------------------

#endif
