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
