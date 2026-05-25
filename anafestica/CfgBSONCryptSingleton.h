//---------------------------------------------------------------------------

#ifndef CfgBSONCryptSingletonH
#define CfgBSONCryptSingletonH

#include <anafestica/CfgSingletonVersionInfo.h>
#include <anafestica/CfgBSONCrypt.h>
#include <anafestica/Migration.h>

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------
namespace BSONCrypt {
//---------------------------------------------------------------------------

static constexpr LPCTSTR ConfigFileExtension = _D( ".bsonc" );

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
                return TConfig::Migrate( *Source, Dest, false, false, Options );
            }
        }
        return TConfig( Dest, false, false, false, Options );
    }();
    return Cfg;
}
//---------------------------------------------------------------------------

class TConfigSingleton {
public:
    static Anafestica::TConfig& GetConfig() {
        return Anafestica::BSONCrypt::GetConfigSingleton();
    }
};

//---------------------------------------------------------------------------
} // End namespace BSONCrypt
//---------------------------------------------------------------------------

using TConfigBSONCryptSingleton = BSONCrypt::TConfigSingleton;

//---------------------------------------------------------------------------
} // End namespace Anafestica
//---------------------------------------------------------------------------

#endif
