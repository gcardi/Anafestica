//---------------------------------------------------------------------------
// TConfig roundtrip tests for all three backends (Registry, JSON, XML)
// covering all 21 types in TConfigNodeValueType.
//---------------------------------------------------------------------------

#pragma hdrstop

//---------------------------------------------------------------------------
#pragma package(smart_init)

#include <boost/test/unit_test.hpp>

#include <memory>
#include <vector>
#include <string>
#include <string_view>
#include <cmath>

#include <windows.h>
#include <objbase.h>

#include <anafestica/CfgRegistry.h>
#include <anafestica/CfgJSON.h>
#include <anafestica/CfgXML.h>
#include <anafestica/CfgNodeValueType.h>

#include <System.Classes.hpp>
#include <System.SysUtils.hpp>
#include <System.Win.Registry.hpp>
#include <System.IOUtils.hpp>
#include <System.DateUtils.hpp>

//---------------------------------------------------------------------------

namespace {

using Anafestica::StringCont;
using Anafestica::BytesCont;

// Set to true via  -- --keep-artifacts  on the command line.
// When true, temp files and registry keys are NOT deleted after each test.
static bool gKeepArtifacts = false;

//---------------------------------------------------------------------------
// Test values – one for each of the 19 TConfigNodeValueType alternatives
//---------------------------------------------------------------------------

constexpr int                kI   = -42;
constexpr unsigned int       kU   = 100U;
constexpr long               kL   = -1000L;
constexpr unsigned long      kUL  = 2000UL;
constexpr char               kC   = 'Z';
constexpr unsigned char      kUC  = 0xABu;
constexpr short              kS   = -32000;
constexpr unsigned short     kUS  = 60000U;
constexpr long long          kLL  = -9000000000LL;
constexpr unsigned long long kULL = 0x1234567890ABCDEFULL;
constexpr bool               kB   = true;
const     wchar_t* const     kSZ  = L"Hello Anafestica";
// Pure date (no time) so ISO-8601 roundtrip is exact across all backends.
inline System::TDateTime     kDT()  { return EncodeDate( 2025, 6, 15 ); }
constexpr float              kFLT = 61.34F;
constexpr double             kDBL = 3.141592653589793;
// Currency value that is exactly representable to 4 decimal places.
inline System::Currency      kCUR() { return System::Currency( 1200.45 ); }

inline StringCont MakeSV() {
    return { L"Uno", L"Due", L"Tre" };
}

inline System::Sysutils::TBytes MakeDAB() {
    System::Sysutils::TBytes b;
    b.Length = 4;
    b[0] = 0x45; b[1] = 0x56; b[2] = 0x67; b[3] = 0x78;
    return b;
}

inline BytesCont MakeVB() {
    return { 0xFE, 0xCA, 0x39, 0x43 };
}

// std::string (UTF-8) and std::wstring (UTF-16) test values
const std::string  kSTR  = "Hello Anafestica (UTF-8)";
const std::wstring kWSTR = L"Hello Anafestica (Wide)";

bool DABEqual( System::Sysutils::TBytes const& a,
               System::Sysutils::TBytes const& b ) noexcept
{
    if ( a.Length != b.Length ) return false;
    for ( int i = 0 ; i < a.Length ; ++i )
        if ( a[i] != b[i] ) return false;
    return true;
}

//---------------------------------------------------------------------------
// Registry-specific helpers
//---------------------------------------------------------------------------

const String RegCfgRoot = L"Software\\Anafestica\\TestSuite\\TConfig";
static int   RegCfgSeq  = 0;

String MakeRegCfgKey() {
    return RegCfgRoot + L"\\Cfg_" + IntToStr( ++RegCfgSeq );
}

struct ScopedRegKey {
    String path;
    explicit ScopedRegKey( String const& p ) : path( p ) {}
    ~ScopedRegKey() noexcept {
        if ( gKeepArtifacts ) return;
        try {
            auto r = std::make_unique<Anafestica::Registry::TRegistry>();
            r->RootKey = HKEY_CURRENT_USER;
            r->DeleteKey( path );
        } catch ( ... ) {}
    }
};

struct RegistryCfgFixture {
    RegistryCfgFixture() {
        auto r = std::make_unique<Anafestica::Registry::TRegistry>();
        r->RootKey = HKEY_CURRENT_USER;
        if ( !r->OpenKey( RegCfgRoot, true ) )
            throw Exception( _D( "Cannot create TConfig registry test root" ) );
        r->CloseKey();
        if ( gKeepArtifacts )
            std::cout << "[keep-artifacts] registry root: HKCU\\" << AnsiString( RegCfgRoot ).c_str() << "\n";
    }
    ~RegistryCfgFixture() noexcept {
        if ( gKeepArtifacts ) return;
        try {
            auto r = std::make_unique<Anafestica::Registry::TRegistry>();
            r->RootKey = HKEY_CURRENT_USER;
            r->DeleteKey( RegCfgRoot );
        } catch ( ... ) {}
    }
};

//---------------------------------------------------------------------------
// Temp-file helpers (JSON / XML backends)
//---------------------------------------------------------------------------

static int TmpFileSeq = 0;

String MakeTempPath( String const& suffix ) {
    return TPath::GetTempPath()
        + _D( "anafestica_tcfg_" ) + IntToStr( ++TmpFileSeq ) + suffix;
}

struct TempFileGuard {
    String path;
    explicit TempFileGuard( String const& p ) : path( p ) {
        if ( gKeepArtifacts )
            std::wcout << L"[keep-artifacts] " << path.c_str() << L"\n";
    }
    ~TempFileGuard() noexcept {
        if ( gKeepArtifacts ) return;
        try { if ( TFile::Exists( path ) ) TFile::Delete( path ); } catch ( ... ) {}
    }
};

// Global fixture: parse --keep-artifacts from user arguments (pass after --).
struct ArtifactSetup {
    ArtifactSetup() {
        auto& mts = boost::unit_test::framework::master_test_suite();
        for ( int i = 1 ; i < mts.argc ; ++i ) {
            if ( std::string( mts.argv[i] ) == "--keep-artifacts" ) {
                gKeepArtifacts = true;
                std::cout << "[keep-artifacts] temp files and registry keys will NOT be deleted.\n";
                break;
            }
        }
    }
};
BOOST_GLOBAL_FIXTURE( ArtifactSetup );

// XML COM fixture – called once per XML test case.
// CoUninitialize is intentionally omitted: the Embarcadero RTL performs its
// own COM teardown at process exit; an explicit unbalanced call here would
// crash the RTL cleanup code.
struct XMLCOMFixture {
    XMLCOMFixture() { ::CoInitializeEx( nullptr, COINIT_MULTITHREADED ); }
};

//---------------------------------------------------------------------------
// *** Registry::TConfig tests ***
//---------------------------------------------------------------------------

BOOST_FIXTURE_TEST_SUITE( TConfig_Registry, RegistryCfgFixture )

BOOST_AUTO_TEST_CASE( Registry_int_roundtrip )
{
    const auto key = MakeRegCfgKey(); ScopedRegKey g( key );
    { Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
      c.GetRootNode().PutItem( L"val", kI ); }
    Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
    BOOST_TEST( c.GetRootNode().GetItem<int>( L"val" ) == kI );
}

BOOST_AUTO_TEST_CASE( Registry_uint_roundtrip )
{
    const auto key = MakeRegCfgKey(); ScopedRegKey g( key );
    { Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
      c.GetRootNode().PutItem( L"val", kU ); }
    Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
    BOOST_TEST( c.GetRootNode().GetItem<unsigned int>( L"val" ) == kU );
}

BOOST_AUTO_TEST_CASE( Registry_long_roundtrip )
{
    const auto key = MakeRegCfgKey(); ScopedRegKey g( key );
    { Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
      c.GetRootNode().PutItem( L"val", kL ); }
    Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
    BOOST_TEST( c.GetRootNode().GetItem<long>( L"val" ) == kL );
}

BOOST_AUTO_TEST_CASE( Registry_ulong_roundtrip )
{
    const auto key = MakeRegCfgKey(); ScopedRegKey g( key );
    { Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
      c.GetRootNode().PutItem( L"val", kUL ); }
    Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
    BOOST_TEST( c.GetRootNode().GetItem<unsigned long>( L"val" ) == kUL );
}

BOOST_AUTO_TEST_CASE( Registry_char_roundtrip )
{
    const auto key = MakeRegCfgKey(); ScopedRegKey g( key );
    { Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
      c.GetRootNode().PutItem( L"val", kC ); }
    Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
    BOOST_TEST( c.GetRootNode().GetItem<char>( L"val" ) == kC );
}

BOOST_AUTO_TEST_CASE( Registry_uchar_roundtrip )
{
    const auto key = MakeRegCfgKey(); ScopedRegKey g( key );
    { Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
      c.GetRootNode().PutItem( L"val", kUC ); }
    Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
    BOOST_TEST( c.GetRootNode().GetItem<unsigned char>( L"val" ) == kUC );
}

BOOST_AUTO_TEST_CASE( Registry_short_roundtrip )
{
    const auto key = MakeRegCfgKey(); ScopedRegKey g( key );
    { Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
      c.GetRootNode().PutItem( L"val", kS ); }
    Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
    BOOST_TEST( c.GetRootNode().GetItem<short>( L"val" ) == kS );
}

BOOST_AUTO_TEST_CASE( Registry_ushort_roundtrip )
{
    const auto key = MakeRegCfgKey(); ScopedRegKey g( key );
    { Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
      c.GetRootNode().PutItem( L"val", kUS ); }
    Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
    BOOST_TEST( c.GetRootNode().GetItem<unsigned short>( L"val" ) == kUS );
}

BOOST_AUTO_TEST_CASE( Registry_longlong_roundtrip )
{
    const auto key = MakeRegCfgKey(); ScopedRegKey g( key );
    { Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
      c.GetRootNode().PutItem( L"val", kLL ); }
    Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
    BOOST_TEST( c.GetRootNode().GetItem<long long>( L"val" ) == kLL );
}

BOOST_AUTO_TEST_CASE( Registry_ulonglong_roundtrip )
{
    const auto key = MakeRegCfgKey(); ScopedRegKey g( key );
    { Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
      c.GetRootNode().PutItem( L"val", kULL ); }
    Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
    BOOST_TEST( c.GetRootNode().GetItem<unsigned long long>( L"val" ) == kULL );
}

BOOST_AUTO_TEST_CASE( Registry_bool_roundtrip )
{
    const auto key = MakeRegCfgKey(); ScopedRegKey g( key );
    { Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
      c.GetRootNode().PutItem( L"val", kB ); }
    Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
    BOOST_TEST( c.GetRootNode().GetItem<bool>( L"val" ) == kB );
}

BOOST_AUTO_TEST_CASE( Registry_string_roundtrip )
{
    const auto key = MakeRegCfgKey(); ScopedRegKey g( key );
    { Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
      c.GetRootNode().PutItem( L"val", String( kSZ ) ); }
    Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
    BOOST_TEST( c.GetRootNode().GetItem<String>( L"val" ) == String( kSZ ) );
}

BOOST_AUTO_TEST_CASE( Registry_datetime_roundtrip )
{
    const auto key = MakeRegCfgKey(); ScopedRegKey g( key );
    const auto dt = kDT();
    { Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
      c.GetRootNode().PutItem( L"val", dt ); }
    Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
    // Registry stores TDateTime as binary double – exact roundtrip.
    BOOST_TEST( c.GetRootNode().GetItem<System::TDateTime>( L"val" ) == dt );
}

BOOST_AUTO_TEST_CASE( Registry_float_roundtrip )
{
    const auto key = MakeRegCfgKey(); ScopedRegKey g( key );
    { Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
      c.GetRootNode().PutItem( L"val", kFLT ); }
    Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
    // Registry stores float via WriteFloat/ReadFloat (binary) – exact roundtrip.
    BOOST_TEST( c.GetRootNode().GetItem<float>( L"val" ) == kFLT );
}

BOOST_AUTO_TEST_CASE( Registry_double_roundtrip )
{
    const auto key = MakeRegCfgKey(); ScopedRegKey g( key );
    { Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
      c.GetRootNode().PutItem( L"val", kDBL ); }
    Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
    BOOST_TEST( c.GetRootNode().GetItem<double>( L"val" ) == kDBL );
}

BOOST_AUTO_TEST_CASE( Registry_currency_roundtrip )
{
    const auto key = MakeRegCfgKey(); ScopedRegKey g( key );
    const auto cur = kCUR();
    { Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
      c.GetRootNode().PutItem( L"val", cur ); }
    Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
    BOOST_TEST( c.GetRootNode().GetItem<System::Currency>( L"val" ) == cur );
}

BOOST_AUTO_TEST_CASE( Registry_stringvec_roundtrip )
{
    const auto key = MakeRegCfgKey(); ScopedRegKey g( key );
    const auto sv = MakeSV();
    { Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
      c.GetRootNode().PutItem( L"val", sv ); }
    Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
    BOOST_TEST( c.GetRootNode().GetItem<StringCont>( L"val" ) == sv );
}

BOOST_AUTO_TEST_CASE( Registry_tbytes_roundtrip )
{
    const auto key = MakeRegCfgKey(); ScopedRegKey g( key );
    const auto dab = MakeDAB();
    { Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
      c.GetRootNode().PutItem( L"val", dab ); }
    Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
    BOOST_TEST( DABEqual( c.GetRootNode().GetItem<System::Sysutils::TBytes>( L"val" ), dab ) );
}

BOOST_AUTO_TEST_CASE( Registry_bytevec_roundtrip )
{
    const auto key = MakeRegCfgKey(); ScopedRegKey g( key );
    const auto vb = MakeVB();
    { Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
      c.GetRootNode().PutItem( L"val", vb ); }
    Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
    BOOST_TEST( c.GetRootNode().GetItem<BytesCont>( L"val" ) == vb );
}

BOOST_AUTO_TEST_CASE( Registry_stdstring_roundtrip )
{
    const auto key = MakeRegCfgKey(); ScopedRegKey g( key );
    { Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
      c.GetRootNode().PutItem( L"val", kSTR ); }
    Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
    BOOST_TEST( c.GetRootNode().GetItem<std::string>( L"val" ) == kSTR );
}

BOOST_AUTO_TEST_CASE( Registry_wstring_roundtrip )
{
    const auto key = MakeRegCfgKey(); ScopedRegKey g( key );
    { Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
      c.GetRootNode().PutItem( L"val", kWSTR ); }
    Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
    BOOST_CHECK( c.GetRootNode().GetItem<std::wstring>( L"val" ) == kWSTR );
}

BOOST_AUTO_TEST_CASE( Registry_string_view_write )
{
    // string_view / wstring_view are materialised to string / wstring on write;
    // read back confirms the stored value.
    const auto key = MakeRegCfgKey(); ScopedRegKey g( key );
    std::string_view  sv  = kSTR;
    std::wstring_view wsv = kWSTR;
    { Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
      c.GetRootNode().PutItem( L"sv",  sv  );
      c.GetRootNode().PutItem( L"wsv", wsv ); }
    Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
    BOOST_TEST( c.GetRootNode().GetItem<std::string> ( L"sv"  ) == kSTR  );
    BOOST_CHECK( c.GetRootNode().GetItem<std::wstring>( L"wsv" ) == kWSTR );
}

BOOST_AUTO_TEST_CASE( Registry_subnode_roundtrip )
{
    const auto key = MakeRegCfgKey(); ScopedRegKey g( key );
    { Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
      c.GetRootNode()[L"Child"][L"GrandChild"].PutItem( L"n", kI ); }
    Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
    BOOST_TEST( c.GetRootNode()[L"Child"][L"GrandChild"].GetItem<int>( L"n" ) == kI );
}

BOOST_AUTO_TEST_SUITE_END()


//---------------------------------------------------------------------------
// *** JSON::TConfig tests ***
//---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE( TConfig_JSON )

BOOST_AUTO_TEST_CASE( JSON_int_roundtrip )
{
    const auto f = MakeTempPath( L".json" ); TempFileGuard g( f );
    { Anafestica::JSON::TConfig c( f );
      c.GetRootNode().PutItem( L"val", kI ); }
    Anafestica::JSON::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<int>( L"val" ) == kI );
}

