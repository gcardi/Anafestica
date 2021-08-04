//---------------------------------------------------------------------------

#ifndef CfgXMLSingletonH
#define CfgXMLSingletonH

#include <anafestica/FileVersionInfo.h>
#include <anafestica/CfgXML.h>

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------
namespace XML {
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
            _D( ".xml" )
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