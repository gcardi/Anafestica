//---------------------------------------------------------------------------

#ifndef CfgIniFileSingletonH
#define CfgIniFileSingletonH

#include <anafestica/FileVersionInfo.h>
#include <anafestica/CfgIniFile.h>

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------
namespace INIFile {
//---------------------------------------------------------------------------

// Build the default INI file path from the executable's version info:
//   $(HOME)\$(CompanyName)\$(ProductName)\$(ProductVersion)\AppName.ini
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
            _D( ".ini" )
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
