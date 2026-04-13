cmake_minimum_required(VERSION 3.27)
project(AnafesticaTest LANGUAGES CXX)

# Use C++Builder compiler by default when not specified on the command line.
if(NOT CMAKE_C_COMPILER)
  set(CMAKE_C_COMPILER bcc64x CACHE STRING "C compiler" FORCE)
endif()

if(NOT CMAKE_CXX_COMPILER)
  set(CMAKE_CXX_COMPILER bcc64x CACHE STRING "C++ compiler" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Suppress Boost macro redefinition warning
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-macro-redefined")

# Optional: change to Release/Debug as needed
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

# Select the Boost registry key, include version, and library directory
# based on which Embarcadero compiler is active.
#
#   bcc64x  → CG_64X_BOOST_ROOT  (Boost 1.89, lib/win64x/release)
#   bcc64   → CG_64_BOOST_ROOT   (Boost 1.70, lib/win64/release)
#   bcc32c  → CG_32_BOOST_ROOT   (Boost 1.70, lib/win32c/release)
#
# Boost is needed for Boost.Test regardless of whether boost::variant is used.
if(CMAKE_CXX_COMPILER MATCHES "bcc64x")
  set(_ANA_BOOST_REGVAL  "CG_64X_BOOST_ROOT")
  set(_ANA_BOOST_INCVER  "boost_1_89")
  set(_ANA_BOOST_LIBDIR  "lib/win64x/release")
elseif(CMAKE_CXX_COMPILER MATCHES "bcc64")
  set(_ANA_BOOST_REGVAL  "CG_64_BOOST_ROOT")
  set(_ANA_BOOST_INCVER  "boost_1_70")
  set(_ANA_BOOST_LIBDIR  "lib/win64/release")
elseif(CMAKE_CXX_COMPILER MATCHES "bcc32c")
  set(_ANA_BOOST_REGVAL  "CG_32_BOOST_ROOT")
  set(_ANA_BOOST_INCVER  "boost_1_70")
  set(_ANA_BOOST_LIBDIR  "lib/win32c/release")
else()
  set(_ANA_BOOST_REGVAL  "CG_64X_BOOST_ROOT")
  set(_ANA_BOOST_INCVER  "boost_1_89")
  set(_ANA_BOOST_LIBDIR  "lib/win64x/release")
endif()
message(STATUS "Boost registry key: ${_ANA_BOOST_REGVAL}")

# Auto-detect BOOST_ROOT from Embarcadero registry setting if not set externally.
if(WIN32 AND (NOT DEFINED BOOST_ROOT OR BOOST_ROOT STREQUAL ""))
  execute_process(
    COMMAND reg query "HKCU\\Software\\Embarcadero\\BDS\\37.0\\Environment Variables" /v ${_ANA_BOOST_REGVAL}
    RESULT_VARIABLE _regs_result
    OUTPUT_VARIABLE _regs_output
    ERROR_QUIET
  )

  message(STATUS "Registry query result: ${_regs_result}")
  message(STATUS "Registry output: '${_regs_output}'")
  if(_regs_result EQUAL 0)
    string(REGEX MATCH "${_ANA_BOOST_REGVAL}[ \t]+REG_[A-Z_]+[ \t]+(.*)" _boost_match "${_regs_output}")
    message(STATUS "Registry match: '${_boost_match}'")
    message(STATUS "CMAKE_MATCH_1='${CMAKE_MATCH_1}'")
    if(_boost_match)
      string(STRIP "${CMAKE_MATCH_1}" BOOST_ROOT)
      message(STATUS "Found ${_ANA_BOOST_REGVAL} in registry: ${BOOST_ROOT}")

      # The registry value is an include path ending in boost_1_NN; trim up two
      # levels to reach the Embarcadero Studio root (e.g. .../37.0).
      get_filename_component(_tmp "${BOOST_ROOT}" ABSOLUTE)
      get_filename_component(_tmp "${_tmp}" DIRECTORY) # .../include
      get_filename_component(_tmp "${_tmp}" DIRECTORY) # .../37.0
      set(BOOST_ROOT "${_tmp}")
      set(BOOST_LIBRARYDIR "${BOOST_ROOT}/${_ANA_BOOST_LIBDIR}")
      message(STATUS "Inferred BOOST_ROOT: ${BOOST_ROOT}")
      message(STATUS "Inferred BOOST_LIBRARYDIR: ${BOOST_LIBRARYDIR}")
    endif()
  endif()
endif()

# find_package policy for Boost module compatibility
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 OLD)
endif()

