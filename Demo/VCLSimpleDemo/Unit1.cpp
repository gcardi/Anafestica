//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include <anafestica/FileVersionInfo.h>

#include "Unit1.h"

using Anafestica::TFileVersionInfo;

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm1 *Form1;
//---------------------------------------------------------------------------

__fastcall TForm1::TForm1(TComponent* Owner)
	: TForm1( Owner, StoreOpts::All, nullptr )
{
}
//---------------------------------------------------------------------------

__fastcall TForm1::TForm1( TComponent* Owner, StoreOpts StoreOptions,
						   Anafestica::TConfigNode* const RootNode )
	: TConfigRegistryForm( Owner, StoreOptions, RootNode )
{
	selectedFontName_ = Label1->Font->Name;
	SetupCaption();
	LoadFontListUIControl();
	RestoreProperties();
	SelectCurrentFont();
}
//---------------------------------------------------------------------------

__fastcall TForm1::~TForm1()
{
    try {
        SaveProperties();
    }
    catch ( ... ) {
    }
}
//---------------------------------------------------------------------------

String TForm1::GetModuleFileName()
{
    return GetModuleName( reinterpret_cast<unsigned>( HInstance ) );
}
//---------------------------------------------------------------------------

void TForm1::SetupCaption()
{
	Caption =
		Format(
			_T( "%s, Ver %s" ),
			ARRAYOFCONST( (
				Application->Title,
				TFileVersionInfo{ GetModuleFileName() }.ProductVersion
            ) )
        );
}
//---------------------------------------------------------------------------

void TForm1::LoadFontListUIControl()
{
	comboboxFontName->Items->Assign( Screen->Fonts );
}
//---------------------------------------------------------------------------

void TForm1::SelectCurrentFont()
{
    comboboxFontName->ItemIndex =
        comboboxFontName->Items->IndexOf( SelectedFontName );
}
//---------------------------------------------------------------------------

void TForm1::SetSelectedFontName( String Val )
{
    if ( selectedFontName_ != Val ) {
        selectedFontName_ = Val;
        lblClock->Font->Name = Val;
    }
}
//---------------------------------------------------------------------------

void __fastcall TForm1::comboboxFontNameChange(TObject *Sender)
{
    SelectedFontName = comboboxFontName->Text;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Timer1Timer(TObject *Sender)
{
    lblClock->Caption = Now().FormatString( _T( "hh.nn.ss") );
}
//---------------------------------------------------------------------------

void TForm1::RestoreProperties()
{
    RESTORE_LOCAL_PROPERTY( SelectedFontName );
}
//---------------------------------------------------------------------------

void TForm1::SaveProperties() const
{
    SAVE_LOCAL_PROPERTY( SelectedFontName );
}
//---------------------------------------------------------------------------

