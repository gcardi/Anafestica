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

// Compiler detection for variant selection.
//
//   bcc64x  (_WIN64, Clang >= 15): std::variant works correctly — selected automatically.
//   bcc64   (_WIN64, Clang <  15): std::variant has an assignment bug (RSP-27418); boost::variant used.
//   bcc32c  (_WIN32, Clang       ): boost::variant used.
//   BCC32   (non-Clang           ): not supported.
//
// ANAFESTICA_USE_STD_VARIANT is the internal flag that switches between the two;
// it is set here automatically and should not need to be defined manually.
#if !defined(__clang__)
  #error "Old legacy BCC32 is not supported. Use bcc32c, bcc64, or bcc64x."
#elif defined(_WIN64)
  #if __clang_major__ >= 15
    // bcc64x: std::variant is fully functional
    #define ANAFESTICA_USE_STD_VARIANT
  #endif
  // bcc64 (Clang < 15): falls through — boost::variant is used below
#endif
// bcc32c (_WIN32 + Clang): falls through — boost::variant is used below

#if defined( ANAFESTICA_USE_STD_VARIANT )
# include <variant>
#else
  // boost::variant path (bcc64/bcc32c): 19 types, within Boost 1.70's
  // hardcoded mpl::list limit of 20. No limit overrides needed.
# include <boost/variant.hpp>
#endif

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------

using StringCont = std::vector<String>;
using BytesCont = std::vector<Byte>;

/// Heterogeneous variant holding any single configuration value.
///
/// Each alternative corresponds to a unique type tag (see @ref TypeTag).
/// Backends encode the tag into the storage format (e.g. a suffix in the
/// value name for Registry/INI, a "type" attribute for XML, a nested key
/// for JSON) so that the correct alternative can be reconstructed on read.
///
/// Because every C++ type occupies a distinct alternative, reading a value
/// with a type that differs from the one stored will cause @c std::get_if
/// (or @c boost::get) to return @c nullptr, and @ref TConfigNode::GetItem
/// will silently return @c T{}.
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
#if defined( ANAFESTICA_USE_STD_VARIANT )
      // std::string and std::wstring require std::variant (bcc64x only).
      // Boost 1.70's mpl::list is hardcoded to 20 types, and we already
      // use 19 above, so adding two more would exceed the limit on bcc64/bcc32c.
      , std::string                 // TT_STR str  (UTF-8)
      , std::wstring                // TT_WSTR wstr (UTF-16)
#endif
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
// ANA_TT_STR and ANA_TT_WSTR are only available on bcc64x (std::variant path).
// On bcc64/bcc32c, std::string and std::wstring are not variant alternatives.
#if defined( ANAFESTICA_USE_STD_VARIANT )
#define ANA_TT_STR  str      // std::string  (UTF-8)
#define ANA_TT_WSTR wstr     // std::wstring (UTF-16)
#endif

/// Identifies the variant alternative stored in @ref TConfigNodeValueType.
///
/// Each enumerator maps 1:1 to an alternative in the variant and to a
/// short text tag (e.g. @c "i", @c "sz", @c "dbl") used by backends to
/// encode the type in the storage format.  See @ref GetTypeTag for the
/// reverse mapping from text to enumerator.
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
#if defined( ANAFESTICA_USE_STD_VARIANT )
    TT_STR, // std::string  (UTF-8)  — bcc64x only
    TT_WSTR // std::wstring (UTF-16) — bcc64x only
#endif
};

#define ana_cnv_xstr( s ) ana_cnv_str( s )
#define ana_cnv_str( s )  #s

/// Maps a type-tag string (e.g. @c "i", @c "sz", @c "dbl") to its
/// @ref TypeTag enumerator.
///
/// Uses binary search over a sorted @c constexpr array of tag strings.
/// On bcc64x (std::variant path) the array has 21 entries including
/// @c "str" and @c "wstr".  On bcc64/bcc32c (boost::variant path) those
/// two tags are absent and the array has 19 entries.
/// Returns @c std::nullopt when @p Val does not match any known tag,
/// which backends use to skip unrecognised entries.
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

    // Keys must be sorted for std::lower_bound.
    // bcc64x  (21 entries): b c cur dab dbl dt flt i l ll s str sv sz u uc ul ull us vb wstr
    // bcc64/bcc32c (19 entries): b c cur dab dbl dt flt i l ll s sv sz u uc ul ull us vb
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
#if defined( ANAFESTICA_USE_STD_VARIANT )
        std::make_pair( _D( "" ) ana_cnv_xstr( ANA_TT_STR ),  TypeTag::TT_STR  ),
#endif
        std::make_pair( _D( "" ) ana_cnv_xstr( ANA_TT_SV ),   TypeTag::TT_SV   ),
        std::make_pair( _D( "" ) ana_cnv_xstr( ANA_TT_SZ ),   TypeTag::TT_SZ   ),
        std::make_pair( _D( "" ) ana_cnv_xstr( ANA_TT_U ),    TypeTag::TT_U    ),
        std::make_pair( _D( "" ) ana_cnv_xstr( ANA_TT_UC ),   TypeTag::TT_UC   ),
        std::make_pair( _D( "" ) ana_cnv_xstr( ANA_TT_UL ),   TypeTag::TT_UL   ),
        std::make_pair( _D( "" ) ana_cnv_xstr( ANA_TT_ULL ),  TypeTag::TT_ULL  ),
        std::make_pair( _D( "" ) ana_cnv_xstr( ANA_TT_US ),   TypeTag::TT_US   ),
        std::make_pair( _D( "" ) ana_cnv_xstr( ANA_TT_VB ),   TypeTag::TT_VB   ),
#if defined( ANAFESTICA_USE_STD_VARIANT )
        std::make_pair( _D( "" ) ana_cnv_xstr( ANA_TT_WSTR ), TypeTag::TT_WSTR ),
#endif
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