BOOST_AUTO_TEST_CASE( JSON_uint_roundtrip )
{
    const auto f = MakeTempPath( L".json" ); TempFileGuard g( f );
    { Anafestica::JSON::TConfig c( f );
      c.GetRootNode().PutItem( L"val", kU ); }
    Anafestica::JSON::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<unsigned int>( L"val" ) == kU );
}

BOOST_AUTO_TEST_CASE( JSON_long_roundtrip )
{
    const auto f = MakeTempPath( L".json" ); TempFileGuard g( f );
    { Anafestica::JSON::TConfig c( f );
      c.GetRootNode().PutItem( L"val", kL ); }
    Anafestica::JSON::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<long>( L"val" ) == kL );
}

BOOST_AUTO_TEST_CASE( JSON_ulong_roundtrip )
{
    const auto f = MakeTempPath( L".json" ); TempFileGuard g( f );
    { Anafestica::JSON::TConfig c( f );
      c.GetRootNode().PutItem( L"val", kUL ); }
    Anafestica::JSON::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<unsigned long>( L"val" ) == kUL );
}

BOOST_AUTO_TEST_CASE( JSON_char_roundtrip )
{
    const auto f = MakeTempPath( L".json" ); TempFileGuard g( f );
    { Anafestica::JSON::TConfig c( f );
      c.GetRootNode().PutItem( L"val", kC ); }
    Anafestica::JSON::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<char>( L"val" ) == kC );
}