# Find Boost unit test framework (prefer find_package, fallback to Embarcadero prebuilt libs)
message(STATUS "DEBUG: BOOST_ROOT='${BOOST_ROOT}'")
message(STATUS "DEBUG: BOOST_LIBRARYDIR='${BOOST_LIBRARYDIR}'")
message(STATUS "DEBUG: ${_ANA_BOOST_REGVAL}='${_ANA_BOOST_REGVAL}'")
set(Boost_USE_STATIC_LIBS OFF CACHE BOOL "" FORCE)
set(Boost_USE_MULTITHREADED ON CACHE BOOL "" FORCE)
set(Boost_USE_STATIC_RUNTIME OFF CACHE BOOL "" FORCE)

find_package(Boost 1.65 COMPONENTS unit_test_framework QUIET)

if(NOT Boost_FOUND AND DEFINED BOOST_ROOT)
  # Use the include path from the environment variable in BDS
  set(Boost_INCLUDE_DIRS "${BOOST_ROOT}/include/${_ANA_BOOST_INCVER}")
  set(Boost_LIBRARYDIR "${BOOST_ROOT}/${_ANA_BOOST_LIBDIR}")

  message(STATUS "BOOST_ROOT is ${BOOST_ROOT}")
  message(STATUS "Boost include path = ${Boost_INCLUDE_DIRS}")
  message(STATUS "Boost library path = ${Boost_LIBRARYDIR}")

  # Determine library extension based on compiler
  if(CMAKE_CXX_COMPILER MATCHES "bcc64x")
    set(LIB_EXTS ".lib" ".a")
  else()
    set(LIB_EXTS ".a" ".lib")
  endif()

  set(BOOST_TEST_LIBS "")
  foreach(EXT IN LISTS LIB_EXTS)
    file(GLOB LIBS "${Boost_LIBRARYDIR}/*boost_unit_test_framework*${EXT}"
                   "${Boost_LIBRARYDIR}/*unit_test_framework*${EXT}")
    list(APPEND BOOST_TEST_LIBS ${LIBS})
  endforeach()

  message(STATUS "Glob results for unit test libs: ${BOOST_TEST_LIBS}")

  if(BOOST_TEST_LIBS)
    list(GET BOOST_TEST_LIBS 0 Boost_UNIT_TEST_FRAMEWORK_LIBRARY)
    set(Boost_FOUND TRUE)
    message(STATUS "Found Boost unit_test_framework library at ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}")
  endif()
endif()

if(NOT Boost_FOUND)
  message(FATAL_ERROR "Boost unit_test_framework required. Cannot find boost at BDS path or system path.")
endif()

if(NOT Boost_FOUND)
  message(FATAL_ERROR "Boost unit_test_framework required")
endif()

set(ANAFESTICA_TEST_COMMON_SOURCES
  Test/TestBcc64x/runner.cpp
)

set(ANAFESTICA_TEST_BCC64X_SOURCES
  Test/TestBcc64x/test_types.cpp
  Test/TestBcc64x/test_config.cpp
  Test/TestBcc64x/test_type_mismatch.cpp
  Test/TestBcc64x/test_singleton_version_info.cpp
)

set(ANAFESTICA_TEST_BCC64_SOURCES
  Test/TestBcc64/test_bcc64_variant_compat.cpp
  Test/TestBcc64/test_config.cpp
  Test/TestBcc64x/test_type_mismatch.cpp
  Test/TestBcc64x/test_types.cpp
  Test/TestBcc64x/test_singleton_version_info.cpp
)

function(configure_anafestica_test_target target_name)
  target_compile_definitions(${target_name} PRIVATE UNICODE _UNICODE)

  # ANAFESTICA_USE_STD_VARIANT is auto-detected inside CfgNodeValueType.h
  # based on the Clang major version, so no manual definition is needed here.

  target_include_directories(${target_name} PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/anafestica
    ${Boost_INCLUDE_DIRS}
  )

  target_link_libraries(${target_name} PRIVATE
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
  )

  if(WIN32)
    # Registry, XML and Windows wrapper support
    target_link_libraries(${target_name} PRIVATE advapi32 rtl xmlrtl)
    if(DEFINED BOOST_ROOT)
      target_link_libraries(${target_name} PRIVATE
        "${BOOST_ROOT}/${_ANA_BOOST_LIBDIR}/SysInit.o"
      )
    endif()
  endif()
endfunction()

add_executable(
  anafestica_test_bcc64x
  ${ANAFESTICA_TEST_COMMON_SOURCES}
  ${ANAFESTICA_TEST_BCC64X_SOURCES}
)
configure_anafestica_test_target(anafestica_test_bcc64x)

add_executable(
  anafestica_test_bcc64
  ${ANAFESTICA_TEST_COMMON_SOURCES}
  ${ANAFESTICA_TEST_BCC64_SOURCES}
)
configure_anafestica_test_target(anafestica_test_bcc64)

# Optional test runner federation
enable_testing()
set(CTEST_OUTPUT_ON_FAILURE TRUE CACHE INTERNAL "Show output when test fails")
add_test(NAME ana_bcc64x COMMAND anafestica_test_bcc64x)
add_test(NAME ana_bcc64 COMMAND anafestica_test_bcc64)
