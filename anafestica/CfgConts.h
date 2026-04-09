//---------------------------------------------------------------------------

#ifndef CfgContsH
#define CfgContsH

#include <memory>
#include <vector>

#include <anafestica/CfgNodeValueType.h>

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------

class TConfigNode;

/// Tracks the dirty state of a value in the in-memory container.
///
/// - @c None  — value was loaded from storage and has not been modified.
/// - @c Write — value has been created or changed; will be flushed on save.
/// - @c Erase — value is marked for deletion; backends remove it on flush.
enum class Operation { None, Write, Erase };

using KeyType = String;

using ValueType = TConfigNodeValueType;

using ValuePairType = std::pair<ValueType,Operation>;
using TConfigNodePtr = std::unique_ptr<TConfigNode>;

using ValueContType = std::map<KeyType,ValuePairType>;
using NodeContType = std::map<KeyType,TConfigNodePtr>;

//---------------------------------------------------------------------------

/// Inserts or updates a value in the container.
///
/// If @p Id does not exist, inserts the pair as-is and returns @c true.
/// If @p Id already exists and the stored variant differs from @p Val,
/// overwrites it and marks the entry as @c Operation::Write.
/// Returns @c false when the key was already present (regardless of
/// whether the value changed).
inline
bool PutItemTo( ValueContType& Values, String Id,
                ValuePairType const & Val )
{
    auto r = Values.insert( std::make_pair( Id, Val ) );
    if ( !r.second ) {
#if defined( ANAFESTICA_USE_STD_VARIANT )
        if ( !( r.first->second.first == Val.first ) ) {
#else
        if ( r.first->second.first != Val.first ) {
#endif
            r.first->second = ValuePairType( Val.first, Operation::Write );
        }
    }
    return r.second;
}
//---------------------------------------------------------------------------

/// Retrieves (or lazily creates) a value entry by key.
///
/// Attempts to insert @p DefVal under @p Id.  If the key already exists
/// the insert is a no-op and the *existing* variant is returned — this
/// is how values loaded from storage survive a subsequent @c GetItem
/// call with a different default.  The returned reference points into
/// the map and remains valid until the map is modified.
inline
ValueType& GetItemFrom( ValueContType& Values, String Id, ValueType DefVal,
                        Operation Op )
{
    auto r =
        Values.insert(
            std::make_pair( Id, std::make_pair( DefVal, Op ) )
        );
    return r.first->second.first;
}

//---------------------------------------------------------------------------
} // End of namespace Anafestica
//---------------------------------------------------------------------------
#endif
