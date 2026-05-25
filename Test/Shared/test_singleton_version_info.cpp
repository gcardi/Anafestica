//---------------------------------------------------------------------------
// Dedicated tests for singleton version-info guard/translation.
//---------------------------------------------------------------------------

#pragma hdrstop

//---------------------------------------------------------------------------
#pragma package(smart_init)

#include <boost/test/unit_test.hpp>

#include <anafestica/CfgSingletonVersionInfo.h>
#include <anafestica/CfgJSONCryptSingleton.h>
#include <anafestica/CfgBSONCryptSingleton.h>
#include <anafestica/CfgXMLCryptSingleton.h>
#include <anafestica/CfgIniFileCryptSingleton.h>

#if defined( ANAFESTICA_TEST_YAML ) && __has_include(<fkYAML/node.hpp>)
# define ANAFESTICA_TEST_YAML_CRYPT_SINGLETON_AVAILABLE
# include <anafestica/CfgYAMLCryptSingleton.h>
#endif

#include <System.SysUtils.hpp>
#include <System.IOUtils.hpp>
#include <System.Classes.hpp>

namespace {

struct TempFileGuard
{
    String Path;

    explicit TempFileGuard( String const& APath )
      : Path( APath )
    {}

    ~TempFileGuard() noexcept
    {
        try {
            if ( TFile::Exists( Path ) ) {
                TFile::Delete( Path );
            }
        }
        catch ( ... ) {
        }
    }
};

} // namespace

BOOST_AUTO_TEST_SUITE( singleton_version_info )

BOOST_AUTO_TEST_CASE( MissingVersionInfo_ThrowsFriendlyMessage )
{
    const String tempFile =
        TPath::GetTempPath() + _D( "anafestica_missing_verinfo_test.bin" );
    TempFileGuard cleanup( tempFile );

    TBytes payload;
    payload.Length = 4;
    payload[0] = 0x01;
    payload[1] = 0x23;
    payload[2] = 0x45;
    payload[3] = 0x67;
    TFile::WriteAllBytes( tempFile, payload );

    try {
        (void)Anafestica::GetSingletonFileVersionInfo( tempFile );
        BOOST_FAIL( "Expected missing-version-info exception" );
    }
    catch ( Exception const& Error ) {
        BOOST_TEST( Error.Message == String( Anafestica::MissingVersionInfoMessage ) );
    }
}

BOOST_AUTO_TEST_CASE( MissingFile_DoesNotThrowFriendlyMessage )
{
    const String missingFile =
        TPath::GetTempPath() + _D( "anafestica_missing_file_verinfo_test.bin" );

    if ( TFile::Exists( missingFile ) ) {
        TFile::Delete( missingFile );
    }

    try {
        (void)Anafestica::GetSingletonFileVersionInfo( missingFile );
        BOOST_FAIL( "Expected exception for missing file" );
    }
    catch ( System::Sysutils::EOSError const& Error ) {
        BOOST_TEST( Error.ErrorCode == static_cast<int>( ERROR_FILE_NOT_FOUND ) );
        BOOST_TEST( Error.Message != String( Anafestica::MissingVersionInfoMessage ) );
    }
    catch ( ... ) {
        BOOST_FAIL( "Expected EOSError for missing file" );
    }
}

BOOST_AUTO_TEST_CASE( CryptSingletonHeadersExposeExpectedExtensions )
{
    BOOST_TEST( String( Anafestica::JSONCrypt::ConfigFileExtension ) == String( _D( ".jsonc" ) ) );
    BOOST_TEST( String( Anafestica::BSONCrypt::ConfigFileExtension ) == String( _D( ".bsonc" ) ) );
    BOOST_TEST( String( Anafestica::XMLCrypt::ConfigFileExtension ) == String( _D( ".xmlc" ) ) );
    BOOST_TEST( String( Anafestica::INIFileCrypt::ConfigFileExtension ) == String( _D( ".inic" ) ) );

    (void)sizeof( Anafestica::TConfigJSONCryptSingleton );
    (void)sizeof( Anafestica::TConfigBSONCryptSingleton );
    (void)sizeof( Anafestica::TConfigXMLCryptSingleton );
    (void)sizeof( Anafestica::TConfigINIFileCryptSingleton );
#if defined( ANAFESTICA_TEST_YAML_CRYPT_SINGLETON_AVAILABLE )
    BOOST_TEST( String( Anafestica::YAMLCrypt::ConfigFileExtension ) == String( _D( ".yamlc" ) ) );
    (void)sizeof( Anafestica::TConfigYAMLCryptSingleton );
#endif
}

BOOST_AUTO_TEST_SUITE_END()
