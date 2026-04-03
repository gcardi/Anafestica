//---------------------------------------------------------------------------

#ifndef CfgNodeValueTypeH
#define CfgNodeValueTypeH

#include <System.Classes.hpp>
#include <System.SysUtils.hpp>

#include <memory>
#include <vector>
#include <string>
#include <cstddef>
#include <optional>
#include <utility>
#include <algorithm>

// Compiler-specific configuration for variant usage
#if defined(__BORLANDC__) && !defined(__clang__)
  // Old BCC32: not supported, emit error
  #error "Old BCC32 compiler is not supported. Please use bcc32c, bcc64, or bcc64x compilers."
#endif

#if defined(__clang__) && defined(_WIN32) && !defined(__WIN64__)
  // bcc32c: use boost::variant (do not define ANAFESTICA_USE_STD_VARIANT)
  // ANAFESTICA_USE_STD_VARIANT is not defined here
#endif

#if defined(__clang__) && defined(__WIN64__) && defined(__BORLANDC__)
  // bcc64: use boost::variant (do not define ANAFESTICA_USE_STD_VARIANT)
  // ANAFESTICA_USE_STD_VARIANT is not defined here
#endif

#if defined(__clang__) && defined(__WIN64__) && !defined(__BORLANDC__)
  // bcc64x: use std::variant (define ANAFESTICA_USE_STD_VARIANT)
  #define ANAFESTICA_USE_STD_VARIANT
#endif

#if defined( ANAFESTICA_USE_STD_VARIANT )
# include <variant>
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
      , std::string                 // TT_STR str  (UTF-8)
      , std::wstring                // TT_WSTR wstr (UTF-16)
    >;

// Type identifiers -- prefixed to avoid macro namespace pollution

#define ANA_TT_I    i        // int
#define ANA_TT_U    u        // unsigned int
#define ANA_TT_L    l        // long
#define ANA_TT_UL   ul       // unsigned long
#define ANA_TT_C    c        // char
#define ANA_TT_UC   uc       // unsigned char
#define ANA_TT_S    s        // short
#define ANA_TT_US   us       // unsigned short
#define ANA_TT_LL   ll       // long long
#define ANA_TT_ULL  ull      // unsigned long long
#define ANA_TT_B    b        // bool
#define ANA_TT_SZ   sz       // System::String
#define ANA_TT_DT   dt       // System::TDateTime
#define ANA_TT_FLT  flt      // float
#define ANA_TT_DBL  dbl      // double
#define ANA_TT_CUR  cur      // System::Currency
#define ANA_TT_SV   sv       // StringCont aka std::vector<String>
#define ANA_TT_DAB  dab      // System::Sysutils::TBytes
#define ANA_TT_VB   vb       // BytesCont aka std::vector<Byte>
#define ANA_TT_STR  str      // std::string  (UTF-8)
#define ANA_TT_WSTR wstr     // std::wstring (UTF-16)

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
    TT_VB,  // BytesCont aka std::vector<Byte>
    TT_STR, // std::string  (UTF-8)
    TT_WSTR // std::wstring (UTF-16)
};

#define ana_cnv_xstr( s ) ana_cnv_str( s )
#define ana_cnv_str( s )  #s

[[nodiscard]] inline
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
    // Alphabetical order: b c cur dab dbl dt flt i l ll s str sv sz u uc ul ull us vb wstr
    static constexpr Cont TypeIds {
        std::make_pair( _D( "" ) ana_cnv_xstr( ANA_TT_B ),    TypeTag::TT_B    ),
        std::make_pair( _D( "" ) ana_cnv_xstr( ANA_TT_C ),    TypeTag::TT_C    ),
        std::make_pair( _D( "" ) ana_cnv_xstr( ANA_TT_CUR ),  TypeTag::TT_CUR  ),
        std::make_pair( _D( "" ) ana_cnv_xstr( ANA_TT_DAB ),  TypeTag::TT_DAB  ),
        std::make_pair( _D( "" ) ana_cnv_xstr( ANA_TT_DBL ),  TypeTag::TT_DBL  ),
        std::make_pair( _D( "" ) ana_cnv_xstr( ANA_TT_DT ),   TypeTag::TT_DT   ),
        std::make_pair( _D( "" ) ana_cnv_xstr( ANA_TT_FLT ),  TypeTag::TT_FLT  ),
        std::make_pair( _D( "" ) ana_cnv_xstr( ANA_TT_I ),    TypeTag::TT_I    ),
        std::make_pair( _D( "" ) ana_cnv_xstr( ANA_TT_L ),    TypeTag::TT_L    ),
        std::make_pair( _D( "" ) ana_cnv_xstr( ANA_TT_LL ),   TypeTag::TT_LL   ),
        std::make_pair( _D( "" ) ana_cnv_xstr( ANA_TT_S ),    TypeTag::TT_S    ),
        std::make_pair( _D( "" ) ana_cnv_xstr( ANA_TT_STR ),  TypeTag::TT_STR  ),
        std::make_pair( _D( "" ) ana_cnv_xstr( ANA_TT_SV ),   TypeTag::TT_SV   ),
        std::make_pair( _D( "" ) ana_cnv_xstr( ANA_TT_SZ ),   TypeTag::TT_SZ   ),
        std::make_pair( _D( "" ) ana_cnv_xstr( ANA_TT_U ),    TypeTag::TT_U    ),
        std::make_pair( _D( "" ) ana_cnv_xstr( ANA_TT_UC ),   TypeTag::TT_UC   ),
        std::make_pair( _D( "" ) ana_cnv_xstr( ANA_TT_UL ),   TypeTag::TT_UL   ),
        std::make_pair( _D( "" ) ana_cnv_xstr( ANA_TT_ULL ),  TypeTag::TT_ULL  ),
        std::make_pair( _D( "" ) ana_cnv_xstr( ANA_TT_US ),   TypeTag::TT_US   ),
        std::make_pair( _D( "" ) ana_cnv_xstr( ANA_TT_VB ),   TypeTag::TT_VB   ),
        std::make_pair( _D( "" ) ana_cnv_xstr( ANA_TT_WSTR ), TypeTag::TT_WSTR ),
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


