//---------------------------------------------------------------------------

#ifndef CfgBSONSingletonH
#define CfgBSONSingletonH

#include <anafestica/CfgSingletonVersionInfo.h>
#include <anafestica/CfgBSON.h>

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------
namespace BSON {
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
            _D( ".bson" )
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
        return Anafestica::BSON::GetConfigSingleton();
    }
private:
};

//---------------------------------------------------------------------------
} // End namespace BSON
//---------------------------------------------------------------------------

using TConfigBSONSingleton = BSON::TConfigSingleton;

//---------------------------------------------------------------------------
} // End namespace Anafestica
//---------------------------------------------------------------------------

#endif
