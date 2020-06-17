//---------------------------------------------------------------------------

#ifndef CfgNodeValueTypeH
#define CfgNodeValueTypeH

#include <System.Classes.hpp>

#include <memory>
#include <vector>

#include <boost/variant.hpp>

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------

using StringCont = std::vector<String>;
using BytesCont = std::vector<Byte>;

using TConfigNodeValueType =
    boost::variant<
        int
      , unsigned int
      , long
      , unsigned long
      , char
      , unsigned char
      , short
      , unsigned short
      , long long
      , unsigned long long
      , bool
      , System::String
      , System::TDateTime
      , float
      , double
      , System::Currency
      , StringCont
      , System::TBytes
      , BytesCont
    >;

//---------------------------------------------------------------------------
} // End of namespace Anafestica
//---------------------------------------------------------------------------
#endif
