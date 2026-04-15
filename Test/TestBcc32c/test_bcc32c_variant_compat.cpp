//---------------------------------------------------------------------------
// bcc32c compatibility-focused regression tests.
//---------------------------------------------------------------------------

#pragma hdrstop

//---------------------------------------------------------------------------
#pragma package(smart_init)

#include <boost/test/unit_test.hpp>

#include <anafestica/CfgItems.h>

#include <System.SysUtils.hpp>

namespace {

enum class LegacyEnum {
    Zero = 0,
    One = 1,
    Two = 2
};

}

BOOST_AUTO_TEST_SUITE( test_bcc32c_compat )

BOOST_AUTO_TEST_CASE( PrimitiveAndEnumRoundtrip )
{
#if defined(__BORLANDC__) && !defined(_WIN64)
    Anafestica::TConfigNode node;

    const int expectedInt = 123;
    const long expectedLong = -456L;
    const unsigned long expectedULong = 789UL;
    const LegacyEnum expectedEnum = LegacyEnum::Two;

    BOOST_TEST( node.PutItem( _D("I"), expectedInt ) );
    BOOST_TEST( node.PutItem( _D("L"), expectedLong ) );
    BOOST_TEST( node.PutItem( _D("UL"), expectedULong ) );
    BOOST_TEST( node.PutItem( _D("E"), expectedEnum ) );

    BOOST_TEST( node.GetItem<int>( _D("I") ) == expectedInt );
    BOOST_TEST( node.GetItem<long>( _D("L") ) == expectedLong );
    BOOST_TEST( node.GetItem<unsigned long>( _D("UL") ) == expectedULong );
    BOOST_TEST( static_cast<int>( node.GetItem<LegacyEnum>( _D("E") ) ) ==
                static_cast<int>( expectedEnum ) );
#else
    BOOST_TEST_MESSAGE( "This suite targets legacy bcc32c only; skipping content." );
    BOOST_TEST( true );
#endif
}

BOOST_AUTO_TEST_CASE( TypeMismatchReturnsDefault )
{
#if defined(__BORLANDC__) && !defined(_WIN64)
    Anafestica::TConfigNode node;

    BOOST_TEST( node.PutItem( _D("Value"), String( _D("abc") ) ) );
    BOOST_TEST( node.GetItem<int>( _D("Value") ) == int{} );
#else
    BOOST_TEST( true );
#endif
}

BOOST_AUTO_TEST_SUITE_END()
