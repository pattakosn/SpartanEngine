cmake_minimum_required(VERSION 3.14...3.31)
project(spartan
        VERSION 1.0
        DESCRIPTION "Panos Karambelas' awesome 3D engine"
        LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
#set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

# Require out-of-source builds
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if(EXISTS "${LOC_PATH}")
  message(FATAL_ERROR "You cannot build in a source directory (or any directory with a CMakeLists.txt file). Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles.")
endif()

# Release build by default, Debug optionally.
set(default_build_type "Debug")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

#set(SPARTAN_GCC_CLANG_CXX_FLAGS  "-Wall;-Wextra;-pedantic")
# I do not know why this is required for clang but without it i had COM compile errors
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  list(APPEND CMAKE_CXX_FLAGS "-fms-extensions")
endif()

#include(CheckIPOSupported)
#check_ipo_supported(RESULT IsIPOSupported)
#if(IsIPOSupported)
#    message(STATUS "IPO / LTO enabled")
#    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
#else()
#    message(STATUS "IPO / LTO not supported: ${result}")
#endif()

add_subdirectory(runtime)
add_subdirectory(editor)
