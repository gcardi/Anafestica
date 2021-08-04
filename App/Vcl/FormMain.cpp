//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
#include <tchar.h>

#include <anafestica/FileVersionInfo.h>

#include "FormMain.h"

using Anafestica::TFileVersionInfo;

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TfrmMain *frmMain;
//---------------------------------------------------------------------------

__fastcall TfrmMain::TfrmMain(TComponent* Owner)
    : TfrmMain( Owner, StoreOpts::All, nullptr )
{
}
//---------------------------------------------------------------------------

__fastcall TfrmMain::TfrmMain( TComponent* Owner, StoreOpts StoreOptions,
                               Anafestica::TConfigNode* const RootNode )
    : TConfigRegistryForm( Owner, StoreOptions, RootNode )
{
    Init();
}
//---------------------------------------------------------------------------

__fastcall TfrmMain::~TfrmMain()
{
    try {
        Destroy();
    }
    catch ( ... ) {
    }
}
//---------------------------------------------------------------------------

void TfrmMain::Init()
{
    SetupCaption();
    RestoreProperties();
}
//---------------------------------------------------------------------------

void TfrmMain::Destroy()
{
    SaveProperties();
}
//---------------------------------------------------------------------------

String TfrmMain::GetModuleFileName()
{
    return GetModuleName( reinterpret_cast<unsigned>( HInstance ) );
}
//---------------------------------------------------------------------------

void TfrmMain::SetupCaption()
{
    TFileVersionInfo const Info( GetModuleFileName() );
    Caption =
        Format(
            _T( "%s, Ver %s" ),
            ARRAYOFCONST( (
                Application->Title,
                Info.ProductVersion
            ) )
        );
}
//---------------------------------------------------------------------------

void TfrmMain::RestoreProperties()
{

}
//---------------------------------------------------------------------------

void TfrmMain::SaveProperties() const
{

}
//---------------------------------------------------------------------------
