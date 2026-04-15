//---------------------------------------------------------------------------

#ifndef CfgItemsH
#define CfgItemsH

#include <System.Classes.hpp>

#include <utility>
#include <memory>
#include <algorithm>
#include <map>
#include <type_traits>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

#include <anafestica/CfgConts.h>

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------

using TConfigPath = std::vector<String>;

/// In-memory hierarchical node that stores typed configuration values.
///
/// Mirrors a registry key (or an XML/JSON/INI section): it owns a map of
/// named values (@ref ValueContType) and a map of named child nodes
/// (@ref NodeContType).  Values are stored as @ref TConfigNodeValueType
/// variants; the variant alternative is chosen at write time and checked
/// at read time via @c std::get_if / @c boost::get.
///
/// @par Type dispatch
/// @c GetItem and @c PutItem use a compile-time tag dispatch
/// (@c is_other_tag / @c is_enum_tag) to route enum types through
/// Delphi RTTI serialisation and all other types through the variant
/// directly.
///
/// @par Type-mismatch behaviour
/// If @c GetItem<T> is called but the stored variant alternative is not
/// @c T, the @c get_if probe returns @c nullptr, the assignment is
/// skipped, and the caller receives @c T{} (the default-initialised
/// value).  This is by design — no exception is thrown.
class TConfigNode
{
private:
    enum class tag_type { other_tg, enum_tg };

    template<tag_type> struct enum_tag {};

    using is_other_tag = enum_tag<tag_type::other_tg>;
    using is_enum_tag = enum_tag<tag_type::enum_tg>;

    template<typename T>
    struct type_to_enum {
        static constexpr tag_type value =
            std::is_enum_v<std::remove_cv_t<std::remove_reference_t<T>>> ?
              tag_type::enum_tg
            :
              tag_type::other_tg;
    };

    template<typename T>
    inline static constexpr auto type_to_enum_v = type_to_enum<T>::value;

public:
    TConfigNode() = default;
    TConfigNode( TConfigNode const & ) = delete;
    TConfigNode& operator=( TConfigNode const & ) = delete;

    /// Returns (or creates) a child node by name.
    ///
    /// If a child named @p Id does not exist yet, a new empty node is
    /// inserted and returned.  Subsequent calls with the same @p Id
    /// return the same node.
    TConfigNode& GetSubNode( String Id ) {
        auto p =
            nodeItems_.insert( NodeContType::value_type( Id, TConfigNodePtr{} ) );
        if ( p.second ) {
            p.first->second = std::move( std::make_unique<TConfigNode>() );
        }
        return *p.first->second;
    }

    TConfigNode& operator[]( String Id ) {
        return GetSubNode( Id );
    }

    template<typename R>
    void Read( R& Reader, TConfigPath const & Path );

    template<typename W>
    void Write( W& Writer, TConfigPath const & Path ) const;

    /// Reads a value by key, writing the result into @p Val.
    ///
    /// Dispatches to @c GetItemAs with either @c is_other_tag or
    /// @c is_enum_tag depending on whether @c T is an enum type.
    /// If the stored variant alternative does not match @c T, @p Val is
    /// left at its incoming value (typically @c T{} when called from the
    /// single-argument overload).
    template<typename T>
    void GetItem( String Id, T& Val, Operation Op = Operation::None ) {
        GetItemAs(
            enum_tag<type_to_enum_v<std::remove_reference_t<decltype( Val )>>>{},
            Id, Val, Op
        );
    }

    /// Reads a value by key, returning @c T{} on type mismatch or absence.
    ///
    /// Convenience overload that default-constructs @c T, calls the
    /// two-argument form, and returns the result.  Because @c Val starts
    /// as @c T{}, a type mismatch (stored variant alternative != @c T)
    /// produces the default value with no exception.
    template<typename T>
    [[nodiscard]] T GetItem( String Id, Operation Op = Operation::None ) {
        T Val {};
        GetItem( Id, Val, Op );
        return Val;
    }

