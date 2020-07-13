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
