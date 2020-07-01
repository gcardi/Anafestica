//---------------------------------------------------------------------------

#ifndef CfgXMLSingletonH
#define CfgXMLSingletonH

#include <System.hpp>
#include <System.IOUtils.hpp>

#include <memory>

#include <anafestica/FileVersionInfo.h>
#include <anafestica/CfgXML.h>


//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------
namespace XML {
//---------------------------------------------------------------------------

class TConfigSingleton {
public:
    Anafestica::TConfig& GetConfig() {
        static auto Cfg = std::make_unique<TConfig>( GetFileName(), false );
        return *Cfg;
    }
private:
    static String GetFileName()
    {
        TFileVersionInfo const Info( ParamStr( {} ) );
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
                    ExtractFileName( ParamStr( {} ) )
                ),
                _T( ".xml" )
            );
    }
};

//---------------------------------------------------------------------------
} // End of namespace XML
//---------------------------------------------------------------------------

using TConfigXMLSingleton = XML::TConfigSingleton;

//---------------------------------------------------------------------------
} // End of namespace Anafestica
//---------------------------------------------------------------------------

#endif