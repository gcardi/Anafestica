//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include <tchar.h>

//---------------------------------------------------------------------------
USEFORM("FormMain.cpp", frmMain);
//---------------------------------------------------------------------------

int WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
{
    try {
         Application->Initialize();
         Application->MainFormOnTaskBar = true;
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
             Application->ShowException( &exception );
         }
    }
    return 0;
}
//---------------------------------------------------------------------------
