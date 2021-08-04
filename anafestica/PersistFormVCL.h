//---------------------------------------------------------------------------

#ifndef PersistFormVCLH
#define PersistFormVCLH

#include <System.Classes.hpp>
#include <Vcl.Forms.hpp>

#include <anafestica/Cfg.h>

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------

template<typename CfgSingleton>
class TPersistFormVCL : public Vcl::Forms::TForm
{
public:
    using inherited = Vcl::Forms::TForm;

    enum class StoreOpts {
        None, OnlySize, OnlyPos, PosAndSize,
        OnlyState, StateAndSize, StateAndPos, All
    };

    __fastcall TPersistFormVCL( System::Classes::TComponent* Owner,
                                StoreOpts StoreOptions = StoreOpts::All,
                                TConfigNode* const RootNode = nullptr );
    __fastcall TPersistFormVCL( System::Classes::TComponent* AOwner, int Dummy,
                                StoreOpts StoreOptions = StoreOpts::All,
                                TConfigNode* const RootNode = nullptr );
    __fastcall TPersistFormVCL( HWND ParentWindow,
                                StoreOpts StoreOptions = StoreOpts::All,
                                TConfigNode* const RootNode = nullptr );
    virtual void __fastcall BeforeDestruction() override;
    TConfigNode& GetConfigNode() const { return configNode_; }
    static TConfigNode& GetConfigRootNode();
    void ReadValues();
    void SaveValues();
protected:
    virtual void DoRestoreState();
    DYNAMIC void __fastcall DoShow() {
        Vcl::Forms::TForm::DoShow();
        try {
            DoRestoreState();
        }
        catch (...) {
        }
    }
private:
    static constexpr LPCTSTR IdLeft_{ _D( "Left" ) };
    static constexpr LPCTSTR IdTop_{ _D( "Top" ) };
    static constexpr LPCTSTR IdRight_{ _D( "Right" ) };
    static constexpr LPCTSTR IdBottom_{ _D( "Bottom" ) };
    static constexpr LPCTSTR IdState_{ _D( "State" ) };

    TConfigNode& configNode_;
    StoreOpts storeOptions_;

    TConfigNode& GetOrCreateConfigNode( TConfigNode * const RootNode );
    static bool HaveToSaveOrRestoreSize( StoreOpts Val ) noexcept;
    static bool HaveToSaveOrRestoreState( StoreOpts Val ) noexcept;
    static bool HaveToSaveOrRestorePos( StoreOpts Val ) noexcept;
};
//---------------------------------------------------------------------------

template<typename CfgSingleton>
__fastcall TPersistFormVCL<CfgSingleton>::TPersistFormVCL(
                                            System::Classes::TComponent* Owner,
                                            StoreOpts StoreOptions,
                                            TConfigNode* const RootNode )
  : inherited( Owner )
  , storeOptions_( StoreOptions )
  , configNode_( GetOrCreateConfigNode( RootNode ) )
{
}
//---------------------------------------------------------------------------

template<typename CfgSingleton>
__fastcall TPersistFormVCL<CfgSingleton>::TPersistFormVCL(
                                          System::Classes::TComponent* AOwner,
                                          int Dummy, StoreOpts StoreOptions,
                                          TConfigNode* const RootNode )
  : inherited( AOwner, Dummy )
  , storeOptions_( StoreOptions )
  , configNode_( GetOrCreateConfigNode( RootNode ) )
{
}
//---------------------------------------------------------------------------

template<typename CfgSingleton>
__fastcall TPersistFormVCL<CfgSingleton>::TPersistFormVCL( HWND ParentWindow,
                                                  StoreOpts StoreOptions,
                                                  TConfigNode* const RootNode )
  : inherited( ParentWindow )
  , storeOptions_( StoreOptions )
  , configNode_( GetOrCreateConfigNode( RootNode ) )
{
}
//---------------------------------------------------------------------------

template<typename CfgSingleton>
void TPersistFormVCL<CfgSingleton>::DoRestoreState()
{
    ReadValues();
}
//---------------------------------------------------------------------------

template<typename CfgSingleton>
TConfigNode& TPersistFormVCL<CfgSingleton>::GetOrCreateConfigNode(
                                                 TConfigNode * const RootNode )
{
    return RootNode ?
        const_cast<TConfigNode&>( *RootNode )
      :
        GetConfigRootNode().GetSubNode( Name );
}
//---------------------------------------------------------------------------

template<typename CfgSingleton>
TConfigNode& TPersistFormVCL<CfgSingleton>::GetConfigRootNode()
{
     return CfgSingleton().GetConfig().GetRootNode();
}
//---------------------------------------------------------------------------

