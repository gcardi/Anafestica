//---------------------------------------------------------------------------

#ifndef CfgXMLCryptSingletonH
#define CfgXMLCryptSingletonH

#include <anafestica/CfgSingletonVersionInfo.h>
#include <anafestica/CfgXMLCrypt.h>
#include <anafestica/Migration.h>

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------
namespace XMLCrypt {
//---------------------------------------------------------------------------

static constexpr LPCTSTR ConfigFileExtension = _D( ".xmlc" );

inline String GetFileName( String FileName )
{
    auto const Info = GetSingletonFileVersionInfo( FileName );
    return
        TPath::ChangeExtension(
            TPath::Combine(
                TPath::Combine(
                    TPath::Combine(
                        TPath::Combine(
                            TPath::GetHomePath(),
                            Info.CompanyName
                        ),
                        Info.ProductName
                    ),
                    Info.ProductVersion
                ),
                ExtractFileName( FileName )
            ),
            ConfigFileExtension
        );
}
//---------------------------------------------------------------------------

inline Anafestica::TConfig&
GetConfigSingleton( String FileName = ParamStr( {} ),
                    Crypt::TOptions Options = Crypt::TOptions::Default() )
{
    static auto Cfg = [FileName, Options]() -> TConfig {
        auto const Dest = GetFileName( FileName );
        if ( !TFile::Exists( Dest ) ) {
            if ( auto const Source =
                    Anafestica::Migration::FindPriorVersionFile(
                        FileName, ConfigFileExtension
                    ) )
            {
                return TConfig::Migrate( *Source, Dest, false, Options );
            }
        }
        return TConfig( Dest, false, false, Options );
    }();
    return Cfg;
}
//---------------------------------------------------------------------------

class TConfigSingleton {
public:
    static Anafestica::TConfig& GetConfig() {
        return Anafestica::XMLCrypt::GetConfigSingleton();
    }
};

//---------------------------------------------------------------------------
} // End namespace XMLCrypt
//---------------------------------------------------------------------------

using TConfigXMLCryptSingleton = XMLCrypt::TConfigSingleton;

//---------------------------------------------------------------------------
} // End namespace Anafestica
//---------------------------------------------------------------------------

#endif