BOOST_AUTO_TEST_CASE( JSON_uchar_roundtrip )
{
    const auto f = MakeTempPath( L".json" ); TempFileGuard g( f );
    { Anafestica::JSON::TConfig c( f );
      c.GetRootNode().PutItem( L"val", kUC ); }
    Anafestica::JSON::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<unsigned char>( L"val" ) == kUC );
}

BOOST_AUTO_TEST_CASE( JSON_short_roundtrip )
{
    const auto f = MakeTempPath( L".json" ); TempFileGuard g( f );
    { Anafestica::JSON::TConfig c( f );
      c.GetRootNode().PutItem( L"val", kS ); }
    Anafestica::JSON::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<short>( L"val" ) == kS );
}

BOOST_AUTO_TEST_CASE( JSON_ushort_roundtrip )
{
    const auto f = MakeTempPath( L".json" ); TempFileGuard g( f );
    { Anafestica::JSON::TConfig c( f );
      c.GetRootNode().PutItem( L"val", kUS ); }
    Anafestica::JSON::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<unsigned short>( L"val" ) == kUS );
}

BOOST_AUTO_TEST_CASE( JSON_longlong_roundtrip )
{
    const auto f = MakeTempPath( L".json" ); TempFileGuard g( f );
    { Anafestica::JSON::TConfig c( f );
      c.GetRootNode().PutItem( L"val", kLL ); }
    Anafestica::JSON::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<long long>( L"val" ) == kLL );
}

