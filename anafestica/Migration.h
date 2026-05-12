//---------------------------------------------------------------------------
//
// Helpers for migrating configuration data from a previous version of the
// application to the current one.
//
// The file-based singleton helpers compute the per-version path
//   $HOME\<CompanyName>\<ProductName>\<ProductVersion>\<exe>.<ext>
// from the executable's version info.  When that path does not exist for the
// current version, FindPriorVersionFile() looks for the most recent strictly
// older sibling directory under
//   $HOME\<CompanyName>\<ProductName>\
// whose name parses as a TVersion (see anafestica/Version.h) and returns
// the corresponding configuration file path — to be passed as the source
// argument to the migration constructor (or Migrate factory) of the chosen
// file backend.
//
// The discovery step is best-effort: directories that don't parse as
// versions (e.g. "backup", "tmp") are silently skipped.  When no strictly
// older version is found, std::nullopt is returned and the caller should
// fall back to the regular single-argument constructor (which will start
// from defaults).
//
//---------------------------------------------------------------------------

#ifndef MigrationH
#define MigrationH

#include <System.hpp>
#include <System.SysUtils.hpp>
#include <System.IOUtils.hpp>

#include <optional>

#include <anafestica/CfgSingletonVersionInfo.h>
#include <anafestica/Version.h>

//---------------------------------------------------------------------------
namespace Anafestica {
namespace Migration {
//---------------------------------------------------------------------------

/// Discovers the previous-version configuration file inside an explicit
/// product-root directory.  Useful for unit tests that need a hermetic
/// directory tree; production code typically calls
/// @ref FindPriorVersionFile instead, which derives @p ProductRoot and
/// @p Current from the executable's version info.
///
/// @param ProductRoot     The directory containing per-version subfolders
///                        (e.g. @c $HOME\Co\Prod\).
/// @param Current         The current version; only strictly older
///                        siblings are considered.
/// @param LeafFileName    Basename to look for inside the chosen sibling
///                        (e.g. @c "myapp.exe" — the extension is replaced
///                        below).
/// @param Extension       File extension of the backend (e.g. @c ".json",
///                        @c ".bson", @c ".xml", @c ".ini", @c ".yaml").
///                        Must include the leading dot.
inline std::optional<String>
FindPriorVersionFileUnder( String ProductRoot, TVersion Current,
                           String LeafFileName, String Extension )
{
    if ( !TDirectory::Exists( ProductRoot ) ) {
        return std::nullopt;
    }

    auto const SubDirs = TDirectory::GetDirectories( ProductRoot );
    String BestDir;
    std::optional<TVersion> Best;

    for ( int Idx = 0; Idx < SubDirs.Length; ++Idx ) {
        String const Dir = SubDirs[Idx];
        String const Leaf = TPath::GetFileName( Dir );
        try {
            TVersion const V( Leaf );
            if ( !( V < Current ) ) {
                // Equal or newer than current — never a migration source.
                continue;
            }
            if ( !Best || *Best < V ) {
                Best = V;
                BestDir = Dir;
            }
        }
        catch ( Exception const & ) {
            // Non-version-shaped sibling (e.g. "backup", "tmp") — skip.
            continue;
        }
    }

    if ( !Best ) {
        return std::nullopt;
    }

    String const Candidate =
        TPath::ChangeExtension(
            TPath::Combine( BestDir, LeafFileName ),
            Extension
        );
    if ( !TFile::Exists( Candidate ) ) {
        // Sibling directory exists but the per-backend file isn't there
        // (a different backend was used at that prior version).  Treat
        // it as "no prior version for this backend."
        return std::nullopt;
    }
    return Candidate;
}

/// Returns the path of the configuration file belonging to the most recent
/// strictly older version of the application, or @c std::nullopt when none
/// exists.
///
/// @param FileName   Path to the executable whose version info drives the
///                   discovery (typically @c ParamStr(0)).
/// @param Extension  File extension of the backend (e.g. @c ".json",
///                   @c ".bson", @c ".xml", @c ".ini", @c ".yaml").
///                   Must include the leading dot.
///
/// Reads the executable's CompanyName / ProductName / ProductVersion via
/// @ref GetSingletonFileVersionInfo, then delegates to
/// @ref FindPriorVersionFileUnder with the resolved product root.
inline std::optional<String>
FindPriorVersionFile( String FileName, String Extension )
{
    auto const Info = GetSingletonFileVersionInfo( FileName );
    TVersion const Current( Info.ProductVersion );
    String const ProductRoot =
        TPath::Combine(
            TPath::Combine( TPath::GetHomePath(), Info.CompanyName ),
            Info.ProductName
        );
    return FindPriorVersionFileUnder(
        ProductRoot, Current, ExtractFileName( FileName ), Extension
    );
}

//---------------------------------------------------------------------------
} // End of namespace Migration
} // End of namespace Anafestica
//---------------------------------------------------------------------------

#endif
