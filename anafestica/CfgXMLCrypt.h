//---------------------------------------------------------------------------

#ifndef CfgXMLCryptH
#define CfgXMLCryptH

#include <anafestica/CfgXML.h>

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------
namespace XMLCrypt {
//---------------------------------------------------------------------------

class TConfig : public XML::TConfig {
public:
    TConfig( String FileName, bool ReadOnly = false,
             bool FlushAllItems = false,
             Crypt::TOptions Options = Crypt::TOptions::Default() )
        : XML::TConfig( FileName, ReadOnly, FlushAllItems, Options )
    {}

    TConfig( String LoadFileName, String SaveFileName,
             bool ReadOnly = false,
             Crypt::TOptions Options = Crypt::TOptions::Default() )
        : XML::TConfig( LoadFileName, SaveFileName, ReadOnly, Options )
    {}

    static TConfig Migrate( String LoadFileName, String SaveFileName,
                            bool ReadOnly = false,
                            Crypt::TOptions Options = Crypt::TOptions::Default() )
    {
        return TConfig( LoadFileName, SaveFileName, ReadOnly, Options );
    }
};

//---------------------------------------------------------------------------
} // End namespace XMLCrypt
//---------------------------------------------------------------------------
} // End namespace Anafestica
//---------------------------------------------------------------------------

#endif
