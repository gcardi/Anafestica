//---------------------------------------------------------------------------

#ifndef FormMainH
#define FormMainH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <FMX.Controls.hpp>
#include <FMX.Forms.hpp>
//---------------------------------------------------------------------------

#include <anafestica/PersistFormWinFMX.h>
#include <anafestica/CfgRegistrySingleton.h>

//---------------------------------------------------------------------------

typedef Anafestica::TPersistFormWinFMX<Anafestica::TConfigRegistrySingleton>
        TConfigRegistryFormWinFMX;

class TfrmMain : public TConfigRegistryFormWinFMX
{
__published:	// IDE-managed Components
private:	// User declarations
    void Init();
    void Destroy();
    static String GetModuleFileName();
    void SetupCaption();
    void RestoreProperties();
    void SaveProperties() const;
public:		// User declarations
    __fastcall TfrmMain(TComponent* Owner);
    __fastcall TfrmMain( TComponent* Owner, StoreOpts StoreOptions,
                         Anafestica::TConfigNode* const RootNode = 0 );
    __fastcall ~TfrmMain();
};
//---------------------------------------------------------------------------
extern PACKAGE TfrmMain *frmMain;
//---------------------------------------------------------------------------
#endif