    void GetItem( String Id, TStrings& Val, Operation Op = Operation::None ) {
        StringCont Strs;
        std::copy(
            System::begin( &Val ), System::end( &Val ),
            std::back_inserter( Strs )
        );
        auto& Result = GetItemFrom( valueItems_, Id, Strs, Op );
#if defined( ANAFESTICA_USE_STD_VARIANT )
        if ( auto* p = std::get_if<StringCont>( &Result ) ) {
#else
        if ( auto* p = boost::get<StringCont>( &Result ) ) {
#endif
            Val.Clear();
            std::copy(
                std::begin( *p ), std::end( *p ),
                System::back_inserter( &Val )
            );
        }
    }

    void GetItem( String Id, TStrings* const Val, Operation Op = Operation::None ) {
        GetItem( Id, *Val, Op );
    }

    /// Stores a value under the given key.
    ///
    /// The variant alternative is determined by @c T at compile time;
    /// backends will encode the corresponding type tag when flushing.
    /// Enum types are routed through Delphi RTTI (stored as @c String
    /// name when RTTI is available, otherwise as @c int).
    template<typename T>
    bool PutItem( String Id, T&& Val, Operation Op = Operation::Write ) {
        return PutItem(
            Id, std::forward<T>( Val ),
            enum_tag<type_to_enum_v<std::remove_reference_t<decltype( Val )>>>{},
            Op
        );
    }

    bool PutItem( String Id, TStrings& Val, Operation Op = Operation::Write ) {
        StringCont Strs;
        Strs.reserve( Val.Count );
        std::copy(
            System::begin( &Val ), System::end( &Val ),
            std::back_inserter( Strs )
        );
        return PutItem( Id, Strs, Op );
    }

    bool PutItem( String Id, TStrings* const Val, Operation Op = Operation::Write ) {
        return PutItem( Id, *Val, Op );
    }

#if defined( ANAFESTICA_USE_STD_VARIANT )
    // Convenience overloads: string_view/wstring_view are non-owning, so they
    // are materialised to string/wstring before being stored in the variant.
    // Only available on bcc64x: boost::variant on bcc64/bcc32c does not carry
    // std::string or std::wstring as alternatives (Boost 1.70 mpl::list limit).
    bool PutItem( String Id, std::string_view Val, Operation Op = Operation::Write ) {
        return PutItem( Id, std::string( Val ), Op );
    }

    bool PutItem( String Id, std::wstring_view Val, Operation Op = Operation::Write ) {
        return PutItem( Id, std::wstring( Val ), Op );
    }
#endif

    [[nodiscard]] size_t GetNodeCount() const noexcept { return nodeItems_.size(); }

    template<typename OutputIterator>
    void EnumerateNodes( OutputIterator Output ) const;

    [[nodiscard]] size_t GetValueCount() const noexcept {
        return std::count_if(
            std::begin( valueItems_ ), std::end( valueItems_ ),
            []( auto const & Val ) {
                return Val.second.second != Operation::Erase;
            }
        );
    }

    template<typename OutputIterator>
    void EnumeratePairs( OutputIterator Out ) const;

    template<typename OutputIterator>
    void EnumerateValueNames( OutputIterator Out ) const;

    template<typename OutputIterator>
    void EnumerateValues( OutputIterator Out ) const;

    void DeleteItem( String Id ) noexcept {
        auto i = valueItems_.find( Id );
        if ( i != std::end( valueItems_ ) ) {
            i->second.second = Operation::Erase;
        }
    }

    void DeleteSubNode( String Id ) {
        auto i = nodeItems_.find( Id );
        if ( i != std::end( nodeItems_ ) ) { i->second->Clear(); }
    }

    [[nodiscard]] bool IsDeleted() const noexcept { return deleted_; }

    [[nodiscard]] bool IsModified() const noexcept {
        return IsDeleted() || ValueListModified() || NodesModified();
    }

    void Clear() noexcept {
        deleted_ = true;
        valueItems_.clear();
        for ( auto& v : nodeItems_ ) { v.second->Clear(); }
    }

    [[nodiscard]] bool ItemExists( String Id ) const noexcept {
        return valueItems_.find( Id ) != std::end( valueItems_ );
    }

    [[nodiscard]] bool SubNodeExists( String Id ) const noexcept {
        return nodeItems_.find( Id ) != std::end( nodeItems_ );
    }

private:
    ValueContType valueItems_;
    NodeContType nodeItems_;
    bool deleted_ {};

    bool ValueListModified() const noexcept {
        return
            std::find_if(
                std::begin( valueItems_ ), std::end( valueItems_ ),
                []( auto const & v ) {
                    switch ( v.second.second ) {
                        case Operation::Write:
                        case Operation::Erase:
                            return true;
                        default:
                            return false;
                    }
                }
            ) != std::end( valueItems_ );
    }

    bool NodesModified() const noexcept {
        return
            std::find_if(
                std::begin( nodeItems_ ), std::end( nodeItems_ ),
                []( auto const & n ) { return n.second->IsModified(); }
            ) != std::end( nodeItems_ );
    }

    static bool IsValueDeleted( ValueContType::value_type const & Val ) noexcept {
        return Val.second.second == Operation::Erase;
    }

    /// Non-enum read path: probes the variant with @c get_if<T>.
    ///
    /// If the stored alternative matches @c T, @p Val is overwritten.
    /// Otherwise @c get_if returns @c nullptr and @p Val is left
    /// unchanged — this is the silent-default-on-mismatch contract.
    template<typename T>
    void GetItemAs( is_other_tag, String Id, T& Val, Operation Op ) {
        auto& Result = GetItemFrom( valueItems_, Id, Val, Op );
#if defined( ANAFESTICA_USE_STD_VARIANT )
        if ( auto* p = std::get_if<std::remove_reference_t<T>>( &Result ) ) {
            Val = *p;
        }
#else
        if ( auto* p = boost::get<std::remove_reference_t<T>>( &Result ) ) {
            Val = *p;
        }
#endif
    }

    /// Enum read path: uses Delphi RTTI when available.
    ///
    /// If @c __delphirtti(T) returns a valid type-info pointer the
    /// stored @c String name is converted back via @c GetEnumValue.
    /// Otherwise the enum is read as a plain @c int and @c static_cast
    /// to @c T.  In both cases the same @c get_if mismatch rule applies.
    template<typename T>
    void GetItemAs( is_enum_tag, String Id, T& Val, Operation Op );

    template<typename T>
    bool PutItem( String Id, T&& Val, is_other_tag, Operation Op = Operation::Write ) {
        return
            PutItemTo(
                valueItems_, Id,
                std::make_pair( ValueType{ std::forward<T>( Val ) }, Op )
            );
    }

    template<typename T>
    bool PutItem( String Id, T Val, is_enum_tag, Operation Op = Operation::Write );

};
//---------------------------------------------------------------------------

template<typename T>
void TConfigNode::GetItemAs( is_enum_tag, String Id, T& Val, Operation Op )
{
    // bcc64 (Clang < 15) has compatibility issues with __delphirtti and GetEnumValue on enums.
    // RSP-27417: Force integer-based enum serialization on bcc64 to work around RTTI bugs.
#if defined(__BORLANDC__) && defined(_WIN64) && !defined(__MINGW64__) && __clang_major__ < 15
    // bcc64: use integer-based enum handling
    auto& Result = GetItemFrom(
        valueItems_, Id, static_cast<int>( Val ), Op
    );
    if ( auto* p = boost::get<int>( &Result ) ) {
        Val = static_cast<T>( *p );
    }
#else
    // bcc64x and bcc32c: use RTTI-based enum handling
    if ( auto Info = __delphirtti( decltype( Val ) ) ) {
        auto& Result = GetItemFrom(
            valueItems_, Id,
            GetEnumName( Info, static_cast<int>( Val ) ), Op
        );
#if defined( ANAFESTICA_USE_STD_VARIANT )
        if ( auto* p = std::get_if<String>( &Result ) ) {
#else
        if ( auto* p = boost::get<String>( &Result ) ) {
#endif
            Val = static_cast<T>( GetEnumValue( Info, *p ) );
        }
    }
    else {
        auto& Result = GetItemFrom(
            valueItems_, Id, static_cast<int>( Val ), Op
        );
#if defined( ANAFESTICA_USE_STD_VARIANT )
        if ( auto* p = std::get_if<int>( &Result ) ) {
#else
        if ( auto* p = boost::get<int>( &Result ) ) {
#endif
            Val = static_cast<T>( *p );
        }
    }
#endif
}
//---------------------------------------------------------------------------

template<typename T>
bool TConfigNode::PutItem( String Id, T Val, is_enum_tag, Operation Op )
{
    // bcc64 (Clang < 15) has compatibility issues with __delphirtti and GetEnumValue on enums.
    // RSP-27417: Force integer-based enum serialization on bcc64 to work around RTTI bugs.
#if defined(__BORLANDC__) && defined(_WIN64) && !defined(__MINGW64__) && __clang_major__ < 15
    // bcc64: use integer-based serialization
    return PutItemTo(
        valueItems_, Id,
        std::make_pair( ValueType{ static_cast<int>( Val ) }, Op )
    );
#else
    // bcc64x and bcc32c: use RTTI-based serialization
    if ( auto Info = __delphirtti( decltype( Val ) ) ) {
        // save enum as text

        return PutItemTo(
            valueItems_, Id,
            std::make_pair(
                ValueType{ GetEnumName( Info, static_cast<int>( Val ) ) },
                Op
            )
        );
    }
    else {
        // save enum as integer
        return PutItemTo(
            valueItems_, Id,
            std::make_pair( ValueType{ static_cast<int>( Val ) }, Op )
        );
    }
#endif
}
//---------------------------------------------------------------------------

template<typename OutputIterator>
inline void TConfigNode::EnumerateNodes( OutputIterator Out ) const
{
    std::transform(
        std::begin( nodeItems_ ), std::end( nodeItems_ ), Out,
        []( auto const & v ){ return v.first; }
    );
}
//---------------------------------------------------------------------------

template<typename OutputIterator>
void TConfigNode::EnumerateValueNames( OutputIterator Out ) const
{
    std::for_each(
        std::begin( valueItems_ ), std::end( valueItems_ ),
        [&Out]( auto const & Val ) {
            if ( Val.second.second != Operation::Erase ) { *Out++ = Val.first; }
        }
    );
}
//---------------------------------------------------------------------------

template<typename OutputIterator>
void TConfigNode::EnumerateValues( OutputIterator Out ) const
{
    std::for_each(
        std::begin( valueItems_ ), std::end( valueItems_ ),
        [&Out]( auto const & Val ) {
            if ( Val.second.second != Operation::Erase ) {
                *Out++ = std::make_pair( Val.first, Val.second.first );
            }
        }
    );
}
//---------------------------------------------------------------------------

/// Recursively populates this node and all descendants from storage.
///
/// Calls @c Reader.CreateValueList to deserialise the values at @p Path,
/// then @c Reader.CreateNodeList to discover child nodes, and recurses
/// into each child with an extended path.  This is the read half of the
/// two-phase RAII lifecycle: construction loads, destruction flushes.
template<typename R>
void TConfigNode::Read( R& Reader, TConfigPath const & Path )
{
    valueItems_ = Reader.CreateValueList( Path );
    nodeItems_ = Reader.CreateNodeList( Path );
    TConfigPath TmpPath;
    TmpPath.reserve( Path.size() + 1 );
    TmpPath = Path;
    TmpPath.push_back({});
    for ( auto& n : nodeItems_ ) {
        TmpPath.back() = n.first;
        n.second->Read( Reader, TmpPath );
    }
}
//---------------------------------------------------------------------------

/// Recursively flushes this node and all descendants to storage.
///
/// Deleted nodes are removed first.  Modified value lists are saved via
/// @c Writer.SaveValueList.  When @c GetAlwaysFlushNodeFlag() is set
/// (used by backends that rewrite the entire file), every node is saved
/// regardless of its dirty state.
template<typename W>
void TConfigNode::Write( W& Writer, TConfigPath const & Path ) const
{
    if ( IsDeleted() ) {
        Writer.DeleteNode( Path );
    }
    if ( Writer.GetAlwaysFlushNodeFlag() || ValueListModified() ) {
        Writer.SaveValueList( Path, valueItems_ );
    }
    TConfigPath TmpPath;
    TmpPath.reserve( Path.size() + 1 );
    TmpPath = Path;
    TmpPath.push_back({});
    for ( auto const & n : nodeItems_ ) {
        TmpPath.back() = n.first;
        if ( Writer.GetAlwaysFlushNodeFlag() || n.second->IsModified() ) {
            n.second->Write( Writer, TmpPath );
        }
    }
}
//---------------------------------------------------------------------------

/// Convenience: reads a value from @p Node into @p Value.
///
/// Makes a temporary copy of the current value, calls @c GetItem to
/// fill it from the node, and assigns back.  This avoids modifying
/// @p Value if the key does not exist (the temporary keeps the old
/// value, and @c GetItem leaves it unchanged on mismatch/absence).
template<typename T>
void RestoreValue( TConfigNode& Node, String KeyName, T& Value )
{
    std::remove_reference_t< decltype( Value )> Tmp{ Value };
    Node.GetItem( KeyName, Tmp );
    Value = Tmp;
}

//---------------------------------------------------------------------------

template<typename T>
void SaveValue( TConfigNode& Node, String KeyName, T const & Value )
{
    Node.PutItem( KeyName, Value );
}

//---------------------------------------------------------------------------
} // End of namespace Anafestica
//---------------------------------------------------------------------------

#define RESTORE_PROPERTY( NODE, PROPERTY ) \
{\
    std::remove_reference_t< decltype( PROPERTY )> Tmp{ PROPERTY }; \
    ( NODE ).GetItem( #PROPERTY, Tmp ); \
    PROPERTY = Tmp; \
}

#define SAVE_PROPERTY( NODE, PROPERTY ) \
    ( NODE ).PutItem( #PROPERTY, PROPERTY )

#define RESTORE_ID_PROPERTY( NODE, ID, PROPERTY ) \
{\
    std::remove_reference_t< decltype( PROPERTY )> Tmp{ PROPERTY }; \
    ( NODE ).GetItem( #ID, Tmp ); \
    PROPERTY = Tmp; \
}

#define SAVE_ID_PROPERTY( NODE, ID, PROPERTY ) \
    ( NODE ).PutItem( #ID, PROPERTY )

//---------------------------------------------------------------------------

#endif