template<typename CfgSingleton>
void TPersistFormVCL<CfgSingleton>::ReadValues()
{
    if ( storeOptions_ != StoreOpts::None ) {
        if ( HaveToSaveOrRestorePos( storeOptions_ ) ) {
            int pLeft { BoundsRect.Left };
            RestoreValue( configNode_, IdLeft_, pLeft );
            int pTop { BoundsRect.Top };
            RestoreValue( configNode_, IdTop_, pTop );
            if ( HaveToSaveOrRestoreSize( storeOptions_ ) ) {
                int pRight { BoundsRect.Right };
                RestoreValue( configNode_, IdRight_, pRight );
                int pBottom {BoundsRect.Bottom };
                RestoreValue( configNode_, IdBottom_, pBottom );
                BoundsRect = TRect( pLeft, pTop, pRight, pBottom );
            }
            else {
                Left = pLeft;
                Top = pTop;
            }
        }
        else if ( HaveToSaveOrRestoreSize( storeOptions_ ) ) {
            int pLeft { BoundsRect.Left };
            RestoreValue( configNode_, IdLeft_, pLeft );
            int pRight { BoundsRect.Right };
            RestoreValue( configNode_, IdRight_, pRight );
            int pTop { BoundsRect.Top };
            RestoreValue( configNode_, IdTop_, pTop );
            int pBottom { BoundsRect.Bottom };
            RestoreValue( configNode_, IdBottom_, pBottom );
            Width = pRight - pLeft;
            Height = pBottom - pTop;
        }

        if ( HaveToSaveOrRestoreState( storeOptions_ ) ) {
            RestoreValue( configNode_, IdState_, WindowState );
        }
    }
}

//---------------------------------------------------------------------------

template<typename CfgSingleton>
void __fastcall TPersistFormVCL<CfgSingleton>::BeforeDestruction()
{
    SaveValues();
    inherited::BeforeDestruction();
}
//---------------------------------------------------------------------------

template<typename CfgSingleton>
void TPersistFormVCL<CfgSingleton>::SaveValues()
{
    if ( storeOptions_ != StoreOpts::None ) {
        WINDOWPLACEMENT WndPl = {};
        WndPl.length = sizeof WndPl;

        if ( !GetWindowPlacement( Handle, &WndPl ) ) {
            RaiseLastOSError();
        }
        if ( HaveToSaveOrRestorePos( storeOptions_ ) ) {
            SaveValue(
                configNode_, IdLeft_,
                static_cast<int>( WndPl.rcNormalPosition.left )
            );
            SaveValue(
                configNode_, IdTop_,
                static_cast<int>( WndPl.rcNormalPosition.top )
            );
        }
        if ( HaveToSaveOrRestoreSize( storeOptions_ ) ) {
            SaveValue(
                configNode_, IdRight_,
                static_cast<int>( WndPl.rcNormalPosition.right )
            );
            SaveValue(
                configNode_, IdBottom_,
                static_cast<int>( WndPl.rcNormalPosition.bottom )
            );
        }
        if ( HaveToSaveOrRestoreState( storeOptions_ ) ) {
            SaveValue(
                configNode_, IdState_, WindowState
            );
        }
    }
}
//---------------------------------------------------------------------------

template<typename CfgSingleton>
bool TPersistFormVCL<CfgSingleton>::HaveToSaveOrRestoreSize( StoreOpts Val ) noexcept
{
    switch ( Val ) {
        case StoreOpts::All:
        case StoreOpts::OnlySize:
        case StoreOpts::PosAndSize:
        case StoreOpts::StateAndSize:
            return true;
        default:
            return false;
    }
}
//---------------------------------------------------------------------------

template<typename CfgSingleton>
bool TPersistFormVCL<CfgSingleton>::HaveToSaveOrRestoreState( StoreOpts Val ) noexcept
{
    switch ( Val ) {
        case StoreOpts::OnlyState:
        case StoreOpts::StateAndSize:
        case StoreOpts::StateAndPos:
        case StoreOpts::All:
            return true;
        default:
            return false;
    }
}
//---------------------------------------------------------------------------

template<typename CfgSingleton>
bool TPersistFormVCL<CfgSingleton>::HaveToSaveOrRestorePos( StoreOpts Val ) noexcept
{
    switch ( Val ) {
        case StoreOpts::OnlyPos:
        case StoreOpts::PosAndSize:
        case StoreOpts::StateAndPos:
        case StoreOpts::All:
            return true;
        default:
            return false;
    }
}
//---------------------------------------------------------------------------

#define RESTORE_LOCAL_PROPERTY( PROPERTY ) \
{\
    std::remove_reference_t< decltype( PROPERTY )> Tmp{ PROPERTY }; \
    GetConfigNode().GetItem( #PROPERTY, Tmp ); \
    PROPERTY = Tmp; \
}

#define SAVE_LOCAL_PROPERTY( PROPERTY ) \
    GetConfigNode().PutItem( #PROPERTY, PROPERTY )

//---------------------------------------------------------------------------
}; // End of namespace Anafestica
//---------------------------------------------------------------------------

#endif
