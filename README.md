# Anafestica
Header only library for persistence of application settings in the Windows Registry or in other "media" (JSON, XML, etc.)

## Getting Started

This library allows you to easily give persistence to your FMX and VCL applications with very few changes to the existing code base. Saving the position, size and status of the modules, along with other custom attributes, is very simple: just add a few lines of code.

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

**TO BE CONTINUED**