BOOST_AUTO_TEST_CASE( JSON_ulonglong_roundtrip )
{
    const auto f = MakeTempPath( L".json" ); TempFileGuard g( f );
    { Anafestica::JSON::TConfig c( f );
      c.GetRootNode().PutItem( L"val", kULL ); }
    Anafestica::JSON::TConfig c( f );
    // ULL is serialised as a decimal string in JSON to avoid 64-bit precision loss.
    BOOST_TEST( c.GetRootNode().GetItem<unsigned long long>( L"val" ) == kULL );
}

BOOST_AUTO_TEST_CASE( JSON_bool_roundtrip )
{
    const auto f = MakeTempPath( L".json" ); TempFileGuard g( f );
    { Anafestica::JSON::TConfig c( f );
      c.GetRootNode().PutItem( L"val", kB ); }
    Anafestica::JSON::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<bool>( L"val" ) == kB );
}

BOOST_AUTO_TEST_CASE( JSON_string_roundtrip )
{
    const auto f = MakeTempPath( L".json" ); TempFileGuard g( f );
    { Anafestica::JSON::TConfig c( f );
      c.GetRootNode().PutItem( L"val", String( kSZ ) ); }
    Anafestica::JSON::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<String>( L"val" ) == String( kSZ ) );
}

BOOST_AUTO_TEST_CASE( JSON_datetime_roundtrip )
{
    const auto f = MakeTempPath( L".json" ); TempFileGuard g( f );
    const auto dt = kDT();
    { Anafestica::JSON::TConfig c( f );
      c.GetRootNode().PutItem( L"val", dt ); }
    Anafestica::JSON::TConfig c( f );
    // ISO-8601 roundtrip – compare date portion only (pure-date test value).
    const auto got = c.GetRootNode().GetItem<System::TDateTime>( L"val" );
    BOOST_TEST( DateOf( got ) == DateOf( dt ) );
}

