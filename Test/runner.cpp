#pragma hdrstop
#pragma argsused

#include <windows.h>

#ifdef _WIN32
#include <tchar.h>
#else
  typedef char _TCHAR;
  #define _tmain main
#endif

#include <vector>
#include <stdexcept>

using std::vector;

#include <System.SysUtils.hpp>

class ConsoleAttrKeeper {
public:
    ConsoleAttrKeeper() = default;
    ~ConsoleAttrKeeper() {
        SetConsoleTextAttribute( GetStdHandle ( STD_OUTPUT_HANDLE ), oldAttr_ );
    }
    ConsoleAttrKeeper( ConsoleAttrKeeper const & ) = delete;
    ConsoleAttrKeeper& operator=( ConsoleAttrKeeper const & ) = delete;
private:
    using AttrType = decltype( CONSOLE_SCREEN_BUFFER_INFO{}.wAttributes );

    AttrType oldAttr_ { GetColor() };

    static AttrType GetColor()  {
        CONSOLE_SCREEN_BUFFER_INFO info;
        if ( !GetConsoleScreenBufferInfo( GetStdHandle ( STD_OUTPUT_HANDLE ), &info ) ) {
            RaiseLastOSError();
        }
        return info.wAttributes;
    }
} Restore;

#if !defined( BOOST_TEST_MODULE )
# define BOOST_TEST_MODULE Test Anafestica
#endif

#include <boost/test/unit_test.hpp>

#if defined( _UNICODE )
# define BOOST_TEST_ALTERNATIVE_INIT_API
#endif

#if defined( _UNICODE )
int _tmain(int argc, _TCHAR* argv[])
{
    static constexpr char zero {};
    vector<LPSTR> UTF8argv;
    vector<char> TextBuffer;
    {
        vector<size_t> BufferPos;
        BufferPos.reserve( argc + 1 );
        for( decltype( argc ) idx {} ; idx < argc ; ++idx ) {
            auto const cvtLen =
                WideCharToMultiByte(
                    CP_UTF8, {}, argv[idx], -1, {}, {}, {}, {}
                );
            if ( !cvtLen ) {
                return GetLastError();
            }
            auto const StrtLn = TextBuffer.size();
            TextBuffer.resize( StrtLn + cvtLen );
            BufferPos.push_back( StrtLn );
            WideCharToMultiByte(
                CP_UTF8, {}, argv[idx], -1, &TextBuffer[StrtLn], cvtLen, {}, {}
            );
        }
        BufferPos.push_back( TextBuffer.size() );
        TextBuffer.resize( TextBuffer.size() + 1 );
        UTF8argv.reserve( argc + 1 );
        for( auto const p : BufferPos ) {
            UTF8argv.push_back( TextBuffer.data() + p );
        }
    }
    extern int main( int, char** );
    return main( argc, &UTF8argv[0] );
}
#endif

