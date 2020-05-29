# Anafestica
Header only library for persistence of application settings in the Windows Registry or in other "media" (JSON, XML, etc.)

## Getting Started

This library allows you to easily give persistence to your FMX and VCL applications with very few changes to the existing code base. Saving the position, size and state of the forms, along with other custom attributes, is very simple: just add a few lines of code.

The library itself is made only by header files and therefore it is easy to use and to include in your codebase, and does not require additional compilation steps: you just have to include the necessary header files in your project. Can also be used in contexts other than GUI applications, but its real advantages are seen in the writing of the latters, where it certainly simplifies the management of the persistence of application attributes such as the position, size and state of the forms, up to the settings of the whole application.

In this refactored public version, the only reader/writer present is for the Windows Registry, but future versions will be able to store data in JSON and XML files. Also should be very easy for users to extend the serialization on different media or formats.

Some pictures may be worth a thousand words... just add some declarations and a contructor to make a Form save itself in the Windows Registry.

<img src="https://i.ibb.co/4RMBg1Y/1.png" alt="Sample header file">

<img src="https://i.ibb.co/TBNYKRk/EED5-A532-D4-E7-484-C-8619-D2-EBF126686-A-2.png" alt="Sample implementation">

But the whole story is a little longer. You may wonder where have been stored in the Registry the Form's attributes. Specifically, which place in the Registry? In the most obvious one, clearly: i.e. in the 

> _HKCU/Software/Vendor/Product/Version_ 

key. 

But where the hell does the Vendor, Product, and Version parts get the library from? It's simple: from the project's version info keys. Namely:

<img src="https://i.ibb.co/x3ZK9gZ/EED5-A532-D4-E7-484-C-8619-D2-EBF126686-A-3.png" alt="Project Info Keys sample settings">

So, the position, size for the Form1 is stored in:

<img src="https://i.ibb.co/ws7wRyp/EED5-A532-D4-E7-484-C-8619-D2-EBF126686-A-4.png" alt="Sample registry node layout">

Note that the state of the form has not been saved. Yes, the library only saves values other than the default.

Since the library uses one or more singletons for serialization, the other few lines of code (to be added only once), are used to ensure that the application forms are destroyed before the aforesaid singletons. To ensure the correct behaviour, it's necessary to modify the project source file in this fashion:

<img src="https://i.ibb.co/gt7MDYs/EED5-A532-D4-E7-484-C-8619-D2-EBF126686-A-5.png" alt="Addings to project source">

In this GitHub repository there are two applications (VCL and FMX) which constitute two basic examples on how to structure an application (or modify an existing one) so that it offers persistence. These applications, as well as the forms that make them up, can also be imported into the Object Repository in order to be readily available for the creation of other new forms or new applications.

Later we will see how to easily manage custom attributes through properties. It is not strictly necessary to use properties, but using them certainly makes the code more readable. Surely you can have granular control, if you want, on the persistence process by calling the library object methods directly without going through macros.

### Prerequisites

You need to install the boost libraries first, because this library uses `boost::variant` instead of `std::variant` due to a unresolved bug in the standard C++Builder's library, which prevents direct assignment of values to `std::variant`. See [RSP-27418](https://quality.embarcadero.com/browse/RSP-27418) on Embarcadero Quality Portal.

So, to get boost libraries (e.g. 1.68.0) you can use GetIt.

<img src="https://i.ibb.co/FmPznnX/3-611-D020-B-2-C12-4839-8567-A7-E8-A650940-E.png" alt="Figure 3">

Note that only _clang-based_ compilers are supported by this library, so you need to install boost libraries for **bcc32c** and **bcc64**.

### Installing

Clone the repository to $(BDSCOMMONDIR) which, normally, is %public%\Documents\Embarcadero\Studio\XX.X, where XX.X corresponds to the version of RAD Studio you want to refer. For example, for RAD Studio 10.3.3, $(BDSCOMMONDIR) corresponds to %public%\Documents\Embarcadero\Studio\XX.X which, in turn, is usually C:\Users\Public\Documents\Embarcadero\Studio\20.0.

```
C:\Users\Public\Documents\Embarcadero\Studio\21.0>git clone https://github.com/gcardi/Anafestica.git
Cloning into 'Anafestica'...
remote: Enumerating objects: 36, done.
remote: Counting objects: 100% (36/36), done.
remote: Compressing objects: 100% (27/27), done.
remote: Total 36 (delta 9), reused 32 (delta 9), pack-reused 0
Unpacking objects: 100% (36/36), done.

C:\Users\Public\Documents\Embarcadero\Studio\21.0>dir
```
In order to complete the installation, the last important thing to do is to add the references to this library to the include path(s) of the development system. Using the IDE menu _Tool -> Options_, add the $(BDSCOMMONDIR)\Anafestica path to both bcc32c and bcc64 settings:

