//---------------------------------------------------------------------------

#ifndef CfgBSONCryptH
#define CfgBSONCryptH

#include <anafestica/CfgBSON.h>

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------
namespace BSONCrypt {
//---------------------------------------------------------------------------

class TConfig : public BSON::TConfig {
public:
    TConfig( String FileName, bool ReadOnly = false,
             bool FlushAllItems = false, bool ExplicitTypes = false,
             Crypt::TOptions Options = Crypt::TOptions::Default() )
        : BSON::TConfig(
            FileName, ReadOnly, FlushAllItems, ExplicitTypes, Options
          )
    {}

    TConfig( String LoadFileName, String SaveFileName,
             bool ReadOnly = false, bool ExplicitTypes = false,
             Crypt::TOptions Options = Crypt::TOptions::Default() )
        : BSON::TConfig(
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
} // End namespace BSONCrypt
//---------------------------------------------------------------------------
} // End namespace Anafestica
//---------------------------------------------------------------------------

#endif
