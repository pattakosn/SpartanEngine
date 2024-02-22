cmake_minimum_required(VERSION 3.14...3.29)
project(spartan
        VERSION 1.0
        DESCRIPTION "Panos Karambelas' awesome 3D engine"
        LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
#set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

# Require out-of-source builds
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if(EXISTS "${LOC_PATH}")
  message(FATAL_ERROR "You cannot build in a source directory (or any directory with a CMakeLists.txt file). Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles.")
endif()

# Releae build by default, Debug optionally.
set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")# "MinSizeRel" "RelWithDebInfo")
endif()

# Dependencies
find_package(Vulkan REQUIRED REQUIRED COMPONENTS glslc dxc)
find_package(spirv_cross_c_shared REQUIRED)
find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)
find_package(assimp CONFIG REQUIRED)
find_package(Bullet REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FREETYPE2 REQUIRED IMPORTED_TARGET freetype2)

#find_package(freeimage CONFIG REQUIRED)
#add_library(freeimage STATIC IMPORTED)
#set_property(TARGET freeimage PROPERTY
#             IMPORTED_LOCATION "/usr/lib/libfreeimage.so")

#fmod
#dxcompiler
#comperssonator
#renderdoc
#pugixml
#spirv-cross-{core,cpp,glsl,hlsl}
#ffx_{backend_vk,fsr2,spd}

#set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fms-extensions")
endif()

add_subdirectory(runtime)
add_subdirectory(editor)
