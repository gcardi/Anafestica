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