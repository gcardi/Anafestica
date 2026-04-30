//---------------------------------------------------------------------------
// TConfig type-mismatch tests for all five backends
// (Registry, JSON, BSON, INI, XML).
//
// Each test stores a value of type A via PutItem<A>(), then reads it back
// with GetItem<B>() where B != A.  Because the variant alternative written
// by the backend (determined by the type suffix) differs from B,
// std::get_if<B> returns nullptr and GetItem silently returns T{}.
//
// Five representative mismatch pairs are tested per backend:
//
//   1. int  -> double    (two numeric types, distinct variant alternatives)
//   2. String -> int     (string stored, code reads as integer)
//   3. int  -> String    (integer stored, code reads as string)
//   4. bool -> int       (bool/int freely convertible in C++ but distinct
//                         variant alternatives)
//   5. double -> float   (closely related FP types, distinct alternatives)
//---------------------------------------------------------------------------

#pragma hdrstop

//---------------------------------------------------------------------------
#pragma package(smart_init)

#include <boost/test/unit_test.hpp>

#include <memory>
#include <string>
#include <cmath>

#include <windows.h>
#include <objbase.h>

#include <anafestica/CfgRegistry.h>
#include <anafestica/CfgJSON.h>
#include <anafestica/CfgBSON.h>
#include <anafestica/CfgXML.h>
#include <anafestica/CfgIniFile.h>

#include <System.SysUtils.hpp>
#include <System.Win.Registry.hpp>
#include <System.IOUtils.hpp>

//---------------------------------------------------------------------------

