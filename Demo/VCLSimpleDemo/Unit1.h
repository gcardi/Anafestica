//---------------------------------------------------------------------------

#ifndef Unit1H
#define Unit1H

//---------------------------------------------------------------------------

#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.StdCtrls.hpp>

#include <anafestica/PersistFormVCL.h>
#include <anafestica/CfgRegistrySingleton.h>

//---------------------------------------------------------------------------

using TConfigRegistryForm =
    Anafestica::TPersistFormVCL<Anafestica::TConfigRegistrySingleton>;

//---------------------------------------------------------------------------

class TForm1 : public TConfigRegistryForm
{
__published:    // IDE-managed Components
    TComboBox *comboboxFontName;
    TLabel *Label1;
    TLabel *lblClock;
    TTimer *Timer1;
    void __fastcall Timer1Timer(TObject *Sender);
    void __fastcall comboboxFontNameChange(TObject *Sender);
private:    // User declarations
    String selectedFontName_;

    static String GetModuleFileName();
    void SetupCaption();
    void RestoreProperties();
    void SaveProperties() const;
    void LoadFontListUIControl();
    void SelectCurrentFont();
    void SetSelectedFontName( String Val );

    __property String SelectedFontName = {
        read = selectedFontName_,
        write = SetSelectedFontName
    };
public:     // User declarations
    __fastcall TForm1( TComponent* Owner );
    __fastcall TForm1( TComponent* Owner, StoreOpts StoreOptions,
                       Anafestica::TConfigNode* const RootNode = nullptr );
    __fastcall ~TForm1();
};
//---------------------------------------------------------------------------
extern PACKAGE TForm1 *Form1;
//---------------------------------------------------------------------------
#endif
