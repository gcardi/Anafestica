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

#if defined( ANAFESTICA_USE_STD_VARIANT )
# include <variant>
# include <any>
#else
# include <boost/variant.hpp>
#endif

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------

using StringCont = std::vector<String>;
using BytesCont = std::vector<Byte>;

using TConfigNodeValueType =
#if defined( ANAFESTICA_USE_STD_VARIANT )
    std::variant<
#else
    boost::variant<
#endif
        int                         // TT_I   i
      , unsigned int                // TT_U   u
      , long                        // TT_L   l
      , unsigned long               // TT_UL  ul
      , char                        // TT_C   c
      , unsigned char               // TT_UC  uc
      , short                       // TT_S   s
      , unsigned short              // TT_US  us
      , long long                   // TT_LL  ll
      , unsigned long long          // TT_ULL ull
      , bool                        // TT_B   b
      , System::String              // TT_SZ  sz
      , System::TDateTime           // TT_DT  dt
      , float                       // TT_FLT flt
      , double                      // TT_DBL dbl
      , System::Currency            // TT_CUR cur
      , StringCont                  // TT_SV  sv
      , System::Sysutils::TBytes    // TT_DAB dab
      , BytesCont                   // TT_VB  vb
    >;

// Type identifiers

#define TT_I   i        // int
#define TT_U   u        // unsigned int
#define TT_L   l        // long
#define TT_UL  ul       // unsigned long
#define TT_C   c        // char
#define TT_UC  uc       // unsigned char
#define TT_S   s        // short
#define TT_US  us       // unsigned short
#define TT_LL  ll       // long long
#define TT_ULL ull      // unsigned long long
#define TT_B   b        // bool
#define TT_SZ  sz       // System::String
#define TT_DT  dt       // System::TDateTime
#define TT_FLT flt      // float
#define TT_DBL dbl      // double
#define TT_CUR cur      // System::Currency
#define TT_SV  sv       // StringCont aka std::vector<String>
#define TT_DAB dab      // System::Sysutils::TBytes
#define TT_VB  vb       // BytesCont aka std::vector<Byte>

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
    TT_SV,  // StringCont aka std::vector<String>
    TT_DAB, // System::Sysutils::TBytes
    TT_VB   // BytesCont aka std::vector<Byte>
};

#define cnv_xstr( s ) cnv_str( s )
#define cnv_str( s )  #s

inline
std::optional<TypeTag> GetTypeTag( String Val )
{
    using Cont =
        std::array<
            std::pair<LPCTSTR,TypeTag>,
#if defined( ANAFESTICA_USE_STD_VARIANT )
            std::variant_size<TConfigNodeValueType>::value
#else
            TConfigNodeValueType::types::size::value
#endif
        >;

    // Keys must be sorted because will be used with std::lower_bound
    static constexpr Cont TypeIds {
        std::make_pair( _D( "" ) cnv_xstr( TT_B ),   TypeTag::b   ),
        std::make_pair( _D( "" ) cnv_xstr( TT_C ),   TypeTag::c   ),
        std::make_pair( _D( "" ) cnv_xstr( TT_CUR ), TypeTag::cur ),
        std::make_pair( _D( "" ) cnv_xstr( TT_DAB ), TypeTag::dab ),
        std::make_pair( _D( "" ) cnv_xstr( TT_DBL ), TypeTag::dbl ),
        std::make_pair( _D( "" ) cnv_xstr( TT_DT ),  TypeTag::dt  ),
        std::make_pair( _D( "" ) cnv_xstr( TT_FLT ), TypeTag::flt ),
        std::make_pair( _D( "" ) cnv_xstr( TT_I ),   TypeTag::i   ),
        std::make_pair( _D( "" ) cnv_xstr( TT_L ),   TypeTag::l   ),
        std::make_pair( _D( "" ) cnv_xstr( TT_LL ),  TypeTag::ll  ),
        std::make_pair( _D( "" ) cnv_xstr( TT_S ),   TypeTag::s   ),
        std::make_pair( _D( "" ) cnv_xstr( TT_SV ),  TypeTag::sv  ),
        std::make_pair( _D( "" ) cnv_xstr( TT_SZ ),  TypeTag::sz  ),
        std::make_pair( _D( "" ) cnv_xstr( TT_U ),   TypeTag::u   ),
        std::make_pair( _D( "" ) cnv_xstr( TT_UC ),  TypeTag::uc  ),
        std::make_pair( _D( "" ) cnv_xstr( TT_UL ),  TypeTag::ul  ),
        std::make_pair( _D( "" ) cnv_xstr( TT_ULL ), TypeTag::ull ),
        std::make_pair( _D( "" ) cnv_xstr( TT_US ),  TypeTag::us  ),
        std::make_pair( _D( "" ) cnv_xstr( TT_VB ),  TypeTag::vb  ),
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


