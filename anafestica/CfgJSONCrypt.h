//---------------------------------------------------------------------------

#ifndef CfgJSONCryptH
#define CfgJSONCryptH

#include <anafestica/CfgJSON.h>

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------
namespace JSONCrypt {
//---------------------------------------------------------------------------

class TConfig : public JSON::TConfig {
public:
    TConfig( String FileName, bool ReadOnly = false, bool Compact = true,
             bool FlushAllItems = false, bool ExplicitTypes = false,
             Crypt::TOptions Options = Crypt::TOptions::Default() )
        : JSON::TConfig(
            FileName, ReadOnly, Compact, FlushAllItems, ExplicitTypes, Options
          )
    {}

    TConfig( String LoadFileName, String SaveFileName,
             bool ReadOnly = false, bool Compact = true,
             bool ExplicitTypes = false,
             Crypt::TOptions Options = Crypt::TOptions::Default() )
        : JSON::TConfig(
            LoadFileName, SaveFileName, ReadOnly, Compact, ExplicitTypes,
            Options
          )
    {}

    static TConfig Migrate( String LoadFileName, String SaveFileName,
                            bool ReadOnly = false, bool Compact = true,
                            bool ExplicitTypes = false,
                            Crypt::TOptions Options = Crypt::TOptions::Default() )
    {
        return TConfig(
            LoadFileName, SaveFileName, ReadOnly, Compact, ExplicitTypes,
            Options
        );
    }
};

//---------------------------------------------------------------------------
} // End namespace JSONCrypt
//---------------------------------------------------------------------------
} // End namespace Anafestica
//---------------------------------------------------------------------------

#endif
