//---------------------------------------------------------------------------

#ifndef CfgYAMLCryptH
#define CfgYAMLCryptH

#include <anafestica/CfgYAML.h>

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------
namespace YAMLCrypt {
//---------------------------------------------------------------------------

class TConfig : public YAML::TConfig {
public:
    TConfig( String FileName, bool ReadOnly = false,
             bool FlushAllItems = false, bool ExplicitTypes = false,
             Crypt::TOptions Options = Crypt::TOptions::Default() )
        : YAML::TConfig(
            FileName, ReadOnly, FlushAllItems, ExplicitTypes, Options
          )
    {}

    TConfig( String LoadFileName, String SaveFileName,
             bool ReadOnly = false, bool ExplicitTypes = false,
             Crypt::TOptions Options = Crypt::TOptions::Default() )
        : YAML::TConfig(
            LoadFileName, SaveFileName, ReadOnly, ExplicitTypes, Options
          )
    {}

    static TConfig Migrate( String LoadFileName, String SaveFileName,
                            bool ReadOnly = false, bool ExplicitTypes = false,
                            Crypt::TOptions Options = Crypt::TOptions::Default() )
    {
        return TConfig(
            LoadFileName, SaveFileName, ReadOnly, ExplicitTypes, Options
        );
    }
};

//---------------------------------------------------------------------------
} // End namespace YAMLCrypt
//---------------------------------------------------------------------------
} // End namespace Anafestica
//---------------------------------------------------------------------------

#endif
