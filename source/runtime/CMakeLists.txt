include(CMakePrintHelpers)

####find_package(assimp CONFIG REQUIRED)
####message(STATUS "[spartan] assimp-${assimp_VERSION}")
####find_package(Bullet REQUIRED)
####message(STATUS "[spartan] bullet-${BULLET_VERSION_STRING}")
#####find_path(CMP_INCLUDE_DIR NAMES compressonatator.h PATHS ${spartan_SOURCE_DIR}/third_party/linux/include REQUIRED)
#####find_library(CMP_COMMON_LIBRARY NAMES CMP_Common PATHS ${spartan_SOURCE_DIR}/third_party/linux/lib REQUIRED)
#####find_library(CMP_LIBRARY NAMES CMP_Compressonator PATHS ${spartan_SOURCE_DIR}/third_party/linux/lib REQUIRED)
#####find_library(CMP_CORE_LIBRARY NAMES CMP_Core PATHS ${spartan_SOURCE_DIR}/third_party/linux/lib REQUIRED)
#####find_library(CMP_COMMON_LIBRARY NAMES CMP_Common PATHS ${spartan_SOURCE_DIR}/third_party/linux/lib REQUIRED)
#####add_library(Compressonator STATIC IMPORTED)
#####set_target_properties(Compressonator PROPERTIES
#####    IMPORTED_LOCATION "/path/to/compressonator/lib/libCMP_Core.a"
#####    INTERFACE_INCLUDE_DIRECTORIES "/path/to/compressonator/include"
#####)
#####target_link_libraries(my_project PRIVATE Compressonator)
####set(COMPRESSONATOR_LIB
####  ${spartan_SOURCE_DIR}/third_party/linux/lib/libCMP_Compressonator.a
####  ${spartan_SOURCE_DIR}/third_party/linux/lib/libCMP_Common.a
####  ${spartan_SOURCE_DIR}/third_party/linux/lib/libCMP_Core.a
####  ${spartan_SOURCE_DIR}/third_party/linux/lib/libCMP_Core_AVX.a
####  ${spartan_SOURCE_DIR}/third_party/linux/lib/libCMP_Core_AVX512.a
####  #${spartan_SOURCE_DIR}/third_party/linux/lib/libCMP_Framework.a
####  ${spartan_SOURCE_DIR}/third_party/linux/lib/libCMP_Core_SSE.a
####  #${spartan_SOURCE_DIR}/third_party/linux/lib/libCMP_GpuDecode.a
####)
####set(DXC_LIB
####  ${spartan_SOURCE_DIR}/third_party/linux/lib/libdxcompiler.so
####  ${spartan_SOURCE_DIR}/third_party/linux/lib/libdxil.so
####)

####find_package(PkgConfig REQUIRED)
####pkg_check_modules(FREETYPE2 REQUIRED IMPORTED_TARGET freetype2)
find_package(Freetype REQUIRED)
if(CMAKE_VERSION VERSION_LESS "4.2.0")
    message(STATUS "Freetype2 found: ${FREETYPE_VERSION_STRING}")
else()
    message(STATUS "Freetype2 found: ${Freetype_VERSION}")
endif()
####list(APPEND CMAKE_PREFIX_PATH "${spartan_SOURCE_DIR}/third_party/linux/lib/cmake/FreeImage")
####find_package(FreeImage REQUIRED)
####message(STATUS "[spartan] FreeImage-${FreeImage_VERSION}")
####find_package(meshoptimizer REQUIRED)
####message(STATUS "[spartan] meshoptimizer-${meshoptimizer_VERSION}")
####list(APPEND CMAKE_PREFIX_PATH "/opt/oidn/lib/cmake")
####find_package(OpenImageDenoise REQUIRED)
####message(STATUS "[spartan] OpenImageDenoise-${OpenImageDenoise_VERSION}")
####find_package(SDL3 REQUIRED CONFIG) #REQUIRED COMPONENTS SDL3-shared)
####message(STATUS "[spartan] SDL3-${SDL3_VERSION}")
####find_package(spirv_cross_c_shared REQUIRED)
#find_package(Vulkan REQUIRED REQUIRED COMPONENTS glslc dxc)
#include(FetchContent)
include(ExternalProject)

