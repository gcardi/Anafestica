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
#include <vector>

#include <anafestica/CfgConts.h>

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------

using TConfigPath = std::vector<String>;

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

    template<typename T>
    void GetItem( String Id, T& Val, Operation Op = Operation::None ) {
        GetItemAs(
            enum_tag<type_to_enum_v<std::remove_reference_t<decltype( Val )>>>{},
            Id, Val, Op
        );
    }

    template<typename T>
    T GetItem( String Id, Operation Op = Operation::None ) {
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
        auto NewStrs =
            boost::get<StringCont>(
                GetItemFrom( valueItems_, Id, Strs, Op )
            );
        Val.Clear();
        std::copy(
            std::begin( NewStrs ), std::end( NewStrs ),
            System::back_inserter( &Val )
        );
    }

    void GetItem( String Id, TStrings* const Val, Operation Op = Operation::None ) {
        GetItem( Id, *Val, Op );
    }

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

    size_t GetNodeCount() const noexcept { return nodeItems_.size(); }

    template<typename OutputIterator>
    void EnumerateNodes( OutputIterator Output ) const;

    size_t GetValueCount() const noexcept {
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

    void DeleteItem( String Id ) {
        auto i = valueItems_.find( Id );
        if ( i != std::end( valueItems_ ) ) {
            i->second.second = Operation::Erase;
        }
    }

    void DeleteSubNode( String Id ) {
        auto i = nodeItems_.find( Id );
        if ( i != std::end( nodeItems_ ) ) { i->second->Clear(); }
    }

    bool IsDeleted() const noexcept { return deleted_; }

    bool IsModified() const noexcept {
        return IsDeleted() || ValueListModified() || NodesModified();
    }

    void Clear() {
        deleted_ = true;
        valueItems_.clear();
        for ( auto& v : nodeItems_ ) { v.second->Clear(); }
    }

    bool ItemExists( String Id ) const noexcept {
        return valueItems_.find( Id ) != std::end( valueItems_ );
    }

    bool SubNodeExists( String Id ) const noexcept {
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

    template<typename T>
    void GetItemAs( is_other_tag, String Id, T& Val, Operation Op ) {
        Val =
            boost::get<std::remove_reference_t<T>>(
                GetItemFrom( valueItems_, Id, Val, Op )
            );
    }

    template<typename T>
    void GetItemAs( is_enum_tag, String Id, T& Val, Operation Op );

    template<typename T>
    bool PutItem( String Id, T&& Val, is_other_tag, Operation Op = Operation::Write ) noexcept {
        return
            PutItemTo(
                valueItems_, Id,
                std::make_pair( ValueType{ std::forward<T>( Val ) }, Op )
            );
    }

    template<typename T>
    bool PutItem( String Id, T Val, is_enum_tag, Operation Op = Operation::Write ) noexcept;

};
//---------------------------------------------------------------------------

template<typename T>
void TConfigNode::GetItemAs( is_enum_tag, String Id, T& Val, Operation Op )
{
    if ( auto Info = __delphirtti( decltype( Val ) ) ) {
        Val =
            static_cast<T>(
                GetEnumValue(
                    Info,
                    boost::get<String>(
                        GetItemFrom(
                            valueItems_,
                            Id,
                            GetEnumName( Info, static_cast<int>( Val ) ),
                            Op
                        )
                    )
                )
            );
    }
    else {
        Val =
            static_cast<T>(
                boost::get<int>(
                    GetItemFrom(
                        valueItems_,
                        Id,
                        static_cast<int>( Val ),
                        Op
                    )
                )
            );
    }
}
//---------------------------------------------------------------------------

template<typename T>
bool TConfigNode::PutItem( String Id, T Val, is_enum_tag, Operation Op ) noexcept
{
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

