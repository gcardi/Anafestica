//---------------------------------------------------------------------------

#ifndef CfgIniFileSingletonH
#define CfgIniFileSingletonH

#include <anafestica/CfgSingletonVersionInfo.h>
#include <anafestica/CfgIniFile.h>
#include <anafestica/Migration.h>

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------
namespace INIFile {
//---------------------------------------------------------------------------

// Build the default INI file path from the executable's version info:
//   $(HOME)\$(CompanyName)\$(ProductName)\$(ProductVersion)\AppName.ini
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
            _D( ".ini" )
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
                        FileName, _D( ".ini" )
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
        return Anafestica::INIFile::GetConfigSingleton();
    }
private:
};

//---------------------------------------------------------------------------
} // End namespace INIFile
//---------------------------------------------------------------------------

using TConfigINIFileSingleton = INIFile::TConfigSingleton;

//---------------------------------------------------------------------------
} // End namespace Anafestica
//---------------------------------------------------------------------------

#endif
