//---------------------------------------------------------------------------
//
// Strict version-string parser used by the configuration-migration helpers.
//
// Grammar (regex):  ^\d+(?:\.\d+[a-zA-Z]*)(?:\.\d+){0,2}$
//
//   - First component: digits only.
//   - Second component: digits + optional letter suffix (case-insensitive).
//   - Up to two additional digits-only components (build, revision).
//
// Examples accepted: 1.2, 1.3a, 2.0beta, 1.0.2.3, 1.10a.0.0.
// Examples rejected: "1" (too few components), "1a.3" (letter on major),
// "1.0.0a" (letter on build/revision), "1.2.3.4.5" (too many components),
// "1.0RC1" (digits after letters within one component).
//
// Malformed input throws a Delphi-style System::Sysutils::Exception with a
// meaningful message.  Comparison treats absent components as zero, so
// "1.1" == "1.1.0" == "1.1.0.0" and "1.0.2.3" < "1.1".  Letter suffixes
// sort lexicographically with "" < "a" < "b", so "1.1" < "1.1a" < "1.1b".
//
//---------------------------------------------------------------------------

#ifndef VersionH
#define VersionH

#include <System.hpp>
#include <System.SysUtils.hpp>

#include <exception>
#include <regex>
#include <string>
#include <tuple>

#if !defined(__clang__)
  #error "Old legacy BCC32 is not supported. Use bcc32c, bcc64, or bcc64x."
#endif

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------

/// Parsed version string (M.m[suffix][.b][.r]).
///
/// Construct from a System::String; throws if the string does not match
/// the documented grammar.  Comparison is total and lexicographic over
/// (major, minor, lowercased suffix, build, revision); missing trailing
/// components compare as zero.
class TVersion {
public:
    /// Default-constructed @c TVersion represents version @c 0.0.0.0 (all
    /// components zero, empty suffix).  Provided so that this type can be
    /// stored in containers — notably @c std::optional under the older
    /// Dinkumware standard library shipped with bcc64 / bcc32c, which
    /// requires the wrapped type to be default constructible.
    TVersion() = default;

    explicit TVersion( String Text )
    {
        // Five capture groups, in order: major, minor, suffix, build, revision.
        // Suffix is optional (may match empty); build/revision are each optional
        // but build must precede revision (matches the user-supplied grammar
        // ^\d+(?:\.\d+[a-zA-Z]*)(?:\.\d+){0,2}$ exactly).
        static std::wregex const Pattern(
            L"^(\\d+)\\.(\\d+)([a-zA-Z]*)(?:\\.(\\d+)(?:\\.(\\d+))?)?$"
        );

        std::wstring const W( Text.c_str(), static_cast<size_t>( Text.Length() ) );
        std::wsmatch Match;
        if ( !std::regex_match( W, Match, Pattern ) ) {
            throw Exception(
                Format(
                    _D( "Malformed version string: \"%s\"" ),
                    ARRAYOFCONST(( Text ))
                )
            );
        }

        try {
            major_ = std::stoull( Match[1].str() );
            minor_ = std::stoull( Match[2].str() );
            if ( Match[3].matched && Match[3].length() > 0 ) {
                minorSuffix_ = String( Match[3].str().c_str() ).LowerCase();
            }
            if ( Match[4].matched ) { build_    = std::stoull( Match[4].str() ); }
            if ( Match[5].matched ) { revision_ = std::stoull( Match[5].str() ); }
        }
        catch ( std::exception const & E ) {
            throw Exception(
                Format(
                    _D( "Version string \"%s\" contains a numeric component out of range: %s" ),
                    ARRAYOFCONST(( Text, String( E.what() ) ))
                )
            );
        }
    }

    [[nodiscard]] unsigned long long Major()    const noexcept { return major_; }
    [[nodiscard]] unsigned long long Minor()    const noexcept { return minor_; }
    [[nodiscard]] String             MinorSuffix() const         { return minorSuffix_; }
    [[nodiscard]] unsigned long long Build()    const noexcept { return build_; }
    [[nodiscard]] unsigned long long Revision() const noexcept { return revision_; }

    // Ordering is lexicographic over (major, minor, suffix, build, revision).
    // String comparison is via UnicodeString::operator< / operator==, so the
    // numeric components stay numeric and the suffix sorts lexically with
    // "" < "a" < "b" (suffix is lowercased at parse time, so the comparison
    // is effectively case-insensitive).
    bool operator<( TVersion const & Other ) const {
        return std::tie( major_, minor_, minorSuffix_, build_, revision_ )
             < std::tie( Other.major_, Other.minor_, Other.minorSuffix_,
                         Other.build_, Other.revision_ );
    }

    bool operator==( TVersion const & Other ) const {
        return std::tie( major_, minor_, minorSuffix_, build_, revision_ )
            == std::tie( Other.major_, Other.minor_, Other.minorSuffix_,
                         Other.build_, Other.revision_ );
    }

private:
    unsigned long long major_    {};
    unsigned long long minor_    {};
    String             minorSuffix_ {};
    unsigned long long build_    {};
    unsigned long long revision_ {};
};

inline bool operator!=( TVersion const & A, TVersion const & B ) { return !( A == B ); }
inline bool operator> ( TVersion const & A, TVersion const & B ) { return   B < A;     }
inline bool operator<=( TVersion const & A, TVersion const & B ) { return !( B < A );  }
inline bool operator>=( TVersion const & A, TVersion const & B ) { return !( A < B );  }

//---------------------------------------------------------------------------
} // End of namespace Anafestica
//---------------------------------------------------------------------------

#endif
