//---------------------------------------------------------------------------

#ifndef CfgContsH
#define CfgContsH

#include <memory>
#include <vector>

#include <anafestica/CfgNodevalueType.h>

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------

class TConfigNode;

enum class Operation { None, Write, Erase };

using KeyType = String;

using ValueType = TConfigNodeValueType;

using ValuePairType = std::pair<ValueType,Operation>;
using TConfigNodePtr = std::unique_ptr<TConfigNode>;

using ValueContType = std::map<KeyType,ValuePairType>;
using NodeContType = std::map<KeyType,TConfigNodePtr>;

//---------------------------------------------------------------------------

inline
bool PutItemTo( ValueContType& Values, String Id,
                ValuePairType const & Val ) noexcept
{
    auto r = Values.insert( std::make_pair( Id, Val ) );
    if ( !r.second ) {
        if ( r.first->second.first != Val.first ) {
            r.first->second = ValuePairType( Val.first, Operation::Write );
        }
    }
    return r.second;
}
//---------------------------------------------------------------------------

inline
ValueType GetItemFrom( ValueContType& Values, String Id, ValueType DefVal,
                       Operation Op )
{
    auto r =
        Values.insert(
            std::make_pair( Id, std::make_pair( DefVal, Op ) )
        );
    return r.second ? DefVal : r.first->second.first;
}

//---------------------------------------------------------------------------
} // End of namespace Anafestica
//---------------------------------------------------------------------------
#endif
