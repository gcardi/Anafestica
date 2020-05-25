//---------------------------------------------------------------------------

#ifndef FormMainH
#define FormMainH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>

#include <anafestica/PersistFormVCL.h>
#include <anafestica/CfgRegistrySingleton.h>

//---------------------------------------------------------------------------

using TConfigRegistryForm =
    Anafestica::TPersistFormVCL<Anafestica::TConfigRegistrySingleton>;

//---------------------------------------------------------------------------

class TfrmMain : public TConfigRegistryForm
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
    __fastcall TfrmMain( TComponent* Owner );
    __fastcall TfrmMain( TComponent* Owner, StoreOpts StoreOptions,
                         Anafestica::TConfigNode* const RootNode = nullptr );
    __fastcall ~TfrmMain();
};
//---------------------------------------------------------------------------
extern PACKAGE TfrmMain *frmMain;
//---------------------------------------------------------------------------
#endif