# PhysX paths and version
set(PHYSX_ROOT_DIR ${CMAKE_BINARY_DIR}/PhysX)
set(PHYSX_INSTALL_DIR ${CMAKE_BINARY_DIR}/physx-install)
set(PHYSX_VERSION "107.3-physx-5.6.1")

# Create necessary directories upfront to avoid cmake complaints for dies not existing
file(MAKE_DIRECTORY ${PHYSX_ROOT_DIR})
file(MAKE_DIRECTORY ${PHYSX_INSTALL_DIR}/include)
file(MAKE_DIRECTORY ${PHYSX_INSTALL_DIR}/lib)

# prepare PhysX source
if(NOT EXISTS ${PHYSX_ROOT_DIR}/src/physx)
    message(STATUS "PhysX source dir not found. Cloning version ${PHYSX_VERSION} ...")
    execute_process(
        COMMAND git clone --depth 1 --branch ${PHYSX_VERSION} https://github.com/NVIDIA-Omniverse/PhysX.git ${PHYSX_ROOT_DIR}/src
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        RESULT_VARIABLE GIT_RESULT
    )
    if(NOT GIT_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to clone PhysX repository")
    endif()
    message(STATUS "PhysX cloned successfully")
endif()

# Configure PhysX 5.6.1 as an external project (using existing source)
ExternalProject_Add(
    PhysX_external
    DOWNLOAD_COMMAND ""
    SOURCE_DIR ${PHYSX_ROOT_DIR}/src
    BINARY_DIR ${PHYSX_ROOT_DIR}/build

    PATCH_COMMAND 
        ${CMAKE_COMMAND} -E copy 
        ${CMAKE_SOURCE_DIR}/build_scripts/linux-gcc-cpu-only.xml 
        ${PHYSX_ROOT_DIR}/src/physx/buildtools/presets/public/linux-gcc-cpu-only.xml

    # PhysX uses generate_projects.sh, not direct CMake
    CONFIGURE_COMMAND
        bash -c "cd ${PHYSX_ROOT_DIR}/src/physx && ./generate_projects.sh linux-gcc-cpu-only --skip-samples --skip-snippets"

    # Build the release configuration
    BUILD_COMMAND
        make -C ${PHYSX_ROOT_DIR}/src/physx/compiler/linux-gcc-cpu-only-release -j${CMAKE_BUILD_PARALLEL_LEVEL}

    # Install: copy headers and libraries
    INSTALL_COMMAND
        ${CMAKE_COMMAND} -E copy_directory
            ${PHYSX_ROOT_DIR}/src/physx/include
            ${PHYSX_INSTALL_DIR}/include &&
        bash -c "cp ${PHYSX_ROOT_DIR}/src/physx/bin/linux.clang/release/*.a ${PHYSX_INSTALL_DIR}/lib/ 2>/dev/null || true"

    BUILD_IN_SOURCE FALSE
    STEP_TARGETS build install
)

# Create imported targets for PhysX libraries
add_library(PhysX::Foundation STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::Foundation PROPERTIES
    IMPORTED_LOCATION ${PHYSX_INSTALL_DIR}/lib/libPhysXFoundation_static_64.a
    INTERFACE_INCLUDE_DIRECTORIES "${PHYSX_INSTALL_DIR}/include"
)
add_dependencies(PhysX::Foundation PhysX_external-install)

add_library(PhysX::Common STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::Common PROPERTIES
    IMPORTED_LOCATION ${PHYSX_INSTALL_DIR}/lib/libPhysXCommon_static_64.a
    INTERFACE_INCLUDE_DIRECTORIES "${PHYSX_INSTALL_DIR}/include"
)
add_dependencies(PhysX::Common PhysX_external-install)

add_library(PhysX::PhysX STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::PhysX PROPERTIES
    IMPORTED_LOCATION ${PHYSX_INSTALL_DIR}/lib/libPhysX_static_64.a
    INTERFACE_INCLUDE_DIRECTORIES "${PHYSX_INSTALL_DIR}/include"
)
add_dependencies(PhysX::PhysX PhysX_external-install)

add_library(PhysX::Extensions STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::Extensions PROPERTIES
    IMPORTED_LOCATION ${PHYSX_INSTALL_DIR}/lib/libPhysXExtensions_static_64.a
    INTERFACE_INCLUDE_DIRECTORIES "${PHYSX_INSTALL_DIR}/include"
)
add_dependencies(PhysX::Extensions PhysX_external-install)

add_library(PhysX::Cooking STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::Cooking PROPERTIES
    IMPORTED_LOCATION ${PHYSX_INSTALL_DIR}/lib/libPhysXCooking_static_64.a
    INTERFACE_INCLUDE_DIRECTORIES "${PHYSX_INSTALL_DIR}/include"
)
add_dependencies(PhysX::Cooking PhysX_external-install)

add_library(PhysX::PvdSDK STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::PvdSDK PROPERTIES
    IMPORTED_LOCATION ${PHYSX_INSTALL_DIR}/lib/libPhysXPvdSDK_static_64.a
    INTERFACE_INCLUDE_DIRECTORIES "${PHYSX_INSTALL_DIR}/include"
)
add_dependencies(PhysX::PvdSDK PhysX_external-install)

add_library(PhysX::CharacterKinematic STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::CharacterKinematic PROPERTIES
    IMPORTED_LOCATION ${PHYSX_INSTALL_DIR}/lib/libPhysXCharacterKinematic_static_64.a
    INTERFACE_INCLUDE_DIRECTORIES "${PHYSX_INSTALL_DIR}/include"
)
add_dependencies(PhysX::CharacterKinematic PhysX_external-install)

add_library(PhysX::Vehicle STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::Vehicle PROPERTIES
    IMPORTED_LOCATION ${PHYSX_INSTALL_DIR}/lib/libPhysXVehicle_static_64.a
    INTERFACE_INCLUDE_DIRECTORIES "${PHYSX_INSTALL_DIR}/include"
)
add_dependencies(PhysX::Vehicle PhysX_external-install)

add_library(PhysX::Vehicle2 STATIC IMPORTED GLOBAL)
set_target_properties(PhysX::Vehicle2 PROPERTIES
    IMPORTED_LOCATION ${PHYSX_INSTALL_DIR}/lib/libPhysXVehicle2_static_64.a
    INTERFACE_INCLUDE_DIRECTORIES "${PHYSX_INSTALL_DIR}/include"
)
add_dependencies(PhysX::Vehicle2 PhysX_external-install)

find_package(Vulkan REQUIRED SPIRV-Tools glslang)
#####if(NOT DEFINED VULKAN_SDK)
#####    set(VULKAN_SDK $ENV{VULKAN_SDK})
#####endif()
#####
#####if(NOT VULKAN_SDK OR NOT EXISTS ${VULKAN_SDK})
#####    message(FATAL_ERROR "VULKAN_SDK not found: ${VULKAN_SDK}")
#####endif()
#####add_library(spirv-cross-core STATIC IMPORTED)
#####set_target_properties(spirv-cross-core PROPERTIES IMPORTED_LOCATION ${VULKAN_SDK}/lib/libspirv-cross-core.a)
#####
#####add_library(spirv-cross-glsl STATIC IMPORTED)
#####set_target_properties(spirv-cross-glsl PROPERTIES IMPORTED_LOCATION ${VULKAN_SDK}/lib/libspirv-cross-glsl.a)
#####
#####add_library(spirv-cross-cpp STATIC IMPORTED)
#####set_target_properties(spirv-cross-cpp PROPERTIES IMPORTED_LOCATION ${VULKAN_SDK}/lib/libspirv-cross-cpp.a)
#####
#####add_library(spirv-cross-c STATIC IMPORTED)
#####set_target_properties(spirv-cross-c PROPERTIES IMPORTED_LOCATION ${VULKAN_SDK}/lib/libspirv-cross-c.a)
#####
#####add_library(spirv-cross-hlsl STATIC IMPORTED)
#####set_target_properties(spirv-cross-hlsl PROPERTIES IMPORTED_LOCATION ${VULKAN_SDK}/lib/libspirv-cross-hlsl.a)

foreach(DIR Commands Core Display FileSystem Font Game Geometry Input IO Logging Math Memory Physics Profiling Rendering Resource World)
  file(GLOB_RECURSE RUNTIME_SRC_${DIR} CONFIGURE_DEPENDS ${DIR}/*.cpp)
  list(APPEND RUNTIME_SRC ${RUNTIME_SRC_${DIR}})

  file(GLOB RUNTIME_HEADER_${DIR} CONFIGURE_DEPENDS ${DIR}/*.h ${DIR}/*.hpp)
  list(APPEND RUNTIME_HEADER ${RUNTIME_HEADER_${DIR}})
endforeach()
# Only vulkan builds are supported using CMake, so only add those
# TODO add a cmake option for directx builds
file(GLOB RUNTIME_SRC_RHI CONFIGURE_DEPENDS RHI/*.cpp RHI/Vulkan/*.cpp)
list(APPEND RUNTIME_SRC ${RUNTIME_SRC_RHI})
file(GLOB RUNTIME_HEADER_RHI CONFIGURE_DEPENDS RHI/*.h RHI/Vulkan/*.h RHI/*.hpp RHI/Vulkan/*.hpp)
list(APPEND RUNTIME_HEADER ${RUNTIME_HEADER_RHI})
#list(REMOVE_ITEM RUNTIME_HEADER "Core/pch.h")

####NIKOS if (MSVC)
####NIKOS   set(LIBRARY_TYPE STATIC)
####NIKOS else()
####NIKOS   set(LIBRARY_TYPE STATIC)
####NIKOS endif()
add_library(runtime ${LIBRARY_TYPE} ${RUNTIME_SRC} ${RUNTIME_HEADER})
target_precompile_headers(runtime PRIVATE Core/pch.h)
####NIKOS set(VCPKG_ROOT "$ENV{HOME}/github.com/vcpkg/installed/x64-linux")

target_include_directories(runtime
  PUBLIC
    Core
#    ${CMAKE_SOURCE_DIR}/third_party/linux/include
    ${spartan_SOURCE_DIR}/third_party/free_image
  PRIVATE
#    ${BULLET_INCLUDE_DIR}
#    ${VCPKG_ROOT}/include
)
#TODO add parameter for API_GRAPHICS_D3D12 instead of API_GRAPHICS_VULKAN
target_compile_definitions(runtime PUBLIC
  $<$<CONFIG:Debug>:DEBUG>
  API_GRAPHICS_VULKAN
    PX_PHYSX_STATIC_LIB
    NDEBUG
    PX_LINUX=1
)

target_compile_options(runtime PUBLIC 
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:${SPARTAN_GCC_CLANG_CXX_FLAGS}>
)

target_link_libraries(runtime
  PUBLIC
    PhysX::Vehicle2
    PhysX::Vehicle
    PhysX::CharacterKinematic
    PhysX::Extensions
    PhysX::PhysX
    PhysX::Cooking
    PhysX::PvdSDK
    PhysX::Common
    PhysX::Foundation
    pthread
    dl
    m
    rt
    Freetype::Freetype
  #    PkgConfig::FREETYPE2
  #  SDL3::SDL3
    #  PRIVATE
  #    assimp::assimp
  #    ${BULLET_LIBRARIES}
  #    ${COMPRESSONATOR_LIB}
  #    ${DXC_LIB}
  #    FreeImage::FreeImage
  #    meshoptimizer::meshoptimizer
  #    OpenImageDenoise
  #    spirv-cross-core
  #    spirv-cross-c
  #    spirv-cross-cpp
  #    spirv-cross-glsl
  #    spirv-cross-hlsl
  #    spirv-cross-c-shared
  #    Vulkan::Vulkan
)