BOOST_AUTO_TEST_CASE( JSON_float_roundtrip )
{
    const auto f = MakeTempPath( L".json" ); TempFileGuard g( f );
    { Anafestica::JSON::TConfig c( f );
      c.GetRootNode().PutItem( L"val", kFLT ); }
    Anafestica::JSON::TConfig c( f );
    // JSON serialises float as double; tolerate small rounding.
    const float got = c.GetRootNode().GetItem<float>( L"val" );
    BOOST_TEST( std::abs( got - kFLT ) < 1e-3F );
}

BOOST_AUTO_TEST_CASE( JSON_double_roundtrip )
{
    const auto f = MakeTempPath( L".json" ); TempFileGuard g( f );
    { Anafestica::JSON::TConfig c( f );
      c.GetRootNode().PutItem( L"val", kDBL ); }
    Anafestica::JSON::TConfig c( f );
    // TJSONNumber serialises to ~15 significant digits; use a small tolerance.
    const double got = c.GetRootNode().GetItem<double>( L"val" );
    BOOST_TEST( std::abs( got - kDBL ) < 1e-12 );
}

BOOST_AUTO_TEST_CASE( JSON_currency_roundtrip )
{
    const auto f = MakeTempPath( L".json" ); TempFileGuard g( f );
    const auto cur = kCUR();
    { Anafestica::JSON::TConfig c( f );
      c.GetRootNode().PutItem( L"val", cur ); }
    Anafestica::JSON::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<System::Currency>( L"val" ) == cur );
}

BOOST_AUTO_TEST_CASE( JSON_stringvec_roundtrip )
{
    const auto f = MakeTempPath( L".json" ); TempFileGuard g( f );
    const auto sv = MakeSV();
    { Anafestica::JSON::TConfig c( f );
      c.GetRootNode().PutItem( L"val", sv ); }
    Anafestica::JSON::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<StringCont>( L"val" ) == sv );
}