namespace {

// Mirror the --keep-artifacts flag from test_config.cpp.
// Each TU has its own copy; the global fixture below keeps them in sync.
static bool gKeepArtifacts = false;

struct MismatchArtifactSetup {
    MismatchArtifactSetup() {
        auto& mts = boost::unit_test::framework::master_test_suite();
        for ( int i = 1 ; i < mts.argc ; ++i ) {
            if ( std::string( mts.argv[i] ) == "--keep-artifacts" ) {
                gKeepArtifacts = true;
                break;
            }
        }
    }
};
BOOST_GLOBAL_FIXTURE( MismatchArtifactSetup );

//---------------------------------------------------------------------------
// Registry helpers (separate key space from test_config)
//---------------------------------------------------------------------------

const String MismatchRegRoot =
    L"Software\\Anafestica\\TestSuite\\TypeMismatch";
static int MismatchRegSeq = 0;

String MakeMismatchRegKey() {
    return MismatchRegRoot + L"\\MM_" + IntToStr( ++MismatchRegSeq );
}

struct ScopedMismatchRegKey {
    String path;
    explicit ScopedMismatchRegKey( String const& p ) : path( p ) {}
    ~ScopedMismatchRegKey() noexcept {
        if ( gKeepArtifacts ) return;
        try {
            auto r = std::make_unique<Anafestica::Registry::TRegistry>();
            r->RootKey = HKEY_CURRENT_USER;
            r->DeleteKey( path );
        } catch ( ... ) {}
    }
};

struct MismatchRegistryFixture {
    MismatchRegistryFixture() {
        auto r = std::make_unique<Anafestica::Registry::TRegistry>();
        r->RootKey = HKEY_CURRENT_USER;
        if ( !r->OpenKey( MismatchRegRoot, true ) )
            throw Exception(
                _D( "Cannot create type-mismatch registry test root" ) );
        r->CloseKey();
    }
    ~MismatchRegistryFixture() noexcept {
        if ( gKeepArtifacts ) return;
        try {
            auto r = std::make_unique<Anafestica::Registry::TRegistry>();
            r->RootKey = HKEY_CURRENT_USER;
            r->DeleteKey( MismatchRegRoot );
        } catch ( ... ) {}
    }
};

//---------------------------------------------------------------------------
// Temp-file helpers (separate sequence from test_config)
//---------------------------------------------------------------------------

static int MismatchTmpSeq = 0;

String MakeMismatchTempPath( String const& suffix ) {
    return TPath::GetTempPath()
        + _D( "anafestica_mm_" ) + IntToStr( ++MismatchTmpSeq ) + suffix;
}

struct MismatchTempFileGuard {
    String path;
    explicit MismatchTempFileGuard( String const& p ) : path( p ) {
        if ( gKeepArtifacts )
            std::wcout << L"[keep-artifacts] " << path.c_str() << L"\n";
    }
    ~MismatchTempFileGuard() noexcept {
        if ( gKeepArtifacts ) return;
        try { if ( TFile::Exists( path ) ) TFile::Delete( path ); }
        catch ( ... ) {}
    }
};

// XML COM initialisation (same pattern as test_config)
struct MismatchXMLCOMFixture {
    MismatchXMLCOMFixture() {
        ::CoInitializeEx( nullptr, COINIT_MULTITHREADED );
    }
};

//---------------------------------------------------------------------------
// *** Registry type-mismatch tests ***
//---------------------------------------------------------------------------

BOOST_FIXTURE_TEST_SUITE( TypeMismatch_Registry, MismatchRegistryFixture )

// 1. Store int, read as double
BOOST_AUTO_TEST_CASE( Registry_int_read_as_double )
{
    const auto key = MakeMismatchRegKey();
    ScopedMismatchRegKey g( key );
    { Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
      c.GetRootNode().PutItem( L"val", int( -42 ) ); }
    Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
    BOOST_TEST( c.GetRootNode().GetItem<double>( L"val" ) == double{} );
}

// 2. Store String, read as int
BOOST_AUTO_TEST_CASE( Registry_string_read_as_int )
{
    const auto key = MakeMismatchRegKey();
    ScopedMismatchRegKey g( key );
    { Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
      c.GetRootNode().PutItem( L"val", String( L"Hello" ) ); }
    Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
    BOOST_TEST( c.GetRootNode().GetItem<int>( L"val" ) == int{} );
}

// 3. Store int, read as String
BOOST_AUTO_TEST_CASE( Registry_int_read_as_string )
{
    const auto key = MakeMismatchRegKey();
    ScopedMismatchRegKey g( key );
    { Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
      c.GetRootNode().PutItem( L"val", int( -42 ) ); }
    Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
    BOOST_TEST( c.GetRootNode().GetItem<String>( L"val" ) == String{} );
}

// 4. Store bool, read as int
BOOST_AUTO_TEST_CASE( Registry_bool_read_as_int )
{
    const auto key = MakeMismatchRegKey();
    ScopedMismatchRegKey g( key );
    { Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
      c.GetRootNode().PutItem( L"val", true ); }
    Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
    BOOST_TEST( c.GetRootNode().GetItem<int>( L"val" ) == int{} );
}

// 5. Store double, read as float
BOOST_AUTO_TEST_CASE( Registry_double_read_as_float )
{
    const auto key = MakeMismatchRegKey();
    ScopedMismatchRegKey g( key );
    { Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
      c.GetRootNode().PutItem( L"val", double( 3.141592653589793 ) ); }
    Anafestica::Registry::TConfig c( HKEY_CURRENT_USER, key );
    BOOST_TEST( c.GetRootNode().GetItem<float>( L"val" ) == float{} );
}

BOOST_AUTO_TEST_SUITE_END()


//---------------------------------------------------------------------------
// *** JSON type-mismatch tests ***
//---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE( TypeMismatch_JSON )

// 1. Store int, read as double
BOOST_AUTO_TEST_CASE( JSON_int_read_as_double )
{
    const auto f = MakeMismatchTempPath( L".json" );
    MismatchTempFileGuard g( f );
    { Anafestica::JSON::TConfig c( f );
      c.GetRootNode().PutItem( L"val", int( -42 ) ); }
    Anafestica::JSON::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<double>( L"val" ) == double{} );
}

// 2. Store String, read as int
BOOST_AUTO_TEST_CASE( JSON_string_read_as_int )
{
    const auto f = MakeMismatchTempPath( L".json" );
    MismatchTempFileGuard g( f );
    { Anafestica::JSON::TConfig c( f );
      c.GetRootNode().PutItem( L"val", String( L"Hello" ) ); }
    Anafestica::JSON::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<int>( L"val" ) == int{} );
}

