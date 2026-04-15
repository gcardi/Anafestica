//---------------------------------------------------------------------------

#pragma hdrstop

#include "test_types.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)

#include <boost/test/unit_test.hpp>
#include <vector>
#include <memory>

#include <anafestica/CfgRegistry.h>
#include <System.Classes.hpp>
#include <System.SysUtils.hpp>
#include <System.Win.Registry.hpp>

namespace {

const String BaseTestRoot = L"Software\\Anafestica\\TestSuite";
static int TestKeyCounter = 0;

String MakeUniqueTestKey(String const & suffix)
{
    return BaseTestRoot + L"\\" + suffix + L"_" + IntToStr(++TestKeyCounter);
}

struct ScopedRegistryKey
{
    String FullPath;

    explicit ScopedRegistryKey(String const & path)
      : FullPath(path)
    {}

    ~ScopedRegistryKey() noexcept
    {
        try {
            auto reg = std::make_unique<Anafestica::Registry::TRegistry>();
            reg->RootKey = HKEY_CURRENT_USER;
            // Delete the subkey; ignore failures.
            reg->DeleteKey(FullPath);
        }
        catch (...) {
        }
    }
};

} // anonymous namespace

struct RegistryTestFixture
{
    RegistryTestFixture()
    {
        // Ensure base root exists for all tests.
        auto reg = std::make_unique<Anafestica::Registry::TRegistry>();
        reg->RootKey = HKEY_CURRENT_USER;
        if (!reg->OpenKey(BaseTestRoot, true)) {
            throw Exception(_D("Unable to create test root registry key"));
        }
        reg->CloseKey();
    }

    ~RegistryTestFixture()
    {
        try {
            auto reg = std::make_unique<Anafestica::Registry::TRegistry>();
            reg->RootKey = HKEY_CURRENT_USER;
            reg->DeleteKey(BaseTestRoot);
        }
        catch (...) {
        }
    }
};

BOOST_FIXTURE_TEST_SUITE(test_types, RegistryTestFixture)

BOOST_AUTO_TEST_CASE(TRegistry_QWORD_roundtrip)
{
    const String keyPath = MakeUniqueTestKey(L"QWORD");
    ScopedRegistryKey cleaner(keyPath);

    auto reg = std::make_unique<Anafestica::Registry::TRegistry>();
    reg->RootKey = HKEY_CURRENT_USER;
    BOOST_REQUIRE(reg->OpenKey(keyPath, true));

    const unsigned long long expected = UINT64_C(0x1122334455667788);
    reg->WriteQWORD(L"Value", expected);

    const auto actual = reg->ReadQWORD<unsigned long long>(L"Value");
    BOOST_TEST(actual == expected);

    auto [dataType, dataSize] = reg->GetExDataType(L"Value");
    BOOST_TEST(static_cast<int>(dataType) == static_cast<int>(Anafestica::Registry::TExRegDataType::Qword));
    BOOST_TEST(dataSize == sizeof(expected));

    reg->CloseKey();
}

BOOST_AUTO_TEST_CASE(TRegistry_QWORD_type_mismatch_throws)
{
    const String keyPath = MakeUniqueTestKey(L"QWORDBad");
    ScopedRegistryKey cleaner(keyPath);

    auto reg = std::make_unique<Anafestica::Registry::TRegistry>();
    reg->RootKey = HKEY_CURRENT_USER;
    BOOST_REQUIRE(reg->OpenKey(keyPath, true));

    reg->WriteString(L"Value", L"not-a-qword");
    reg->CloseKey();

    BOOST_REQUIRE(reg->OpenKey(keyPath, false));
    BOOST_CHECK_THROW(
        reg->ReadQWORD<unsigned long long>(L"Value"),
        ERegistryException
    );
    reg->CloseKey();
}

BOOST_AUTO_TEST_CASE(TRegistry_MultiSz_roundtrip)
{
    const String keyPath = MakeUniqueTestKey(L"MultiSz");
    ScopedRegistryKey cleaner(keyPath);

    auto reg = std::make_unique<Anafestica::Registry::TRegistry>();
    reg->RootKey = HKEY_CURRENT_USER;
    BOOST_REQUIRE(reg->OpenKey(keyPath, true));

    std::vector<String> expected { L"alpha", L"beta", L"gamma" };
    reg->WriteStrings(L"Value", expected);

    std::vector<String> result;
    const auto count = reg->ReadStringsTo(L"Value", std::back_inserter(result));
    BOOST_TEST(count == expected.size());
    BOOST_TEST(result == expected);

    reg->CloseKey();
}

