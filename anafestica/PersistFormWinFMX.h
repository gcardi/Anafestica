//---------------------------------------------------------------------------

#ifndef PersistFormWinFMXH
#define PersistFormWinFMXH

#include <Classes.hpp>
#include <FMX.Forms.hpp>
#include <FMX.Platform.Win.hpp>

#include <anafestica/Cfg.h>

//---------------------------------------------------------------------------
namespace Anafestica {
//---------------------------------------------------------------------------

template<typename CfgSingleton>
class TPersistFormWinFMX : public Fmx::Forms::TForm
{
public:
    enum class StoreOpts {
        None, OnlySize, OnlyPos, PosAndSize,
        OnlyState, StateAndSize, StateAndPos, All
    };

    __fastcall TPersistFormWinFMX( System::Classes::TComponent* Owner,
                                   StoreOpts StoreOptions = StoreOpts::All,
                                   TConfigNode* const RootNode = nullptr );
    __fastcall TPersistFormWinFMX( System::Classes::TComponent* AOwner,
                                   NativeInt Dummy,
                                   StoreOpts StoreOptions = StoreOpts::All,
                                   TConfigNode* const RootNode = nullptr );
    void __fastcall BeforeDestruction();
    TConfigNode& GetConfigNode() const { return configNode_; }
    static TConfigNode& GetConfigRootNode();
    void ReadValues();
    void SaveValues();
protected:
private:
    static constexpr LPCTSTR IdLeft_{ _T( "Left" ) };
    static constexpr LPCTSTR IdTop_{ _T( "Top" ) };
    static constexpr LPCTSTR IdRight_{ _T( "Right" ) };
    static constexpr LPCTSTR IdBottom_{ _T( "Bottom" ) };
    static constexpr LPCTSTR IdState_{ _T( "State" ) };

    TConfigNode& configNode_;
    StoreOpts storeOptions_;

