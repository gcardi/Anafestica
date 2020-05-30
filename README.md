# Anafestica
An header-only library for the persistence of application settings in the Windows Registry or in other "media" (JSON, XML, etc.)

## Rationale

This library allows to easily give persistence to FMX and VCL applications with very few changes to the existing codebase. Saving the position, the size, and the state of the forms, along with other custom attributes, is very simple: just add a few lines of code.

The idea behind the library is to have a hierarchical polymorphic container that resembles the Windows registry, but with a more generalized interface. This container consists of nodes which, for example in the case of the Windows registry, containing a copy of keys and values related to a specific Windows registry key. These keys and values, if already existing, are loaded from the medium (in this case from a Windows Registry key) when the application starts and remains in memory for the whole application execution session. It's possible to read or modify these keys and values, but everything remains confined to the application's memory. When the application ends, the container is usually saved at the correct position, in the appropriate storage medium, and with its default format. If, in the meantime, the application crashes, the data initially loaded from the storage medium remain unchanged.

The library is mainly made by two parts: a container part (which is generalized) and a serialization part. From the application point of view, it allows having a coherent interface and permits to store persistent data on different storage mediums or formats. 

Application's persistent data are usually stored in the Windows Registry, respecting the conventions regarding the nature of the application itself (nature intended as a normal application or, e.g., a service application or other application type). Changing the serialization part via a template parameter (i.e it's passed like a Policy), it's possible to specify the data format (i.e. JSON, XML, INI, etc) or the storage medium, as the local computer, remote storage, or particular networking services. It also possible to have several serialization formats in the same application. Each serialization format is associated with a specific container, by the mean that each container links a serializer with the own serialization format.

The current library version includes the reader/writer for the Windows Registry only. In future versions will be possible to store data in JSON and XML files too. It will be added a code refactoring for an easy approach to extend the serialization in different media or formats.

## Getting Started

The library itself is made only by header files and therefore it is easy to use and to include in the codebase and does not require additional compilation steps: just needs to include the necessary header files in the project. Can also be used in contexts other than GUI applications, but its real advantages are seen in the writing of the latter, where it certainly simplifies the management of the persistence of application attributes such as the position, size, and state of the forms, up to the settings of the whole application.

### Prerequisites

It is necessary to install the boost libraries first. Anafestica uses "boost::variant" instead of "std::variant" due to an unresolved bug in the standard C++Builder's library, which prevents direct assignment of values to std::variant (See (https://quality.embarcadero.com/browse/RSP-27418) on Embarcadero Quality Portal).

It's possible to use GetIt for installing boost libraries (e.g. 1.68.0) in Embarcadero.

<img src="https://i.ibb.co/FmPznnX/3-611-D020-B-2-C12-4839-8567-A7-E8-A650940-E.png" alt="Figure 3">

Note that only _clang-based_ compilers are supported by this library.

### Installing

Clone the repository to $(BDSCOMMONDIR) which, normally, is %public%\Documents\Embarcadero\Studio\XX.X, where XX.X corresponds to the version of RAD Studio of interest. For example, for RAD Studio 10.3.3, $(BDSCOMMONDIR) corresponds to %public%\Documents\Embarcadero\Studio\XX.X which, in turn, is usually C:\Users\Public\Documents\Embarcadero\Studio\20.0.

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

