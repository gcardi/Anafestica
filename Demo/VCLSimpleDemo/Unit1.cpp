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
    : TConfigForm( Owner, StoreOptions, RootNode )
{
    // startup code
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
    //auto x = GetConfigNode().GetItem<String>( _D( "SelectedFontName" ) );
    //ShowMessage( x );
}
//---------------------------------------------------------------------------

void TForm1::SaveProperties() const
{
    SAVE_LOCAL_PROPERTY( SelectedFontName );
    //GetConfigNode()[_D( "Sub" )].PutItem( _D( "Item" ), 100 );
    GetConfigNode()[_D( "Sub" )][_D( "Sotto" )].PutItem( _D( "Item" ), 100 );

    using StringCont = std::vector<String>;
    using BytesCont = std::vector<Byte>;

    unsigned int             u   = 10;
    long                     l   = 11L;
    unsigned long            ul  = 12UL;
    char                     c   = '@';
    unsigned char            uc  = 0x34;
    short                    s   = -32000;
    unsigned short           us  = 65000;
    long long                ll  = -45LL;
    unsigned long long       ull = 0x1000000000000;
    bool                     b   = true;
    System::TDateTime        dt  = Now();
    float                    flt = 61.34F;
    double                   dbl = M_PI;
    System::Currency         cur = 1200.45;
    StringCont               sv  { { _D( "Uno" ), _D( "Due" ), _D( "Tre" ), } };
    System::Sysutils::TBytes dab;
    dab.Length = 3;
    dab[0] = 0x45;
    dab[1] = 0x56;
    dab[2] = 0x67;
    BytesCont                vb  = { 0xFE, 0xCA, 0x39, 0x43 };




    SAVE_LOCAL_PROPERTY( u );
    SAVE_LOCAL_PROPERTY( l );
    SAVE_LOCAL_PROPERTY( ul );
    SAVE_LOCAL_PROPERTY( c );
    SAVE_LOCAL_PROPERTY( uc );
    SAVE_LOCAL_PROPERTY( s );
    SAVE_LOCAL_PROPERTY( us );
    SAVE_LOCAL_PROPERTY( ll );
    SAVE_LOCAL_PROPERTY( ull );
    SAVE_LOCAL_PROPERTY( b );
    SAVE_LOCAL_PROPERTY( dt );
    SAVE_LOCAL_PROPERTY( flt );
    SAVE_LOCAL_PROPERTY( dbl );
    SAVE_LOCAL_PROPERTY( cur );
    SAVE_LOCAL_PROPERTY( sv );
    SAVE_LOCAL_PROPERTY( dab );
    SAVE_LOCAL_PROPERTY( vb );





}
//---------------------------------------------------------------------------

