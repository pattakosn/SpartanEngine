cmake_minimum_required(VERSION 3.30)
project(spartan
        VERSION 0.3.3
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



###NIKOS EXECUTABLE_NAME=spartan_d3d12 or spartan_d3d12
###NIKOS LIBRARY_DIR=third_party/libraries
###NIKOS OBJ_DIR=binaries/obj
###NIKOS TARGET_DIR=binaries
###NIKOS sto install xreaizontai:
###NIKO     binaries/download_assets.py,file_utilities.py, data


# Require out-of-source builds
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if(EXISTS "${LOC_PATH}")
  message(FATAL_ERROR "[spartan] You cannot build in a source directory (or any directory with a CMakeLists.txt file). Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles.")
endif()

# debug build by default, release optionally.
set(default_build_type "Debug")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "[spartan] Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

#NIKOS TODO: ensure these are used:
#            -mavx2
#            /arch:AVX2, symbols "On", debugformat "c7, staticruntime "On", conformancemode "On"
#set(SPARTAN_GCC_CLANG_CXX_FLAGS  "-Wall;-Wextra;-pedantic")
# I do not know why this is required for clang but without it i had COM compile errors
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  list(APPEND CMAKE_CXX_FLAGS "-fms-extensions")
endif()
option(SPARTAN_ENABLE_IWYU "Enable IncludeWhatYouUse tool" OFF)
if(SPARTAN_ENABLE_IWYU)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang") 
    find_program(IWYU_PATH NAMES include-what-you-use)
    if(IWYU_PATH)
      set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${IWYU_PATH})
      message(STATUS "IWYU enabled: ${IWYU_PATH}")
    else()
      message(WARNING "include-what-you-use not found!")
    endif()
  endif()
endif()
#NIKOS TODO: add other spartan specific cmake options*


option(SPARTAN_ENABLE_IPO "Enable Interprocedural Optimization (IPO, also known as LTO), if available" OFF)
if(SPARTAN_ENABLE_IPO)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT IsIPOSupported)
  if(IsIPOSupported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    message(STATUS "[spartan] nterprocedural Optimization (IPO, also known as LTO) is enabled")
  else()
    message(STATUS "[spartan] Interprocedural Optimization (IPO, also known as LTO) requested but not supported: ${result}")
  endif()
else()
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION FALSE)
  message(STATUS "[spartan] Interprocedural Optimization (IPO, also known as LTO) is disabled")
endif()

set(API_GRAPHICS_VULKAN "API_GRAPHICS_VULKAN")
#set(API_GRAPHICS_D3D12 "API_GRAPHICS_D3D12")
set(DEFAULT_API_GRAPHICS ${API_GRAPHICS_VULKAN})
set(API_GRAPHICS_USED ${DEFAULT_API_GRAPHICS} CACHE STRING "spartan supports Vulkan on Windows or Linux and DX12 on Windows.Will by default build for Vulkan")
if(UNIX)
    set_property(CACHE API_GRAPHICS_USED PROPERTY STRINGS ${API_GRAPHICS_VULKAN})
elseif(WIN32)
    set_property(CACHE API_GRAPHICS_USED PROPERTY STRINGS ${API_GRAPHICS_VULKAN})# ${API_GRAPHICS_D3D12})
endif()
if(API_GRAPHICS_USED STREQUAL "API_GRAPHICS_VULKAN")
    message(STATUS "[spartan] Vulkan build")
#elseif(API_GRAPHICS_USED STREQUAL "API_GRAPHICS_D3D12")
#    message(STATUS "[spartan] D3D12 build")
endif()# Append _vulkan or _d3d12 to the executable name based on the selected graphics API
if(API_GRAPHICS_USED STREQUAL "API_GRAPHICS_VULKAN")
    set(EXECUTABLE_SUFFIX "_vulkan")
#elseif(API_GRAPHICS_USED STREQUAL "API_GRAPHICS_D3D12")
#    set(EXECUTABLE_SUFFIX "_d3d12")
else()
    message(FATAL_ERROR "[spartan] Invalid API_GRAPHICS_USED value")
endif()

add_subdirectory(source)
