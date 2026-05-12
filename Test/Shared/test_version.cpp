//---------------------------------------------------------------------------
// Tests for the TVersion value type used by the migration helpers.
//
// Covers:
//   - grammar acceptance (M.m, M.m[suffix], M.m.b, M.m.b.r, M.m[suffix].b.r)
//   - grammar rejection  (single-component, letters on wrong component,
//                         too many components, garbage)
//   - ordering rules     (numeric per component, suffix > absent suffix,
//                         missing trailing components compare as zero)
//   - case-insensitive suffix
//   - exception message contains the offending input
//---------------------------------------------------------------------------

#pragma hdrstop

//---------------------------------------------------------------------------
#pragma package(smart_init)

#include <boost/test/unit_test.hpp>

#include <anafestica/Version.h>

#include <System.SysUtils.hpp>

BOOST_AUTO_TEST_SUITE( version )

//---------------------------------------------------------------------------
// Grammar acceptance
//---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( ParsesTwoComponents )
{
    Anafestica::TVersion const V( _D( "1.2" ) );
    BOOST_TEST( V.Major() == 1U );
    BOOST_TEST( V.Minor() == 2U );
    BOOST_TEST( V.MinorSuffix() == String() );
    BOOST_TEST( V.Build() == 0U );
    BOOST_TEST( V.Revision() == 0U );
}

BOOST_AUTO_TEST_CASE( ParsesTwoComponentsWithSuffix )
{
    Anafestica::TVersion const V( _D( "1.3a" ) );
    BOOST_TEST( V.Major() == 1U );
    BOOST_TEST( V.Minor() == 3U );
    BOOST_TEST( V.MinorSuffix() == String( _D( "a" ) ) );
    BOOST_TEST( V.Build() == 0U );
    BOOST_TEST( V.Revision() == 0U );
}

BOOST_AUTO_TEST_CASE( ParsesThreeComponents )
{
    Anafestica::TVersion const V( _D( "1.2.3" ) );
    BOOST_TEST( V.Major() == 1U );
    BOOST_TEST( V.Minor() == 2U );
    BOOST_TEST( V.MinorSuffix() == String() );
    BOOST_TEST( V.Build() == 3U );
    BOOST_TEST( V.Revision() == 0U );
}

BOOST_AUTO_TEST_CASE( ParsesFourComponents )
{
    Anafestica::TVersion const V( _D( "1.0.2.3" ) );
    BOOST_TEST( V.Major() == 1U );
    BOOST_TEST( V.Minor() == 0U );
    BOOST_TEST( V.MinorSuffix() == String() );
    BOOST_TEST( V.Build() == 2U );
    BOOST_TEST( V.Revision() == 3U );
}

BOOST_AUTO_TEST_CASE( ParsesSuffixOnMinorWithTrailingComponents )
{
    Anafestica::TVersion const V( _D( "1.2a.0.0" ) );
    BOOST_TEST( V.Major() == 1U );
    BOOST_TEST( V.Minor() == 2U );
    BOOST_TEST( V.MinorSuffix() == String( _D( "a" ) ) );
    BOOST_TEST( V.Build() == 0U );
    BOOST_TEST( V.Revision() == 0U );
}

BOOST_AUTO_TEST_CASE( ParsesMultiCharSuffix )
{
    Anafestica::TVersion const V( _D( "2.0beta" ) );
    BOOST_TEST( V.Major() == 2U );
    BOOST_TEST( V.Minor() == 0U );
    BOOST_TEST( V.MinorSuffix() == String( _D( "beta" ) ) );
}

BOOST_AUTO_TEST_CASE( ParsesMultiDigitComponents )
{
    Anafestica::TVersion const V( _D( "10.20.30.40" ) );
    BOOST_TEST( V.Major()    == 10U );
    BOOST_TEST( V.Minor()    == 20U );
    BOOST_TEST( V.Build()    == 30U );
    BOOST_TEST( V.Revision() == 40U );
}

//---------------------------------------------------------------------------
// Grammar rejection
//---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( RejectsSingleComponent )
{
    BOOST_CHECK_THROW( Anafestica::TVersion( _D( "1" ) ), Exception );
}

BOOST_AUTO_TEST_CASE( RejectsLetterOnMajor )
{
    BOOST_CHECK_THROW( Anafestica::TVersion( _D( "1a.3" ) ), Exception );
}

BOOST_AUTO_TEST_CASE( RejectsLetterOnBuildOrRevision )
{
    BOOST_CHECK_THROW( Anafestica::TVersion( _D( "1.0.0a" ) ),    Exception );
    BOOST_CHECK_THROW( Anafestica::TVersion( _D( "1.0.04.0a" ) ), Exception );
}

