//---------------------------------------------------------------------------

#ifndef CfgNodeValueTypeH
#define CfgNodeValueTypeH

#include <System.Classes.hpp>
#include <System.SysUtils.hpp>

#include <memory>
#include <vector>
#include <cstddef>
#include <optional>
#include <utility>
#include <algorithm>

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
      , System::Sysutils::TBytes
      , BytesCont
    >;

#define TT_I   i
#define TT_U   u
#define TT_L   l
#define TT_UL  ul
#define TT_C   c
#define TT_UC  uc
#define TT_S   s
#define TT_US  us
#define TT_LL  ll
#define TT_ULL ull
#define TT_B   b
#define TT_SZ  sz
#define TT_DT  dt
#define TT_FLT flt
#define TT_DBL dbl
#define TT_CUR cur
#define TT_SV  sv
#define TT_DAB dab
#define TT_VB  vb

enum class TypeTag : size_t {
    TT_I,   // int
    TT_U,   // unsigned int
    TT_L,   // long
    TT_UL,  // unsigned long
    TT_C,   // char
    TT_UC,  // unsigned char
    TT_S,   // short
    TT_US,  // unsigned short
    TT_LL,  // long long
    TT_ULL, // unsigned long long
    TT_B,   // bool
    TT_SZ,  // System::String
    TT_DT,  // System::TDateTime
    TT_FLT, // float
    TT_DBL, // double
    TT_CUR, // System::Currency
    TT_SV,  // StringCont
    TT_DAB, // System::Sysutils::TBytes
    TT_VB   // BytesCont
};

#define cnv_xstr( s ) cnv_str( s )
#define cnv_str( s )  #s

inline
std::optional<TypeTag> GetTypeTag( String Val )
{
    using Cont =
        std::array<
            std::pair<LPCTSTR,TypeTag>,
            TConfigNodeValueType::types::size::value
        >;

    // Keys must be sorted because will be used with std::lower_bound
    static constexpr Cont TypeIds {
        std::make_pair( _T( "" ) cnv_xstr( TT_B ),   TypeTag::b   ),
        std::make_pair( _T( "" ) cnv_xstr( TT_C ),   TypeTag::c   ),
        std::make_pair( _T( "" ) cnv_xstr( TT_CUR ), TypeTag::cur ),
        std::make_pair( _T( "" ) cnv_xstr( TT_DAB ), TypeTag::dab ),
        std::make_pair( _T( "" ) cnv_xstr( TT_DBL ), TypeTag::dbl ),
        std::make_pair( _T( "" ) cnv_xstr( TT_DT ),  TypeTag::dt  ),
        std::make_pair( _T( "" ) cnv_xstr( TT_FLT ), TypeTag::flt ),
        std::make_pair( _T( "" ) cnv_xstr( TT_I ),   TypeTag::i   ),
        std::make_pair( _T( "" ) cnv_xstr( TT_L ),   TypeTag::l   ),
        std::make_pair( _T( "" ) cnv_xstr( TT_LL ),  TypeTag::ll  ),
        std::make_pair( _T( "" ) cnv_xstr( TT_S ),   TypeTag::s   ),
        std::make_pair( _T( "" ) cnv_xstr( TT_SV ),  TypeTag::sv  ),
        std::make_pair( _T( "" ) cnv_xstr( TT_SZ ),  TypeTag::sz  ),
        std::make_pair( _T( "" ) cnv_xstr( TT_U ),   TypeTag::u   ),
        std::make_pair( _T( "" ) cnv_xstr( TT_UC ),  TypeTag::uc  ),
        std::make_pair( _T( "" ) cnv_xstr( TT_UL ),  TypeTag::ul  ),
        std::make_pair( _T( "" ) cnv_xstr( TT_ULL ), TypeTag::ull ),
        std::make_pair( _T( "" ) cnv_xstr( TT_US ),  TypeTag::us  ),
        std::make_pair( _T( "" ) cnv_xstr( TT_VB ),  TypeTag::vb  ),
    };

    auto TypeIdIt =
        std::lower_bound(
            std::begin( TypeIds ), std::end( TypeIds ),
            Val,
            []( auto const & Lhs, auto const & Rhs ) {
                return String( Lhs.first ) < Rhs;
            }
        );

    if ( TypeIdIt != std::end( TypeIds ) && String( TypeIdIt->first ) == Val ) {
        return std::optional<TypeTag>(
            TypeIds[std::distance( std::begin( TypeIds ), TypeIdIt )].second
        );
    }
    return std::nullopt;
}

//---------------------------------------------------------------------------
} // End of namespace Anafestica
//---------------------------------------------------------------------------
#endif


