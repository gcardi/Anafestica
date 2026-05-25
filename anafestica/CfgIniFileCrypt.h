//---------------------------------------------------------------------------

#ifndef CfgIniFileCryptH
#define CfgIniFileCryptH

#include <anafestica/CfgIniFile.h>

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------
namespace INIFileCrypt {
//---------------------------------------------------------------------------

class TConfig : public INIFile::TConfig {
public:
    TConfig( String FileName, bool ReadOnly = false,
             bool FlushAllItems = false,
             Crypt::TOptions Options = Crypt::TOptions::Default() )
        : INIFile::TConfig( FileName, ReadOnly, FlushAllItems, Options )
    {}

    TConfig( String LoadFileName, String SaveFileName,
             bool ReadOnly = false,
             Crypt::TOptions Options = Crypt::TOptions::Default() )
        : INIFile::TConfig( LoadFileName, SaveFileName, ReadOnly, Options )
    {}

    static TConfig Migrate( String LoadFileName, String SaveFileName,
                            bool ReadOnly = false,
                            Crypt::TOptions Options = Crypt::TOptions::Default() )
    {
        return TConfig( LoadFileName, SaveFileName, ReadOnly, Options );
    }
};

//---------------------------------------------------------------------------
} // End namespace INIFileCrypt
//---------------------------------------------------------------------------
} // End namespace Anafestica
//---------------------------------------------------------------------------

#endif