<img src="https://i.ibb.co/RBQxLGt/EED5-A532-D4-E7-484-C-8619-D2-EBF126686-A-6.png" alt="BCC64">

<img src="https://i.ibb.co/JcgH89t/EED5-A532-D4-E7-484-C-8619-D2-EBF126686-A-7.png" alt="BCC32C">

### Using the library

Before starting, it's better to state that the following operations can be skipped by loading one of reference applications in Anafestica/App (then saving them as a copy in a different place) or by saving a prototype of a "typical" application in the object repository for subsequent use, so as not to have to repeatedly perform the steps that we are going to describe for each new project. So don't be frightened if the steps seem long and tortuous: you will only have to do them once. Or never do them, if you load a reference project and save it somewhere else (in this case I recommend changing the GUID of the project, by hand, inside the cbproj file, to make it universally unique).

In this repository, in the Anafestica/Demo/VCLSimpleDemo/ path, there is a Demo app that shows how to make a custom text attribute persistent (as well as the position, size and state of the Form). It is one of the simplest scenarios for the management of persistent attributes: it is therefore assumed that the data refer to the Form itself, so it will be stored with the other typical attributes of a Form, that is position, size and state.

The demo application is a clock gadget where you can change the font typeface. The application must be able to allow the choice of the font and remember it between different execution sessions.

<img src="https://i.ibb.co/t4hqQFz/EED5-A532-D4-E7-484-C-8619-D2-EBF126686-A-8.png" alt="Demo app screenshot">

The structure of this application is very simple. Now let's see how to build it from scratch.

Let's create a new VCL application for C ++ Builder: 

<img src="https://i.ibb.co/KrL66P5/EED5-A532-D4-E7-484-C-8619-D2-EBF126686-A-9.png" alt="C++ VCL app creation">

Next, let's add the 64 bit platform for Windows:

<img src="https://i.ibb.co/59VN2Rq/EED5-A532-D4-E7-484-C-8619-D2-EBF126686-A-10.png" alt="Add the 64 bit platform for Windows">

Now let's make some essential "adjustments" to the project. From the Project->Options menu (Shift + Ctrl + F11):

Turn off classic C++ compiler (it's better for all platforms):

<img src="https://i.ibb.co/7NGKxzm/EED5-A532-D4-E7-484-C-8619-D2-EBF126686-A-12.png" alt="Turn off classic C++ compiler">

Set appropriate values ​​for CompanyName, ProductName and ProductVersion in the version info keys for all platforms. Note: if you skip this step, when you start the application it will give you a "resource not found error". The application needs these values ​​because it uses them to create the HKCU \ CompanyName \ ProductName \ ProductVersion path in the Registry.

<img src="https://i.ibb.co/qdDQQP3/EED5-A532-D4-E7-484-C-8619-D2-EBF126686-A-13.png" alt="Set version info keys">

Let's save the project.

<img src="https://i.ibb.co/v4CDpkG/EED5-A532-D4-E7-484-C-8619-D2-EBF126686-A-11.png" alt="Save demo project">

Now **close** the project.

Close the project? Why the hell do you need to close the project?

Because the template project contained in the IDE and used to start this application, as it is, doesn't propagate the settings you made so far on all the platforms and their associated configurations. So now, with a simple text editor, we can go to remove the problematic values inside the project file (.cbproj). 

Let's open the main project configuration file (but it's better to make a backup copy of this file first):

<img src="https://i.ibb.co/PGrNRth/EED5-A532-D4-E7-484-C-8619-D2-EBF126686-A-14.png" alt="Fix project file from broken template"></a>

Now, let's edit the file, e.g with Notepad: hence, let's remove all <VerInfo_Keys> tags from all nodes except the first one, which is usually <PropertyGroup Condition = "'$ (Base)'! = ''">:
  
<img src="https://i.ibb.co/LJDXWnm/EED5-A532-D4-E7-484-C-8619-D2-EBF126686-A-15.png" alt="Clear extra Ver Info Keys">

Let's save the modified project file the reopens it in the IDE. If there's problems please take the backup copy and try again.

Now, let's reduce the size of the main form a bit... for example with the Width property set to 340 and the Height property to 200. Next, copy in to the clipboard this snippet, then paste it to the main form:

```dfm
object lblClock: TLabel
  Left = 0
  Top = 56
  Width = 324
  Height = 105
  Align = alBottom
  Alignment = taCenter
  Anchors = [akLeft, akTop, akRight, akBottom]
  AutoSize = False
  Caption = '--:--:--'
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -53
  Font.Name = 'Tahoma'
  Font.Style = []
  ParentFont = False
  Layout = tlCenter
  ExplicitWidth = 331
  ExplicitHeight = 184
end
```

