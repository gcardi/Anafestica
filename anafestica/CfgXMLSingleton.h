//---------------------------------------------------------------------------

#ifndef CfgXMLSingletonH
#define CfgXMLSingletonH

#include <anafestica/CfgSingletonVersionInfo.h>
#include <anafestica/CfgXML.h>
#include <anafestica/Migration.h>

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------
namespace XML {
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
            _D( ".xml" )
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
                        FileName, _D( ".xml" )
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
        return Anafestica::XML::GetConfigSingleton();
    }
private:
};

//---------------------------------------------------------------------------
} // End of namespace XML
//---------------------------------------------------------------------------

using TConfigXMLSingleton = XML::TConfigSingleton;

//---------------------------------------------------------------------------
} // End of namespace Anafestica
//---------------------------------------------------------------------------

#endif