//---------------------------------------------------------------------------

#ifndef CfgJSONSingletonH
#define CfgJSONSingletonH

#include <anafestica/CfgSingletonVersionInfo.h>
#include <anafestica/CfgJSON.h>
#include <anafestica/Migration.h>


//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------
namespace JSON {
//---------------------------------------------------------------------------

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
            _D( ".json" )
        );
}
//---------------------------------------------------------------------------

inline Anafestica::TConfig& GetConfigSingleton( String FileName = ParamStr( {} ) )
{
    static auto Cfg = [FileName]() -> TConfig {
        auto const Dest = GetFileName( FileName );
        if ( !TFile::Exists( Dest ) ) {
            if ( auto const Source =
                    Anafestica::Migration::FindPriorVersionFile(
                        FileName, _D( ".json" )
                    ) )
            {
                return TConfig::Migrate( *Source, Dest );
            }
        }
        return TConfig( Dest );
    }();
    return Cfg;
}
//---------------------------------------------------------------------------

class TConfigSingleton {
public:
    static Anafestica::TConfig& GetConfig() {
        return Anafestica::JSON::GetConfigSingleton();
    }
private:
};

//---------------------------------------------------------------------------
} // End of namespace JSON
//---------------------------------------------------------------------------

using TConfigJSONSingleton = JSON::TConfigSingleton;

//---------------------------------------------------------------------------
} // End of namespace Anafestica
//---------------------------------------------------------------------------

#endif