BOOST_AUTO_TEST_CASE( JSON_tbytes_roundtrip )
{
    const auto f = MakeTempPath( L".json" ); TempFileGuard g( f );
    const auto dab = MakeDAB();
    { Anafestica::JSON::TConfig c( f );
      c.GetRootNode().PutItem( L"val", dab ); }
    Anafestica::JSON::TConfig c( f );
    BOOST_TEST( DABEqual( c.GetRootNode().GetItem<System::Sysutils::TBytes>( L"val" ), dab ) );
}

BOOST_AUTO_TEST_CASE( JSON_bytevec_roundtrip )
{
    const auto f = MakeTempPath( L".json" ); TempFileGuard g( f );
    const auto vb = MakeVB();
    { Anafestica::JSON::TConfig c( f );
      c.GetRootNode().PutItem( L"val", vb ); }
    Anafestica::JSON::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<BytesCont>( L"val" ) == vb );
}

BOOST_AUTO_TEST_CASE( JSON_stdstring_roundtrip )
{
    const auto f = MakeTempPath( L".json" ); TempFileGuard g( f );
    { Anafestica::JSON::TConfig c( f );
      c.GetRootNode().PutItem( L"val", kSTR ); }
    Anafestica::JSON::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<std::string>( L"val" ) == kSTR );
}

BOOST_AUTO_TEST_CASE( JSON_wstring_roundtrip )
{
    const auto f = MakeTempPath( L".json" ); TempFileGuard g( f );
    { Anafestica::JSON::TConfig c( f );
      c.GetRootNode().PutItem( L"val", kWSTR ); }
    Anafestica::JSON::TConfig c( f );
    BOOST_CHECK( c.GetRootNode().GetItem<std::wstring>( L"val" ) == kWSTR );
}

BOOST_AUTO_TEST_CASE( JSON_string_view_write )
{
    const auto f = MakeTempPath( L".json" ); TempFileGuard g( f );
    std::string_view  sv  = kSTR;
    std::wstring_view wsv = kWSTR;
    { Anafestica::JSON::TConfig c( f );
      c.GetRootNode().PutItem( L"sv",  sv  );
      c.GetRootNode().PutItem( L"wsv", wsv ); }
    Anafestica::JSON::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<std::string> ( L"sv"  ) == kSTR  );
    BOOST_CHECK( c.GetRootNode().GetItem<std::wstring>( L"wsv" ) == kWSTR );
}

BOOST_AUTO_TEST_CASE( JSON_subnode_roundtrip )
{
    const auto f = MakeTempPath( L".json" ); TempFileGuard g( f );
    { Anafestica::JSON::TConfig c( f );
      c.GetRootNode()[L"Child"][L"GrandChild"].PutItem( L"n", kI ); }
    Anafestica::JSON::TConfig c( f );
    BOOST_TEST( c.GetRootNode()[L"Child"][L"GrandChild"].GetItem<int>( L"n" ) == kI );
}

BOOST_AUTO_TEST_CASE( JSON_explicit_types_roundtrip )
{
    // With ExplicitTypes=true all values are wrapped with type tag objects.
    const auto f = MakeTempPath( L".json" ); TempFileGuard g( f );
    { Anafestica::JSON::TConfig c( f, /*ReadOnly*/false, /*Compact*/true,
                                   /*FlushAllItems*/false, /*ExplicitTypes*/true );
      c.GetRootNode().PutItem( L"i",  kI   );
      c.GetRootNode().PutItem( L"b",  kB   );
      c.GetRootNode().PutItem( L"sz", String( kSZ ) ); }
    Anafestica::JSON::TConfig c( f, false, true, false, true );
    BOOST_TEST( c.GetRootNode().GetItem<int>( L"i" )    == kI   );
    BOOST_TEST( c.GetRootNode().GetItem<bool>( L"b" )   == kB   );
    BOOST_TEST( c.GetRootNode().GetItem<String>( L"sz" ) == String( kSZ ) );
}

BOOST_AUTO_TEST_SUITE_END()


//---------------------------------------------------------------------------
// *** XML::TConfig tests ***
//---------------------------------------------------------------------------

BOOST_FIXTURE_TEST_SUITE( TConfig_XML, XMLCOMFixture )

BOOST_AUTO_TEST_CASE( XML_int_roundtrip )
{
    const auto f = MakeTempPath( L".xml" ); TempFileGuard g( f );
    { Anafestica::XML::TConfig c( f );
      c.GetRootNode().PutItem( L"val", kI ); }
    Anafestica::XML::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<int>( L"val" ) == kI );
}

