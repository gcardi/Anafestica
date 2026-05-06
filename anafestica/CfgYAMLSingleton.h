//---------------------------------------------------------------------------

#ifndef CfgYAMLSingletonH
#define CfgYAMLSingletonH

#include <anafestica/CfgSingletonVersionInfo.h>
#include <anafestica/CfgYAML.h>


//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------
namespace YAML {
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
            _D( ".yaml" )
        );
}
//---------------------------------------------------------------------------

inline Anafestica::TConfig& GetConfigSingleton( String FileName = ParamStr( {} ) )
{
    static auto Cfg = TConfig( GetFileName( FileName ), false );
    return Cfg;
}
//---------------------------------------------------------------------------

class TConfigSingleton {
public:
    static Anafestica::TConfig& GetConfig() {
        return Anafestica::YAML::GetConfigSingleton();
    }
private:
};

//---------------------------------------------------------------------------
} // End of namespace YAML
//---------------------------------------------------------------------------

using TConfigYAMLSingleton = YAML::TConfigSingleton;

//---------------------------------------------------------------------------
} // End of namespace Anafestica
//---------------------------------------------------------------------------

#endif
