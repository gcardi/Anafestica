# Anafestica
An header-only library for the persistence of application settings in the Windows Registry or in other "media" (JSON, XML, etc.)

## Rationale

This library allows you to easily give persistence to your FMX and VCL applications with very few changes to the existing codebase. Saving the position, the size, and the state of the forms, along with other custom attributes, is very simple: just add a few lines of code.

The idea behind the library is to have a hierarchical polymorphic container that resembles the Windows registry, but with a more generalized interface. This container consists of nodes which, for example in the case of the Windows registry, containing a copy of keys and values related to a specific Windows registry key. These keys and values, if already existing, are loaded from the medium (in this case from a Windows Registry key) when the application starts and remains in memory for the whole application execution session. You can read or modify these keys and values, but everything remains confined to the application's memory. When the application ends, the container is usually saved at the correct position, in the appropriate storage medium, and with its default format. If, in the meantime, the application crashes, the data initially loaded from the storage medium remain unchanged.

The library is mainly made by two parts: a container part (which is generalized) and a serialization part. From the application point of view, it allows having a coherent interface and permits to store persistent data on different storage mediums or formats. For example, usually, persistent data of applications are stored in the Windows Registry, respecting some conventions regarding the nature of the application itself (nature intended as a normal application or, e.g., a service application or other application type). But, changing the serialization part, it's possible to use the filesystem and specify a particular data format (e.g JSON, or XML, or INI files, etc...). Maybe even store data on network services or hardware dongles. It depends on the serialization object, which is selectable as it is a template parameter (i.e it's passed as a Policy). You can also have several supported serialization formats in the same application. As a serialization format is ever associated with a specific container, there are no limits to the possible combinations and the number of usable containers. Each container links a specific serializer with the own serialization format.

In this refactored public version, the only reader/writer present is for the Windows Registry, but future versions will be able to store data in JSON and XML files. It also should be very easy for users to extend the serialization in different media or formats.

## Getting Started

The library itself is made only by header files and therefore it is easy to use and to include in your codebase and does not require additional compilation steps: you just have to include the necessary header files in your project. Can also be used in contexts other than GUI applications, but its real advantages are seen in the writing of the latter, where it certainly simplifies the management of the persistence of application attributes such as the position, size, and state of the forms, up to the settings of the whole application.

### Prerequisites

You need to install the boost libraries first because this library uses `boost::variant` instead of `std::variant` due to an unresolved bug in the standard C++Builder's library, which prevents direct assignment of values to `std::variant`. See [RSP-27418](https://quality.embarcadero.com/browse/RSP-27418) on Embarcadero Quality Portal.

So, to get boost libraries (e.g. 1.68.0), you can use GetIt.

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
To complete the installation, the last important thing to do is to add the references to this library to the include path(s) of the development system. Using the IDE menu _Tool -> Options_, add the $(BDSCOMMONDIR)\Anafestica path to both bcc32c and bcc64 settings:

<img src="https://i.ibb.co/RBQxLGt/EED5-A532-D4-E7-484-C-8619-D2-EBF126686-A-6.png" alt="BCC64">

<img src="https://i.ibb.co/JcgH89t/EED5-A532-D4-E7-484-C-8619-D2-EBF126686-A-7.png" alt="BCC32C">

## More

Try a [Quick Tour](QUICK_TOUR.md) in Anafestica.