// 3. Store int, read as String
BOOST_AUTO_TEST_CASE( JSON_int_read_as_string )
{
    const auto f = MakeMismatchTempPath( L".json" );
    MismatchTempFileGuard g( f );
    { Anafestica::JSON::TConfig c( f );
      c.GetRootNode().PutItem( L"val", int( -42 ) ); }
    Anafestica::JSON::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<String>( L"val" ) == String{} );
}

// 4. Store bool, read as int
BOOST_AUTO_TEST_CASE( JSON_bool_read_as_int )
{
    const auto f = MakeMismatchTempPath( L".json" );
    MismatchTempFileGuard g( f );
    { Anafestica::JSON::TConfig c( f );
      c.GetRootNode().PutItem( L"val", true ); }
    Anafestica::JSON::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<int>( L"val" ) == int{} );
}

// 5. Store double, read as float
BOOST_AUTO_TEST_CASE( JSON_double_read_as_float )
{
    const auto f = MakeMismatchTempPath( L".json" );
    MismatchTempFileGuard g( f );
    { Anafestica::JSON::TConfig c( f );
      c.GetRootNode().PutItem( L"val", double( 3.141592653589793 ) ); }
    Anafestica::JSON::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<float>( L"val" ) == float{} );
}

BOOST_AUTO_TEST_SUITE_END()


//---------------------------------------------------------------------------
// *** BSON type-mismatch tests ***
//---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE( TypeMismatch_BSON )

BOOST_AUTO_TEST_CASE( BSON_int_read_as_double )
{
    const auto f = MakeMismatchTempPath( L".bson" );
    MismatchTempFileGuard g( f );
    { Anafestica::BSON::TConfig c( f );
      c.GetRootNode().PutItem( L"val", int( -42 ) ); }
    Anafestica::BSON::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<double>( L"val" ) == double{} );
}

BOOST_AUTO_TEST_CASE( BSON_string_read_as_int )
{
    const auto f = MakeMismatchTempPath( L".bson" );
    MismatchTempFileGuard g( f );
    { Anafestica::BSON::TConfig c( f );
      c.GetRootNode().PutItem( L"val", String( L"Hello" ) ); }
    Anafestica::BSON::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<int>( L"val" ) == int{} );
}

BOOST_AUTO_TEST_CASE( BSON_int_read_as_string )
{
    const auto f = MakeMismatchTempPath( L".bson" );
    MismatchTempFileGuard g( f );
    { Anafestica::BSON::TConfig c( f );
      c.GetRootNode().PutItem( L"val", int( -42 ) ); }
    Anafestica::BSON::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<String>( L"val" ) == String{} );
}

BOOST_AUTO_TEST_CASE( BSON_bool_read_as_int )
{
    const auto f = MakeMismatchTempPath( L".bson" );
    MismatchTempFileGuard g( f );
    { Anafestica::BSON::TConfig c( f );
      c.GetRootNode().PutItem( L"val", true ); }
    Anafestica::BSON::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<int>( L"val" ) == int{} );
}

BOOST_AUTO_TEST_CASE( BSON_double_read_as_float )
{
    const auto f = MakeMismatchTempPath( L".bson" );
    MismatchTempFileGuard g( f );
    { Anafestica::BSON::TConfig c( f );
      c.GetRootNode().PutItem( L"val", double( 3.141592653589793 ) ); }
    Anafestica::BSON::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<float>( L"val" ) == float{} );
}

BOOST_AUTO_TEST_SUITE_END()


//---------------------------------------------------------------------------
// *** INIFile type-mismatch tests ***
//---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE( TypeMismatch_INIFile )

// 1. Store int, read as double
BOOST_AUTO_TEST_CASE( INIFile_int_read_as_double )
{
    const auto f = MakeMismatchTempPath( L".ini" );
    MismatchTempFileGuard g( f );
    { Anafestica::INIFile::TConfig c( f );
      c.GetRootNode().PutItem( L"val", int( -42 ) ); }
    Anafestica::INIFile::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<double>( L"val" ) == double{} );
}