BOOST_AUTO_TEST_CASE( RejectsTooManyComponents )
{
    BOOST_CHECK_THROW( Anafestica::TVersion( _D( "1.2.3.4.5" ) ),   Exception );
    BOOST_CHECK_THROW( Anafestica::TVersion( _D( "1.2a.0.0.3" ) ), Exception );
}

BOOST_AUTO_TEST_CASE( RejectsDigitsAfterLettersInOneComponent )
{
    BOOST_CHECK_THROW( Anafestica::TVersion( _D( "1.0RC1" ) ), Exception );
}

BOOST_AUTO_TEST_CASE( RejectsGarbage )
{
    BOOST_CHECK_THROW( Anafestica::TVersion( _D( "" ) ),       Exception );
    BOOST_CHECK_THROW( Anafestica::TVersion( _D( "abc" ) ),    Exception );
    BOOST_CHECK_THROW( Anafestica::TVersion( _D( "1." ) ),     Exception );
    BOOST_CHECK_THROW( Anafestica::TVersion( _D( ".1" ) ),     Exception );
    BOOST_CHECK_THROW( Anafestica::TVersion( _D( "1.2 " ) ),   Exception );
    BOOST_CHECK_THROW( Anafestica::TVersion( _D( " 1.2" ) ),   Exception );
}

BOOST_AUTO_TEST_CASE( MalformedExceptionMentionsInput )
{
    try {
        Anafestica::TVersion V( _D( "definitely-not-a-version" ) );
        BOOST_FAIL( "Expected exception for malformed input" );
    }
    catch ( Exception const & E ) {
        BOOST_TEST( E.Message.Pos( _D( "definitely-not-a-version" ) ) > 0 );
    }
}

//---------------------------------------------------------------------------
// Ordering
//---------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE( MajorMinorOrdering )
{
    BOOST_TEST( ( Anafestica::TVersion( _D( "1.0" ) ) < Anafestica::TVersion( _D( "1.1" ) ) ) );
    BOOST_TEST( ( Anafestica::TVersion( _D( "1.9" ) ) < Anafestica::TVersion( _D( "2.0" ) ) ) );
    BOOST_TEST( ( Anafestica::TVersion( _D( "1.9" ) ) < Anafestica::TVersion( _D( "1.10" ) ) ) );  // numeric, not lex
}

BOOST_AUTO_TEST_CASE( SuffixSortsAfterAbsentSuffix )
{
    BOOST_TEST( ( Anafestica::TVersion( _D( "1.1" ) )  < Anafestica::TVersion( _D( "1.1a" ) ) ) );
    BOOST_TEST( ( Anafestica::TVersion( _D( "1.1a" ) ) < Anafestica::TVersion( _D( "1.1b" ) ) ) );
}

BOOST_AUTO_TEST_CASE( SuffixDoesNotOverrideMinorBump )
{
    BOOST_TEST( ( Anafestica::TVersion( _D( "1.1z" ) ) < Anafestica::TVersion( _D( "1.2" ) ) ) );
}

BOOST_AUTO_TEST_CASE( MissingTrailingComponentsCompareAsZero )
{
    BOOST_TEST( ( Anafestica::TVersion( _D( "1.1" ) )   == Anafestica::TVersion( _D( "1.1.0" ) )   ) );
    BOOST_TEST( ( Anafestica::TVersion( _D( "1.1" ) )   == Anafestica::TVersion( _D( "1.1.0.0" ) ) ) );
    BOOST_TEST( ( Anafestica::TVersion( _D( "1.1.0" ) ) == Anafestica::TVersion( _D( "1.1.0.0" ) ) ) );
    BOOST_TEST( ( Anafestica::TVersion( _D( "1.0.2.3" ) ) < Anafestica::TVersion( _D( "1.1" ) )    ) );
}

BOOST_AUTO_TEST_CASE( SuffixIsCaseInsensitive )
{
    Anafestica::TVersion const Lower( _D( "1.1a" ) );
    Anafestica::TVersion const Upper( _D( "1.1A" ) );
    BOOST_TEST( ( Lower == Upper ) );
    BOOST_TEST( !( Lower < Upper ) );
    BOOST_TEST( !( Upper < Lower ) );
}

BOOST_AUTO_TEST_CASE( ComparisonOperatorsAreConsistent )
{
    Anafestica::TVersion const A( _D( "1.1" ) );
    Anafestica::TVersion const B( _D( "1.2" ) );
    BOOST_TEST( ( A != B ) );
    BOOST_TEST( ( A <  B ) );
    BOOST_TEST( ( B >  A ) );
    BOOST_TEST( ( A <= B ) );
    BOOST_TEST( ( B >= A ) );
    BOOST_TEST( ( A <= A ) );
    BOOST_TEST( ( A >= A ) );
}

BOOST_AUTO_TEST_SUITE_END()
