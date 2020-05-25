//---------------------------------------------------------------------------

#ifndef CfgRegistryHModSingletonH
#define CfgRegistryHModSingletonH

#include <System.hpp>
#include <System.SysUtils.hpp>

#include <memory>

#include <anafestica/FileVersionInfo.h>
#include <anafestica/CfgRegistry.h>

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------
namespace Registry {
//---------------------------------------------------------------------------

class TConfigHModSingleton {
public:
	Anafestica::TConfig& GetConfig() {
        static auto Cfg =
            std::make_unique<TConfig>(
                HKEY_CURRENT_USER,
                Format(
                    _T( "Software\\%s" ),
                    ARRAYOFCONST(( GetProductPath() ))
                )
            );

        return *Cfg;
    }
private:
	static String GetProductPath() {
        TFileVersionInfo const Info(
            GetModuleName( reinterpret_cast<unsigned>( HInstance ) )
        );

        return Format(
            _T( "%s\\%s\\%s" ),
            ARRAYOFCONST( (
                Info.CompanyName,
                Info.ProductName,
                Info.ProductVersion
            ) )
        );
    }
};

//---------------------------------------------------------------------------
} // End of namespace Registry
//---------------------------------------------------------------------------

using TConfigRegistryHModSingleton = Registry::TConfigHModSingleton;

//---------------------------------------------------------------------------
} // End of namespace Anafestica
//---------------------------------------------------------------------------

#endif
