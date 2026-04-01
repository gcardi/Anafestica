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

# Auto-detect BOOST_ROOT from Embarcadero registry setting if not set externally.
if(WIN32 AND (NOT DEFINED BOOST_ROOT OR BOOST_ROOT STREQUAL ""))
  execute_process(
    COMMAND reg query "HKCU\\Software\\Embarcadero\\BDS\\37.0\\Environment Variables" /v CG_64X_BOOST_ROOT
    RESULT_VARIABLE _regs_result
    OUTPUT_VARIABLE _regs_output
    ERROR_QUIET
  )

  message(STATUS "Registry query result: ${_regs_result}")
  message(STATUS "Registry output: '${_regs_output}'")
  if(_regs_result EQUAL 0)
    string(REGEX MATCH "CG_64X_BOOST_ROOT[ \t]+REG_[A-Z_]+[ \t]+(.*)" _boost_match "${_regs_output}")
    message(STATUS "Registry match: '${_boost_match}'")
    message(STATUS "CMAKE_MATCH_1='${CMAKE_MATCH_1}'")
    if(_boost_match)
      string(STRIP "${CMAKE_MATCH_1}" BOOST_ROOT)
      message(STATUS "Found CG_64X_BOOST_ROOT in registry: ${BOOST_ROOT}")

      # If BOOST_ROOT is an include path, trim to Embarcadero Studio root.
      # Example: c:/program files (x86)/embarcadero/studio/37.0/include/boost_1_89
      get_filename_component(_tmp "${BOOST_ROOT}" ABSOLUTE)
      get_filename_component(_tmp "${_tmp}" DIRECTORY) # .../include
      get_filename_component(_tmp "${_tmp}" DIRECTORY) # .../37.0
      set(BOOST_ROOT "${_tmp}")
      set(BOOST_LIBRARYDIR "${BOOST_ROOT}/lib/win64x/release")
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
message(STATUS "DEBUG: CG_64X_BOOST_ROOT='${CG_64X_BOOST_ROOT}'")
set(Boost_USE_STATIC_LIBS OFF CACHE BOOL "" FORCE)
set(Boost_USE_MULTITHREADED ON CACHE BOOL "" FORCE)
set(Boost_USE_STATIC_RUNTIME OFF CACHE BOOL "" FORCE)

find_package(Boost 1.65 COMPONENTS unit_test_framework QUIET)

if(NOT Boost_FOUND AND DEFINED BOOST_ROOT)
  # Use the include path from the environment variable in BDS
  set(Boost_INCLUDE_DIRS "${BOOST_ROOT}/include/boost_1_89")
  set(Boost_LIBRARYDIR "${BOOST_ROOT}/lib/win64x/release")

  message(STATUS "BOOST_ROOT is ${BOOST_ROOT}")
  message(STATUS "Boost include path = ${Boost_INCLUDE_DIRS}")
  message(STATUS "Boost library path = ${Boost_LIBRARYDIR}")

  file(GLOB BOOST_TEST_LIBS
    "${Boost_LIBRARYDIR}/*boost_unit_test_framework*1_89*"
    "${Boost_LIBRARYDIR}/*unit_test_framework*1_89*"
  )

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

# Source files
set(ANAFESTICA_TEST_SOURCES
  Test/runner.cpp
  Test/test_types.cpp
)

add_executable(anafestica_test ${ANAFESTICA_TEST_SOURCES})

target_compile_definitions(anafestica_test PRIVATE UNICODE _UNICODE)

target_include_directories(anafestica_test PRIVATE
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/anafestica
  ${Boost_INCLUDE_DIRS}
)

target_link_libraries(anafestica_test PRIVATE
  ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
)

if(WIN32)
  # Registry and Windows wrapper support
  target_link_libraries(anafestica_test PRIVATE advapi32 rtl)
  if(DEFINED BOOST_ROOT)
    target_link_libraries(anafestica_test PRIVATE
      "${BOOST_ROOT}/lib/win64x/release/SysInit.o"
    )
  endif()
endif()

# Optional test runner federation
enable_testing()
set(CTEST_OUTPUT_ON_FAILURE TRUE CACHE INTERNAL "Show output when test fails")
add_test(NAME ana COMMAND anafestica_test)