// 2. Store String, read as int
BOOST_AUTO_TEST_CASE( INIFile_string_read_as_int )
{
    const auto f = MakeMismatchTempPath( L".ini" );
    MismatchTempFileGuard g( f );
    { Anafestica::INIFile::TConfig c( f );
      c.GetRootNode().PutItem( L"val", String( L"Hello" ) ); }
    Anafestica::INIFile::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<int>( L"val" ) == int{} );
}

// 3. Store int, read as String
BOOST_AUTO_TEST_CASE( INIFile_int_read_as_string )
{
    const auto f = MakeMismatchTempPath( L".ini" );
    MismatchTempFileGuard g( f );
    { Anafestica::INIFile::TConfig c( f );
      c.GetRootNode().PutItem( L"val", int( -42 ) ); }
    Anafestica::INIFile::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<String>( L"val" ) == String{} );
}

// 4. Store bool, read as int
BOOST_AUTO_TEST_CASE( INIFile_bool_read_as_int )
{
    const auto f = MakeMismatchTempPath( L".ini" );
    MismatchTempFileGuard g( f );
    { Anafestica::INIFile::TConfig c( f );
      c.GetRootNode().PutItem( L"val", true ); }
    Anafestica::INIFile::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<int>( L"val" ) == int{} );
}

// 5. Store double, read as float
BOOST_AUTO_TEST_CASE( INIFile_double_read_as_float )
{
    const auto f = MakeMismatchTempPath( L".ini" );
    MismatchTempFileGuard g( f );
    { Anafestica::INIFile::TConfig c( f );
      c.GetRootNode().PutItem( L"val", double( 3.141592653589793 ) ); }
    Anafestica::INIFile::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<float>( L"val" ) == float{} );
}

BOOST_AUTO_TEST_SUITE_END()


//---------------------------------------------------------------------------
// *** XML type-mismatch tests ***
//---------------------------------------------------------------------------

BOOST_FIXTURE_TEST_SUITE( TypeMismatch_XML, MismatchXMLCOMFixture )

// 1. Store int, read as double
BOOST_AUTO_TEST_CASE( XML_int_read_as_double )
{
    const auto f = MakeMismatchTempPath( L".xml" );
    MismatchTempFileGuard g( f );
    { Anafestica::XML::TConfig c( f );
      c.GetRootNode().PutItem( L"val", int( -42 ) ); }
    Anafestica::XML::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<double>( L"val" ) == double{} );
}

// 2. Store String, read as int
BOOST_AUTO_TEST_CASE( XML_string_read_as_int )
{
    const auto f = MakeMismatchTempPath( L".xml" );
    MismatchTempFileGuard g( f );
    { Anafestica::XML::TConfig c( f );
      c.GetRootNode().PutItem( L"val", String( L"Hello" ) ); }
    Anafestica::XML::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<int>( L"val" ) == int{} );
}

// 3. Store int, read as String
BOOST_AUTO_TEST_CASE( XML_int_read_as_string )
{
    const auto f = MakeMismatchTempPath( L".xml" );
    MismatchTempFileGuard g( f );
    { Anafestica::XML::TConfig c( f );
      c.GetRootNode().PutItem( L"val", int( -42 ) ); }
    Anafestica::XML::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<String>( L"val" ) == String{} );
}

// 4. Store bool, read as int
BOOST_AUTO_TEST_CASE( XML_bool_read_as_int )
{
    const auto f = MakeMismatchTempPath( L".xml" );
    MismatchTempFileGuard g( f );
    { Anafestica::XML::TConfig c( f );
      c.GetRootNode().PutItem( L"val", true ); }
    Anafestica::XML::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<int>( L"val" ) == int{} );
}

// 5. Store double, read as float
BOOST_AUTO_TEST_CASE( XML_double_read_as_float )
{
    const auto f = MakeMismatchTempPath( L".xml" );
    MismatchTempFileGuard g( f );
    { Anafestica::XML::TConfig c( f );
      c.GetRootNode().PutItem( L"val", double( 3.141592653589793 ) ); }
    Anafestica::XML::TConfig c( f );
    BOOST_TEST( c.GetRootNode().GetItem<float>( L"val" ) == float{} );
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace
