//---------------------------------------------------------------------------

#ifndef CfgJSONSingletonH
#define CfgJSONSingletonH

#include <anafestica/FileVersionInfo.h>
#include <anafestica/CfgJSON.h>


//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------
namespace JSON {
//---------------------------------------------------------------------------

inline String GetFileName( String FileName )
{
    TFileVersionInfo const Info( FileName );
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
    static auto Cfg = TConfig( GetFileName( FileName ), false );
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