BOOST_AUTO_TEST_CASE( XML_uint_roundtrip )
{
    const auto f = MakeTempPath( L".xml" ); TempFileGuard g( f );
    { Anafestica::XML::TConfig c( f );
      c.GetRootNode().PutItem( L"val", kU ); }
    Anafestica::XML::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<unsigned int>( L"val" ) == kU );
}

BOOST_AUTO_TEST_CASE( XML_long_roundtrip )
{
    const auto f = MakeTempPath( L".xml" ); TempFileGuard g( f );
    { Anafestica::XML::TConfig c( f );
      c.GetRootNode().PutItem( L"val", kL ); }
    Anafestica::XML::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<long>( L"val" ) == kL );
}

BOOST_AUTO_TEST_CASE( XML_ulong_roundtrip )
{
    const auto f = MakeTempPath( L".xml" ); TempFileGuard g( f );
    { Anafestica::XML::TConfig c( f );
      c.GetRootNode().PutItem( L"val", kUL ); }
    Anafestica::XML::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<unsigned long>( L"val" ) == kUL );
}

BOOST_AUTO_TEST_CASE( XML_char_roundtrip )
{
    const auto f = MakeTempPath( L".xml" ); TempFileGuard g( f );
    { Anafestica::XML::TConfig c( f );
      c.GetRootNode().PutItem( L"val", kC ); }
    Anafestica::XML::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<char>( L"val" ) == kC );
}

BOOST_AUTO_TEST_CASE( XML_uchar_roundtrip )
{
    const auto f = MakeTempPath( L".xml" ); TempFileGuard g( f );
    { Anafestica::XML::TConfig c( f );
      c.GetRootNode().PutItem( L"val", kUC ); }
    Anafestica::XML::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<unsigned char>( L"val" ) == kUC );
}

BOOST_AUTO_TEST_CASE( XML_short_roundtrip )
{
    const auto f = MakeTempPath( L".xml" ); TempFileGuard g( f );
    { Anafestica::XML::TConfig c( f );
      c.GetRootNode().PutItem( L"val", kS ); }
    Anafestica::XML::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<short>( L"val" ) == kS );
}

BOOST_AUTO_TEST_CASE( XML_ushort_roundtrip )
{
    const auto f = MakeTempPath( L".xml" ); TempFileGuard g( f );
    { Anafestica::XML::TConfig c( f );
      c.GetRootNode().PutItem( L"val", kUS ); }
    Anafestica::XML::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<unsigned short>( L"val" ) == kUS );
}

BOOST_AUTO_TEST_CASE( XML_longlong_roundtrip )
{
    const auto f = MakeTempPath( L".xml" ); TempFileGuard g( f );
    { Anafestica::XML::TConfig c( f );
      c.GetRootNode().PutItem( L"val", kLL ); }
    Anafestica::XML::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<long long>( L"val" ) == kLL );
}

BOOST_AUTO_TEST_CASE( XML_ulonglong_roundtrip )
{
    const auto f = MakeTempPath( L".xml" ); TempFileGuard g( f );
    { Anafestica::XML::TConfig c( f );
      c.GetRootNode().PutItem( L"val", kULL ); }
    Anafestica::XML::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<unsigned long long>( L"val" ) == kULL );
}

BOOST_AUTO_TEST_CASE( XML_bool_roundtrip )
{
    const auto f = MakeTempPath( L".xml" ); TempFileGuard g( f );
    { Anafestica::XML::TConfig c( f );
      c.GetRootNode().PutItem( L"val", kB ); }
    Anafestica::XML::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<bool>( L"val" ) == kB );
}

BOOST_AUTO_TEST_CASE( XML_string_roundtrip )
{
    const auto f = MakeTempPath( L".xml" ); TempFileGuard g( f );
    { Anafestica::XML::TConfig c( f );
      c.GetRootNode().PutItem( L"val", String( kSZ ) ); }
    Anafestica::XML::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<String>( L"val" ) == String( kSZ ) );
}

BOOST_AUTO_TEST_CASE( XML_datetime_roundtrip )
{
    const auto f = MakeTempPath( L".xml" ); TempFileGuard g( f );
    const auto dt = kDT();
    { Anafestica::XML::TConfig c( f );
      c.GetRootNode().PutItem( L"val", dt ); }
    Anafestica::XML::TConfig c( f );
    // ISO-8601 roundtrip – compare date portion only.
    const auto got = c.GetRootNode().GetItem<System::TDateTime>( L"val" );
    BOOST_TEST( DateOf( got ) == DateOf( dt ) );
}

