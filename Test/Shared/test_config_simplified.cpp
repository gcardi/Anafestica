//---------------------------------------------------------------------------
// Simplified TConfig roundtrip tests for the 19-type subset shared by all
// three toolchains. The std::variant-only std::string/std::wstring coverage
// remains in test_config.cpp.
//---------------------------------------------------------------------------

#pragma hdrstop

//---------------------------------------------------------------------------
#pragma package(smart_init)

#include <boost/test/unit_test.hpp>

#include <anafestica/CfgItems.h>
#include <anafestica/CfgRegistry.h>
#include <anafestica/CfgJSON.h>
#include <anafestica/CfgXML.h>
#include <anafestica/CfgIniFile.h>

#include <System.SysUtils.hpp>

namespace {

using StringCont = std::vector<System::UnicodeString>;
using BytesCont  = std::vector<unsigned char>;

// Test constants for the types common to all three toolchains
constexpr int                kI   = 42;
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
inline System::TDateTime     kDT()  { return EncodeDate( 2025, 6, 15 ); }
constexpr float              kFLT = 61.34F;
constexpr double             kDBL = 3.141592653589793;
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

bool DABEqual( System::Sysutils::TBytes const& a,
               System::Sysutils::TBytes const& b ) noexcept
{
    if ( a.Length != b.Length ) return false;
    for ( int i = 0 ; i < a.Length ; ++i )
        if ( a[i] != b[i] ) return false;
    return true;
}

#if defined( ANAFESTICA_USE_STD_VARIANT )
const String RegCfgRoot = L"Software\\Anafestica\\TestBcc64x\\ConfigSimplified";
#elif defined( _WIN64 )
const String RegCfgRoot = L"Software\\Anafestica\\TestBcc64\\Config";
#else
const String RegCfgRoot = L"Software\\Anafestica\\TestBcc32c\\Config";
#endif
static int   RegCfgSeq  = 0;

String MakeRegCfgKey() {
    return RegCfgRoot + L"\\Cfg_" + IntToStr( ++RegCfgSeq );
}

struct ScopedRegKey {
    String path;
    explicit ScopedRegKey( String const& p ) : path( p ) {}
    ~ScopedRegKey() noexcept {
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
    }
    ~RegistryCfgFixture() noexcept {
        try {
            auto r = std::make_unique<Anafestica::Registry::TRegistry>();
            r->RootKey = HKEY_CURRENT_USER;
            r->DeleteKey( RegCfgRoot );
        } catch ( ... ) {}
    }
};

}

//---------------------------------------------------------------------------
// *** Registry::TConfig basic roundtrip tests (common-type subset) ***
//---------------------------------------------------------------------------

BOOST_FIXTURE_TEST_SUITE( TConfig_Registry_Simplified, RegistryCfgFixture )

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
    BOOST_TEST( c.GetRootNode().GetItem<System::TDateTime>( L"val" ) == dt );
}

BOOST_AUTO_TEST_CASE( Registry_float_roundtrip )
{
    const auto key = MakeRegCfgKey(); ScopedRegKey g( key );
    { Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
      c.GetRootNode().PutItem( L"val", kFLT ); }
    Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
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
    auto retrieved = c.GetRootNode().GetItem<BytesCont>( L"val" );
    BOOST_TEST( retrieved == vb );
}

BOOST_AUTO_TEST_SUITE_END()