    TConfigNode& GetOrCreateConfigNode( TConfigNode * const RootNode );
    static bool HaveToSaveOrRestoreSize( StoreOpts Val ) noexcept;
    static bool HaveToSaveOrRestoreState( StoreOpts Val ) noexcept;
    static bool HaveToSaveOrRestorePos( StoreOpts Val ) noexcept;
};

//---------------------------------------------------------------------------

template<typename CfgSingleton>
__fastcall TPersistFormWinFMX<CfgSingleton>::TPersistFormWinFMX(
                                            System::Classes::TComponent* Owner,
                                            StoreOpts StoreOptions,
                                            TConfigNode* const RootNode )
    : Fmx::Forms::TForm( Owner )
    , storeOptions_( StoreOptions )
    , configNode_( GetOrCreateConfigNode( RootNode ) )
{
    try {
        ReadValues();
    }
    catch ( ... ) {
    }
}
//---------------------------------------------------------------------------

template<typename CfgSingleton>
__fastcall TPersistFormWinFMX<CfgSingleton>::TPersistFormWinFMX(
                                            System::Classes::TComponent* Owner,
                                            NativeInt Dummy,
                                            StoreOpts StoreOptions,
                                            TConfigNode* const RootNode )
    : Fmx::Forms::TForm( Owner, Dummy )
    , storeOptions_( StoreOptions )
    , configNode_( GetOrCreateConfigNode( RootNode ) )
{
    try {
        ReadValues();
    }
    catch ( ... ) {
    }
}
//---------------------------------------------------------------------------

template<typename CfgSingleton>
TConfigNode& TPersistFormWinFMX<CfgSingleton>::GetOrCreateConfigNode(
                                                 TConfigNode * const RootNode )
{
    return RootNode ?
        const_cast<TConfigNode&>( *RootNode )
      :
        GetConfigRootNode().GetSubNode( this->Name );
}
//---------------------------------------------------------------------------

template<typename CfgSingleton>
TConfigNode& TPersistFormWinFMX<CfgSingleton>::GetConfigRootNode()
{
     return CfgSingleton().GetConfig().GetRootNode();
}
//---------------------------------------------------------------------------

template<typename CfgSingleton>
void TPersistFormWinFMX<CfgSingleton>::ReadValues()
{
    if ( storeOptions_ != StoreOpts::None ) {
        Fmx::Platform::Win::TWinWindowHandle* hWnd =
            Fmx::Platform::Win::WindowHandleToPlatform( this->Handle );

        WINDOWPLACEMENT WndPl = {};
        WndPl.length = sizeof WndPl;

        if ( !GetWindowPlacement( hWnd->Wnd, &WndPl ) ) {
            RaiseLastOSError();
        }

        if ( HaveToSaveOrRestoreState( storeOptions_ ) ) {
            //RESTORE_VALUE( configNode_, IdState_, WndPl.showCmd );
            RestoreValue( configNode_, IdState_, WndPl.showCmd );
        }

        auto const WindowWidth =
            WndPl.rcNormalPosition.right - WndPl.rcNormalPosition.left;
        auto const WindowHeight =
            WndPl.rcNormalPosition.bottom - WndPl.rcNormalPosition.top;
        if ( HaveToSaveOrRestorePos( storeOptions_ ) ) {
            //RESTORE_VALUE( configNode_, IdLeft_, WndPl.rcNormalPosition.left );
            RestoreValue( configNode_, IdLeft_, WndPl.rcNormalPosition.left );
            //RESTORE_VALUE( configNode_, IdTop_, WndPl.rcNormalPosition.top );
            RestoreValue( configNode_, IdTop_, WndPl.rcNormalPosition.top );
            if ( HaveToSaveOrRestoreSize( storeOptions_ ) ) {
                RestoreValue( configNode_, IdRight_, WndPl.rcNormalPosition.right );
                RestoreValue( configNode_, IdBottom_, WndPl.rcNormalPosition.bottom );
            }
            else {
                WndPl.rcNormalPosition.right =
                    WndPl.rcNormalPosition.left + WindowWidth;
                WndPl.rcNormalPosition.bottom =
                    WndPl.rcNormalPosition.top + WindowHeight;
            }
        }
        else if ( HaveToSaveOrRestoreSize( storeOptions_ ) ) {
            WndPl.rcNormalPosition.right =
                WndPl.rcNormalPosition.left + WindowWidth;
            WndPl.rcNormalPosition.bottom =
                WndPl.rcNormalPosition.top + WindowHeight;
        }

        if ( !::SetWindowPlacement( hWnd->Wnd, &WndPl ) ) {
            RaiseLastOSError();
        }
    }
}
//---------------------------------------------------------------------------

template<typename CfgSingleton>
void __fastcall TPersistFormWinFMX<CfgSingleton>::BeforeDestruction()
{
    SaveValues();
    Fmx::Forms::TForm::BeforeDestruction();
}
//---------------------------------------------------------------------------

template<typename CfgSingleton>
void TPersistFormWinFMX<CfgSingleton>::SaveValues()
{
    if ( storeOptions_ != StoreOpts::None ) {
        Fmx::Platform::Win::TWinWindowHandle* hWnd =
            Fmx::Platform::Win::WindowHandleToPlatform( this->Handle );

        WINDOWPLACEMENT WndPl = {};
        WndPl.length = sizeof WndPl;

        if ( !GetWindowPlacement( hWnd->Wnd, &WndPl ) ) {
            RaiseLastOSError();
        }
        if ( HaveToSaveOrRestorePos( storeOptions_ ) ) {
            SaveValue( configNode_, IdLeft_, WndPl.rcNormalPosition.left );
            SaveValue( configNode_, IdTop_, WndPl.rcNormalPosition.top );
        }
        if ( HaveToSaveOrRestoreSize( storeOptions_ ) ) {
            SaveValue( configNode_, IdRight_, WndPl.rcNormalPosition.right );
            SaveValue( configNode_, IdBottom_, WndPl.rcNormalPosition.bottom );
        }
        if ( HaveToSaveOrRestoreState( storeOptions_ ) ) {
            SaveValue( configNode_, IdState_, WndPl.showCmd );
        }
    }
}
//---------------------------------------------------------------------------

template<typename CfgSingleton>
bool TPersistFormWinFMX<CfgSingleton>::HaveToSaveOrRestoreSize( StoreOpts Val ) noexcept
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
bool TPersistFormWinFMX<CfgSingleton>::HaveToSaveOrRestoreState( StoreOpts Val ) noexcept
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
bool TPersistFormWinFMX<CfgSingleton>::HaveToSaveOrRestorePos( StoreOpts Val ) noexcept
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
