//---------------------------------------------------------------------------

#ifndef CfgRegistrySingletonH
#define CfgRegistrySingletonH

#include <anafestica/FileVersionInfo.h>
#include <anafestica/CfgRegistry.h>

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------
namespace Registry {
//---------------------------------------------------------------------------

inline String GetProductPath( String FileName )
{
    TFileVersionInfo const Info( FileName );

    return Format(
        _D( "%s\\%s\\%s" ),
        ARRAYOFCONST( (
            Info.CompanyName,
            Info.ProductName,
            Info.ProductVersion
        ) )
    );
}
//---------------------------------------------------------------------------

inline Anafestica::TConfig& GetConfigSingleton( String FileName = ParamStr( {} ) )
{
    static auto Cfg = TConfig(
        HKEY_CURRENT_USER,
        Format(
            _D( "Software\\%s" ),
            ARRAYOFCONST(( GetProductPath( FileName ) ))
        )
    );
    return Cfg;
}
//---------------------------------------------------------------------------

class TConfigSingleton {
public:
    static Anafestica::TConfig& GetConfig() {
        return Anafestica::Registry::GetConfigSingleton();
    }
private:
};

//---------------------------------------------------------------------------
} // End of namespace Registry
//---------------------------------------------------------------------------

using TConfigRegistrySingleton = Registry::TConfigSingleton;

//---------------------------------------------------------------------------
} // End of namespace Anafestica
//---------------------------------------------------------------------------

#endif
