//---------------------------------------------------------------------------

#ifndef CfgSingletonVersionInfoH
#define CfgSingletonVersionInfoH

#include <anafestica/FileVersionInfo.h>

#include <Winapi.Windows.hpp>

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------

inline constexpr LPCTSTR MissingVersionInfoMessage =
    _D(
        "It appears that this executable is missing version information. "
        "This information is essential for persistence management. "
        "Please add it."
    );

inline bool IsMissingVersionInfoError( DWORD Error )
{
    return Error == ERROR_SUCCESS ||
           Error == ERROR_RESOURCE_TYPE_NOT_FOUND ||
           Error == ERROR_RESOURCE_NAME_NOT_FOUND ||
           Error == ERROR_RESOURCE_LANG_NOT_FOUND ||
           Error == ERROR_RESOURCE_DATA_NOT_FOUND;
}
//---------------------------------------------------------------------------

inline bool IsMissingVersionInfoException( Exception const& Error )
{
    auto const OSError = dynamic_cast<System::Sysutils::EOSError const*>( &Error );
    return OSError != nullptr &&
           IsMissingVersionInfoError( static_cast<DWORD>( OSError->ErrorCode ) );
}
//---------------------------------------------------------------------------

inline void EnsureVersionInfoAvailable( String FileName )
{
    DWORD Handle = 0;
    ::SetLastError( ERROR_SUCCESS );
    auto const Size = ::GetFileVersionInfoSize( FileName.c_str(), &Handle );
    if ( !Size ) {
        auto const Error = ::GetLastError();
        if ( IsMissingVersionInfoError( Error ) ) {
            throw Exception( MissingVersionInfoMessage );
        }
    }
}
//---------------------------------------------------------------------------

inline TFileVersionInfo GetSingletonFileVersionInfo( String FileName )
{
    EnsureVersionInfoAvailable( FileName );

    try {
        auto Info = TFileVersionInfo( FileName );

        // Pre-validate the fields required by singleton path conventions.
        auto const CompanyName = Info.CompanyName;
        auto const ProductName = Info.ProductName;
        auto const ProductVersion = Info.ProductVersion;
        (void)CompanyName;
        (void)ProductName;
        (void)ProductVersion;

        return Info;
    }
    catch ( Exception const& Error ) {
        if ( IsMissingVersionInfoException( Error ) ||
             Error.Message == _D( "Translation table not found (Version Info)" ) ) {
            throw Exception( MissingVersionInfoMessage );
        }
        throw;
    }
}

//---------------------------------------------------------------------------
} // End of namespace Anafestica
//---------------------------------------------------------------------------

#endif