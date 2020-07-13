//---------------------------------------------------------------------------

#pragma hdrstop

#include "test_types.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)

#include <boost/test/unit_test.hpp>

#include <anafestica/CfgRegistry.h>

struct RegistryTestFixture
{
    RegistryTestFixture()
    {
        // TODO: Common set-up each test case here.
        fclose( fopen(m_configFile.c_str(), "w+") );
    }

    ~RegistryTestFixture()
    {
        // TODO: Common tear-down for each test case here.
        remove(m_configFile.c_str());
    }


};

BOOST_AUTO_TEST_SUITE( test_types, RegistryTestFixture )

BOOST_AUTO_TEST_CASE( integer )
{
    int i = 1;
    BOOST_TEST(i);
    BOOST_TEST(i == 2);
}

BOOST_AUTO_TEST_SUITE_END()