BOOST_AUTO_TEST_CASE(TRegistry_BinaryData_roundtrip)
{
    const String keyPath = MakeUniqueTestKey(L"Binary");
    ScopedRegistryKey cleaner(keyPath);

    auto reg = std::make_unique<Anafestica::Registry::TRegistry>();
    reg->RootKey = HKEY_CURRENT_USER;
    BOOST_REQUIRE(reg->OpenKey(keyPath, true));

    TBytes expected;
    expected.Length = 5;
    for (int i = 0; i < expected.Length; ++i) {
        expected[i] = static_cast<Byte>(i * 3 + 1);
    }

    reg->WriteBinaryData(L"Value", expected);

    const TBytes actual = reg->ReadBinaryData(L"Value");
    BOOST_TEST(actual.Length == expected.Length);
    for (int i = 0; i < actual.Length; ++i) {
        BOOST_TEST(actual[i] == expected[i]);
    }

    std::vector<Byte> roundTripped;
    const auto count = reg->ReadBinaryDataTo(L"Value", std::back_inserter(roundTripped));
    BOOST_TEST(count == static_cast<size_t>(expected.Length));
    BOOST_TEST(roundTripped.size() == static_cast<size_t>(expected.Length));
    for (size_t i = 0; i < roundTripped.size(); ++i) {
        BOOST_TEST(roundTripped[i] == expected[i]);
    }

    reg->CloseKey();
}

BOOST_AUTO_TEST_CASE(TRegistry_BinaryData_type_mismatch_throws)
{
    const String keyPath = MakeUniqueTestKey(L"BinaryBad");
    ScopedRegistryKey cleaner(keyPath);

    auto reg = std::make_unique<Anafestica::Registry::TRegistry>();
    reg->RootKey = HKEY_CURRENT_USER;
    BOOST_REQUIRE(reg->OpenKey(keyPath, true));

    reg->WriteString(L"Value", L"not-binary");
    reg->CloseKey();

    BOOST_REQUIRE(reg->OpenKey(keyPath, false));
    std::vector<Byte> sink;
    BOOST_CHECK_THROW(
        reg->ReadBinaryDataTo(L"Value", std::back_inserter(sink)),
        ERegistryException
    );
    BOOST_CHECK_THROW(
        reg->ReadBinaryData(L"Value"),
        ERegistryException
    );
    reg->CloseKey();
}

BOOST_AUTO_TEST_CASE(TRegistry_ExpandString_roundtrip)
{
    const String keyPath = MakeUniqueTestKey(L"ExpandSz");
    ScopedRegistryKey cleaner(keyPath);

    auto reg = std::make_unique<Anafestica::Registry::TRegistry>();
    reg->RootKey = HKEY_CURRENT_USER;
    BOOST_REQUIRE(reg->OpenKey(keyPath, true));

    const String variable = L"%SystemRoot%";
    const String expectedSuffix = L"\\Temp";

    reg->WriteExpandString(L"Value", variable + expectedSuffix);
    const String expanded = reg->ReadExpandString(L"Value");

    const String systemRoot = GetEnvironmentVariable(L"SystemRoot");
    BOOST_TEST(expanded.Pos(systemRoot) == 1);
    BOOST_TEST(expanded.SubString(1, systemRoot.Length()) == systemRoot);
    BOOST_TEST(expanded.Pos(expectedSuffix) > 0);

    reg->CloseKey();
}

BOOST_AUTO_TEST_CASE(Variant_Usage_Info)
{
#ifdef ANAFESTICA_USE_STD_VARIANT
    BOOST_TEST_MESSAGE("Using std::variant");
#else
    BOOST_TEST_MESSAGE("Using boost::variant");
#endif
    BOOST_TEST(true); // Always pass
}

BOOST_AUTO_TEST_SUITE_END()


