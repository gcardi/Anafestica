//---------------------------------------------------------------------------

#include <fmx.h>
#ifdef _WIN32
#include <tchar.h>
#endif
#pragma hdrstop

#include <tchar.h>

#include <System.StartUpCopy.hpp>

//---------------------------------------------------------------------------
USEFORM("FormMain.cpp", frmMain);
//---------------------------------------------------------------------------
extern "C" int FMXmain()
{
    try {
        Application->Initialize();
        Application->CreateForm(__classid(TfrmMain), &frmMain);
        Application->Run();
        while ( auto const Cnt = Screen->FormCount ) {
            delete Screen->Forms[Cnt - 1];
        }
    }
    catch ( Exception &exception ) {
         Application->ShowException( &exception );
    }
    catch ( ... ) {
         try {
             throw Exception( _T( "Unknown exception" ) );
         }
         catch ( Exception &exception ) {
             Application->ShowException(&exception);
         }
    }
    return 0;
}
//---------------------------------------------------------------------------