Now the main form should have this look:

<img src="https://i.ibb.co/JjDNWWQ/EED5-A532-D4-E7-484-C-8619-D2-EBF126686-A-16.png" alt="Main Form with label control">

Add this snippet to the form:

```dfm
object comboboxFontName: TComboBox
  Left = 24
  Top = 24
  Width = 145
  Height = 21
  Style = csDropDownList
  TabOrder = 0
end
object Label1: TLabel
  Left = 24
  Top = 8
  Width = 52
  Height = 13
  Caption = 'Font Name'
end
object Timer1: TTimer
  Left = 224
  Top = 24
end
```
Now the form should be like this:

<img src="https://i.ibb.co/zn0Hz91/EED5-A532-D4-E7-484-C-8619-D2-EBF126686-A-18.png" alt="Complete GUI">

At this point, we can modify the Unit1.h file (the header file of the main form) as the following (you can copy and paste these lines directly in the code editor for the Unit1.h file):

```cpp
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
```

We can see several additional lines compared to the original file. 

There are two additional includes and one type alias. The type alias is used as base class of TForm1 in place of the more classic TForm:

```cpp
...
#include <anafestica/PersistFormVCL.h>
#include <anafestica/CfgRegistrySingleton.h>

//---------------------------------------------------------------------------

using TConfigRegistryForm =
	Anafestica::TPersistFormVCL<Anafestica::TConfigRegistrySingleton>;

//---------------------------------------------------------------------------

class TForm1 : public TConfigRegistryForm
...
```

The type alias is necessary to make happy the IDE's Form designer that doesn't like the syntax of C++ templates. Just because the TForm1 class derives from this type of alias, it allows it to acquire intrinsic ability to save its attributes, such as position, size and state and, optionally, the specific attributes of the application that the programmer will want to save.

Next, there is a new constructor that takes several parameters and also a destructor:

```cpp
    ...
    __fastcall TForm1( TComponent* Owner, StoreOpts StoreOptions,
                       Anafestica::TConfigNode* const RootNode = nullptr );
    __fastcall ~TForm1();
    ...
```

The constructor will takes care to read the user defined attributes from the persistent storage; conversely the destructor save them.

Now we will see some lines of code containing data, function signatures and a property.

```cpp
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
```

Proceding step by step in the explanation, it's possible to note a non-static member variable named selectedFontName_ which name is self explanatory. This variable is directly connected to the getter of the property SelectedFontName. The setter of the SelectedFontName property is linked to the TForm1's non-static and non-const member function, called SetSelectedFontName. The presence of this property is important (despite it is a private member), since it allows you to store and retrieve very easily the attribute it represents, that is, with only two lines of code: one used to retrieve the value and the other to save the latter.

The other methods are only the result of a simple functional decomposition aimed at simplifying the reading of the code (and, obviously, to make the toxicity-metric utilities happy).

Now all that remains is to look at the implementation of the methods, and provide the event handlers for the combobox that contains the list of fonts and for the timer that updates the lblClock caption. In reality, there is another important thing (mentioned previously) to do: provide for the destruction of the forms before the application returns from the WinMain function. But we'll see it later.

Let's implement the two (empty) event handlers by double clicking on Timer1 and comboboxFontName:

<img src="https://i.ibb.co/hFktX8x/EED5-A532-D4-E7-484-C-8619-D2-EBF126686-A-19.png" alt="Generating event handlers">

Now copy and paste in the Unit1.cpp file the following code:

```cpp
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
```

Now let's dissect the newly pasted code. 

The 




It use a TComboBox which will be filled with the list of fonts present on the system each time the application is started. 

```cpp
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

void TForm1::LoadFontListUIControl()
{
    comboboxFontName->Items->Assign( Screen->Fonts );
}
//---------------------------------------------------------------------------
```

```cpp
//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
#include <tchar.h>
//---------------------------------------------------------------------------
USEFORM("Unit1.cpp", Form1);
//---------------------------------------------------------------------------
int WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
{
    try
    {
        Application->Initialize();
        Application->MainFormOnTaskBar = true;
        Application->CreateForm(__classid(TForm1), &Form1);
        Application->Run();
        while ( auto const Cnt = Screen->FormCount ) {
            delete Screen->Forms[Cnt - 1];
        }
    }
    catch (Exception &exception)
    {
        Application->ShowException(&exception);
    }
    catch (...)
    {
        try
        {
            throw Exception("");
        }
        catch (Exception &exception)
        {
            Application->ShowException(&exception);
        }
    }
    return 0;
}
//---------------------------------------------------------------------------
```

**TO BE CONTINUED**