BOOST_AUTO_TEST_CASE( XML_float_roundtrip )
{
    const auto f = MakeTempPath( L".xml" ); TempFileGuard g( f );
    { Anafestica::XML::TConfig c( f );
      c.GetRootNode().PutItem( L"val", kFLT ); }
    Anafestica::XML::TConfig c( f );
    const float got = c.GetRootNode().GetItem<float>( L"val" );
    BOOST_TEST( std::abs( got - kFLT ) < 1e-3F );
}

BOOST_AUTO_TEST_CASE( XML_double_roundtrip )
{
    const auto f = MakeTempPath( L".xml" ); TempFileGuard g( f );
    { Anafestica::XML::TConfig c( f );
      c.GetRootNode().PutItem( L"val", kDBL ); }
    Anafestica::XML::TConfig c( f );
    // FloatToStrF with 17 digits → std::stod: tolerate last-bit rounding.
    const double got = c.GetRootNode().GetItem<double>( L"val" );
    BOOST_TEST( std::abs( got - kDBL ) < 1e-12 );
}

BOOST_AUTO_TEST_CASE( XML_currency_roundtrip )
{
    const auto f = MakeTempPath( L".xml" ); TempFileGuard g( f );
    const auto cur = kCUR();
    { Anafestica::XML::TConfig c( f );
      c.GetRootNode().PutItem( L"val", cur ); }
    Anafestica::XML::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<System::Currency>( L"val" ) == cur );
}

BOOST_AUTO_TEST_CASE( XML_stringvec_roundtrip )
{
    const auto f = MakeTempPath( L".xml" ); TempFileGuard g( f );
    const auto sv = MakeSV();
    { Anafestica::XML::TConfig c( f );
      c.GetRootNode().PutItem( L"val", sv ); }
    Anafestica::XML::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<StringCont>( L"val" ) == sv );
}

BOOST_AUTO_TEST_CASE( XML_tbytes_roundtrip )
{
    const auto f = MakeTempPath( L".xml" ); TempFileGuard g( f );
    const auto dab = MakeDAB();
    { Anafestica::XML::TConfig c( f );
      c.GetRootNode().PutItem( L"val", dab ); }
    Anafestica::XML::TConfig c( f );
    BOOST_TEST( DABEqual( c.GetRootNode().GetItem<System::Sysutils::TBytes>( L"val" ), dab ) );
}

BOOST_AUTO_TEST_CASE( XML_bytevec_roundtrip )
{
    const auto f = MakeTempPath( L".xml" ); TempFileGuard g( f );
    const auto vb = MakeVB();
    { Anafestica::XML::TConfig c( f );
      c.GetRootNode().PutItem( L"val", vb ); }
    Anafestica::XML::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<BytesCont>( L"val" ) == vb );
}

BOOST_AUTO_TEST_CASE( XML_stdstring_roundtrip )
{
    const auto f = MakeTempPath( L".xml" ); TempFileGuard g( f );
    { Anafestica::XML::TConfig c( f );
      c.GetRootNode().PutItem( L"val", kSTR ); }
    Anafestica::XML::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<std::string>( L"val" ) == kSTR );
}

BOOST_AUTO_TEST_CASE( XML_wstring_roundtrip )
{
    const auto f = MakeTempPath( L".xml" ); TempFileGuard g( f );
    { Anafestica::XML::TConfig c( f );
      c.GetRootNode().PutItem( L"val", kWSTR ); }
    Anafestica::XML::TConfig c( f );
    BOOST_CHECK( c.GetRootNode().GetItem<std::wstring>( L"val" ) == kWSTR );
}

BOOST_AUTO_TEST_CASE( XML_string_view_write )
{
    const auto f = MakeTempPath( L".xml" ); TempFileGuard g( f );
    std::string_view  sv  = kSTR;
    std::wstring_view wsv = kWSTR;
    { Anafestica::XML::TConfig c( f );
      c.GetRootNode().PutItem( L"sv",  sv  );
      c.GetRootNode().PutItem( L"wsv", wsv ); }
    Anafestica::XML::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<std::string> ( L"sv"  ) == kSTR  );
    BOOST_CHECK( c.GetRootNode().GetItem<std::wstring>( L"wsv" ) == kWSTR );
}

BOOST_AUTO_TEST_CASE( XML_subnode_roundtrip )
{
    const auto f = MakeTempPath( L".xml" ); TempFileGuard g( f );
    { Anafestica::XML::TConfig c( f );
      c.GetRootNode()[L"Child"][L"GrandChild"].PutItem( L"n", kI ); }
    Anafestica::XML::TConfig c( f );
    BOOST_TEST( c.GetRootNode()[L"Child"][L"GrandChild"].GetItem<int>( L"n" ) == kI );
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace
