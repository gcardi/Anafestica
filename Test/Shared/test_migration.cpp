//---------------------------------------------------------------------------
// Tests for the configuration-migration helpers.
//
// Exercises the file-based migration constructor (load-A → save-B) using the
// JSON backend as a representative, plus the dirty-tracking behaviour of the
// base TConfig destructor (no flush when nothing changed) and the
// FindPriorVersionFileUnder discovery helper.
//
// The other file backends (BSON, XML, INI, YAML) share the exact same
// constructor / Migrate factory shape and the same ShouldFlushOnDestruction
// logic, so the JSON tests cover their migration semantics by extension.
//---------------------------------------------------------------------------

#pragma hdrstop

//---------------------------------------------------------------------------
#pragma package(smart_init)

#include <boost/test/unit_test.hpp>

#include <anafestica/CfgJSON.h>
#include <anafestica/Migration.h>
#include <anafestica/Version.h>

#include <System.SysUtils.hpp>
#include <System.IOUtils.hpp>
#include <System.Classes.hpp>

namespace {

//---------------------------------------------------------------------------
// Temp filesystem helpers
//---------------------------------------------------------------------------

unsigned TmpSeq = 0;

String MakeTempJsonPath( String Tag )
{
    return TPath::GetTempPath()
         + _D( "anafestica_mig_" ) + Tag + _D( "_" )
         + IntToStr( static_cast<int>( ++TmpSeq ) )
         + _D( ".json" );
}

String MakeTempDir( String Tag )
{
    String const Dir =
        TPath::GetTempPath()
      + _D( "anafestica_mig_dir_" ) + Tag + _D( "_" )
      + IntToStr( static_cast<int>( ++TmpSeq ) );
    TDirectory::CreateDirectory( Dir );
    return Dir;
}

struct TempFileGuard {
    String Path;
    explicit TempFileGuard( String P ) : Path( P ) {}
    ~TempFileGuard() noexcept {
        try { if ( TFile::Exists( Path ) ) TFile::Delete( Path ); }
        catch ( ... ) {}
    }
};

struct TempDirGuard {
    String Path;
    explicit TempDirGuard( String P ) : Path( P ) {}
    ~TempDirGuard() noexcept {
        try {
            if ( TDirectory::Exists( Path ) ) {
                TDirectory::Delete( Path, true );  // recursive
            }
        }
        catch ( ... ) {}
    }
};

} // namespace

BOOST_AUTO_TEST_SUITE( migration )

//---------------------------------------------------------------------------
// Dirty-tracking: no flush when nothing changed and no file existed
//---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( FreshInstallNoChangesDoesNotCreateFile )
{
    String const Path = MakeTempJsonPath( _D( "fresh_noop" ) );
    TempFileGuard Cleanup( Path );

    // Pre-state: ensure the file truly does not exist.
    if ( TFile::Exists( Path ) ) { TFile::Delete( Path ); }

    {
        Anafestica::JSON::TConfig Cfg( Path );
        (void)Cfg;
        // No PutItem.  Destructor must NOT flush because nothing was
        // loaded (file absent) and nothing was modified.
    }

    BOOST_TEST( !TFile::Exists( Path ) );
}

BOOST_AUTO_TEST_CASE( ExistingFileNoChangesDoesNotRewrite )
{
    String const Path = MakeTempJsonPath( _D( "existing_noop" ) );
    TempFileGuard Cleanup( Path );

    // Seed the file with arbitrary content that does NOT match the
    // backend's expected schema (so a rewrite would replace it).
    String const Sentinel =
        _D( "{\"this-is-not-anafestica-schema\":\"sentinel\"}" );
    TFile::WriteAllText( Path, Sentinel );

    {
        Anafestica::JSON::TConfig Cfg( Path );
        (void)Cfg;
        // No modifications: dtor must NOT rewrite, so the sentinel content
        // survives unchanged.
    }

    BOOST_TEST( TFile::Exists( Path ) );
    BOOST_TEST( TFile::ReadAllText( Path ) == Sentinel );
}

BOOST_AUTO_TEST_CASE( FreshInstallWithChangeWritesFile )
{
    String const Path = MakeTempJsonPath( _D( "fresh_modified" ) );
    TempFileGuard Cleanup( Path );

    if ( TFile::Exists( Path ) ) { TFile::Delete( Path ); }

    {
        Anafestica::JSON::TConfig Cfg( Path );
        Cfg.GetRootNode().PutItem( _D( "key" ), int( 42 ) );
    }

    BOOST_TEST( TFile::Exists( Path ) );

    // Round-trip the value.
    Anafestica::JSON::TConfig Reload( Path, /*ReadOnly*/ true );
    BOOST_TEST( Reload.GetRootNode().GetItem<int>( _D( "key" ) ) == 42 );
}

//---------------------------------------------------------------------------
// Migration ctor: source -> destination
//---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( MigrationCopiesSourceContentsToDestination )
{
    String const Src = MakeTempJsonPath( _D( "mig_src" ) );
    String const Dst = MakeTempJsonPath( _D( "mig_dst" ) );
    TempFileGuard CleanupSrc( Src );
    TempFileGuard CleanupDst( Dst );

    // Populate source via a regular single-file config.
    {
        Anafestica::JSON::TConfig SrcCfg( Src );
        SrcCfg.GetRootNode().PutItem( _D( "port" ),  int( 8080 ) );
        SrcCfg.GetRootNode().PutItem( _D( "host" ),  String( _D( "example" ) ) );
    }
    BOOST_REQUIRE( TFile::Exists( Src ) );
    BOOST_REQUIRE( !TFile::Exists( Dst ) );

    // Migrate.  No modifications between load and dtor — still must flush.
    {
        auto Cfg = Anafestica::JSON::TConfig::Migrate( Src, Dst );
        (void)Cfg;
    }

    BOOST_TEST( TFile::Exists( Dst ) );

    Anafestica::JSON::TConfig Reload( Dst, /*ReadOnly*/ true );
    BOOST_TEST( Reload.GetRootNode().GetItem<int>( _D( "port" ) ) == 8080 );
    BOOST_TEST(
        Reload.GetRootNode().GetItem<String>( _D( "host" ) ) ==
        String( _D( "example" ) )
    );

    // Source must remain untouched.
    BOOST_TEST( TFile::Exists( Src ) );
}

BOOST_AUTO_TEST_CASE( MigrationDestinationWinsWhenBothExist )
{
    String const Src = MakeTempJsonPath( _D( "destwin_src" ) );
    String const Dst = MakeTempJsonPath( _D( "destwin_dst" ) );
    TempFileGuard CleanupSrc( Src );
    TempFileGuard CleanupDst( Dst );

    // Source has an "old" value; destination has the "new" value already.
    {
        Anafestica::JSON::TConfig SrcCfg( Src );
        SrcCfg.GetRootNode().PutItem( _D( "v" ), int( 1 ) );
    }
    {
        Anafestica::JSON::TConfig DstCfg( Dst );
        DstCfg.GetRootNode().PutItem( _D( "v" ), int( 99 ) );
    }

    // Migrate with both present.  Destination must win.
    {
        auto Cfg = Anafestica::JSON::TConfig::Migrate( Src, Dst );
        BOOST_TEST( Cfg.GetRootNode().GetItem<int>( _D( "v" ) ) == 99 );
    }

    Anafestica::JSON::TConfig Reload( Dst, /*ReadOnly*/ true );
    BOOST_TEST( Reload.GetRootNode().GetItem<int>( _D( "v" ) ) == 99 );
}

BOOST_AUTO_TEST_CASE( MigrationSourceMissingBehavesLikeFreshInstall )
{
    String const Src = MakeTempJsonPath( _D( "miss_src" ) );
    String const Dst = MakeTempJsonPath( _D( "miss_dst" ) );
    TempFileGuard CleanupSrc( Src );
    TempFileGuard CleanupDst( Dst );

    if ( TFile::Exists( Src ) ) { TFile::Delete( Src ); }
    if ( TFile::Exists( Dst ) ) { TFile::Delete( Dst ); }

    // No source, no destination, no modifications: nothing should be written.
    {
        auto Cfg = Anafestica::JSON::TConfig::Migrate( Src, Dst );
        (void)Cfg;
    }

    BOOST_TEST( !TFile::Exists( Src ) );
    BOOST_TEST( !TFile::Exists( Dst ) );
}

BOOST_AUTO_TEST_CASE( MigrationPreservesModificationsAfterLoad )
{
    String const Src = MakeTempJsonPath( _D( "mod_src" ) );
    String const Dst = MakeTempJsonPath( _D( "mod_dst" ) );
    TempFileGuard CleanupSrc( Src );
    TempFileGuard CleanupDst( Dst );

    // Seed source.
    {
        Anafestica::JSON::TConfig SrcCfg( Src );
        SrcCfg.GetRootNode().PutItem( _D( "from_src" ), int( 1 ) );
    }

    // Migrate + modify in one session.
    {
        auto Cfg = Anafestica::JSON::TConfig::Migrate( Src, Dst );
        Cfg.GetRootNode().PutItem( _D( "added_during_migration" ), int( 2 ) );
    }

    Anafestica::JSON::TConfig Reload( Dst, /*ReadOnly*/ true );
    BOOST_TEST(
        Reload.GetRootNode().GetItem<int>( _D( "from_src" ) ) == 1
    );
    BOOST_TEST(
        Reload.GetRootNode().GetItem<int>( _D( "added_during_migration" ) ) == 2
    );
}

//---------------------------------------------------------------------------
// FindPriorVersionFileUnder discovery
//---------------------------------------------------------------------------

namespace {

void SeedVersionDir( String ProductRoot, String VerLeaf, String FileLeaf )
{
    String const VerDir = TPath::Combine( ProductRoot, VerLeaf );
    TDirectory::CreateDirectory( VerDir );
    String const FilePath = TPath::Combine( VerDir, FileLeaf );
    TFile::WriteAllText( FilePath, _D( "{}" ) );
}

} // namespace

BOOST_AUTO_TEST_CASE( FindPriorPicksHighestStrictlyOlder )
{
    String const Root = MakeTempDir( _D( "find_prior" ) );
    TempDirGuard Cleanup( Root );

    SeedVersionDir( Root, _D( "1.0" ),   _D( "app.json" ) );
    SeedVersionDir( Root, _D( "1.2.3" ), _D( "app.json" ) );
    SeedVersionDir( Root, _D( "1.5" ),   _D( "app.json" ) );  // older than current
    SeedVersionDir( Root, _D( "2.0" ),   _D( "app.json" ) );  // current — must NOT match
    SeedVersionDir( Root, _D( "2.1" ),   _D( "app.json" ) );  // newer — must NOT match

    auto const Result =
        Anafestica::Migration::FindPriorVersionFileUnder(
            Root,
            Anafestica::TVersion( _D( "2.0" ) ),
            _D( "app.exe" ),
            _D( ".json" )
        );

    BOOST_REQUIRE( Result.has_value() );
    BOOST_TEST(
        TPath::GetFileName( TPath::GetDirectoryName( *Result ) ) ==
        String( _D( "1.5" ) )
    );
}

BOOST_AUTO_TEST_CASE( FindPriorIgnoresNonVersionDirectories )
{
    String const Root = MakeTempDir( _D( "find_prior_junk" ) );
    TempDirGuard Cleanup( Root );

    SeedVersionDir( Root, _D( "backup" ),     _D( "app.json" ) );
    SeedVersionDir( Root, _D( "tmp" ),        _D( "app.json" ) );
    SeedVersionDir( Root, _D( "1.0RC1" ),     _D( "app.json" ) ); // malformed
    SeedVersionDir( Root, _D( "1.1" ),        _D( "app.json" ) ); // valid older
    SeedVersionDir( Root, _D( "2.0.0a" ),     _D( "app.json" ) ); // malformed

    auto const Result =
        Anafestica::Migration::FindPriorVersionFileUnder(
            Root,
            Anafestica::TVersion( _D( "2.0" ) ),
            _D( "app.exe" ),
            _D( ".json" )
        );

    BOOST_REQUIRE( Result.has_value() );
    BOOST_TEST(
        TPath::GetFileName( TPath::GetDirectoryName( *Result ) ) ==
        String( _D( "1.1" ) )
    );
}

BOOST_AUTO_TEST_CASE( FindPriorReturnsNulloptWhenNoOlderVersion )
{
    String const Root = MakeTempDir( _D( "find_prior_none" ) );
    TempDirGuard Cleanup( Root );

    SeedVersionDir( Root, _D( "2.0" ), _D( "app.json" ) );
    SeedVersionDir( Root, _D( "3.1" ), _D( "app.json" ) );

    auto const Result =
        Anafestica::Migration::FindPriorVersionFileUnder(
            Root,
            Anafestica::TVersion( _D( "1.0" ) ),
            _D( "app.exe" ),
            _D( ".json" )
        );

    BOOST_TEST( !Result.has_value() );
}

BOOST_AUTO_TEST_CASE( FindPriorReturnsNulloptWhenRootMissing )
{
    String const Root =
        TPath::Combine(
            TPath::GetTempPath(),
            _D( "anafestica_mig_definitely_does_not_exist_" )
          + IntToStr( static_cast<int>( ++TmpSeq ) )
        );
    BOOST_REQUIRE( !TDirectory::Exists( Root ) );

    auto const Result =
        Anafestica::Migration::FindPriorVersionFileUnder(
            Root,
            Anafestica::TVersion( _D( "2.0" ) ),
            _D( "app.exe" ),
            _D( ".json" )
        );

    BOOST_TEST( !Result.has_value() );
}

BOOST_AUTO_TEST_CASE( FindPriorReturnsNulloptWhenExtensionMismatch )
{
    String const Root = MakeTempDir( _D( "find_prior_ext" ) );
    TempDirGuard Cleanup( Root );

    // Older version exists but only as a different backend's file.
    SeedVersionDir( Root, _D( "1.0" ), _D( "app.xml" ) );

    auto const Result =
        Anafestica::Migration::FindPriorVersionFileUnder(
            Root,
            Anafestica::TVersion( _D( "2.0" ) ),
            _D( "app.exe" ),
            _D( ".json" )
        );

    BOOST_TEST( !Result.has_value() );
}

BOOST_AUTO_TEST_SUITE_END()
