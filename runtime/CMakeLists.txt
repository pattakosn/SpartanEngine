find_package(assimp CONFIG REQUIRED)
message(STATUS "[spartan] assimp-${assimp_VERSION}")
find_package(Bullet REQUIRED)
message(STATUS "[spartan] bullet-${BULLET_VERSION_STRING}")
#find_path(CMP_INCLUDE_DIR NAMES compressonatator.h PATHS ${spartan_SOURCE_DIR}/third_party/linux/include REQUIRED)
#find_library(CMP_COMMON_LIBRARY NAMES CMP_Common PATHS ${spartan_SOURCE_DIR}/third_party/linux/lib REQUIRED)
#find_library(CMP_LIBRARY NAMES CMP_Compressonator PATHS ${spartan_SOURCE_DIR}/third_party/linux/lib REQUIRED)
#find_library(CMP_CORE_LIBRARY NAMES CMP_Core PATHS ${spartan_SOURCE_DIR}/third_party/linux/lib REQUIRED)
#find_library(CMP_COMMON_LIBRARY NAMES CMP_Common PATHS ${spartan_SOURCE_DIR}/third_party/linux/lib REQUIRED)
#add_library(Compressonator STATIC IMPORTED)
#set_target_properties(Compressonator PROPERTIES
#    IMPORTED_LOCATION "/path/to/compressonator/lib/libCMP_Core.a"
#    INTERFACE_INCLUDE_DIRECTORIES "/path/to/compressonator/include"
#)
#target_link_libraries(my_project PRIVATE Compressonator)
set(COMPRESSONATOR_LIB
  ${spartan_SOURCE_DIR}/third_party/linux/lib/libCMP_Compressonator.a
  ${spartan_SOURCE_DIR}/third_party/linux/lib/libCMP_Common.a
  ${spartan_SOURCE_DIR}/third_party/linux/lib/libCMP_Core.a
  ${spartan_SOURCE_DIR}/third_party/linux/lib/libCMP_Core_AVX.a
  ${spartan_SOURCE_DIR}/third_party/linux/lib/libCMP_Core_AVX512.a
  #${spartan_SOURCE_DIR}/third_party/linux/lib/libCMP_Framework.a
  ${spartan_SOURCE_DIR}/third_party/linux/lib/libCMP_Core_SSE.a
  #${spartan_SOURCE_DIR}/third_party/linux/lib/libCMP_GpuDecode.a
)
set(DXC_LIB
  ${spartan_SOURCE_DIR}/third_party/linux/lib/libdxcompiler.so
  ${spartan_SOURCE_DIR}/third_party/linux/lib/libdxil.so
)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FREETYPE2 REQUIRED IMPORTED_TARGET freetype2)
list(APPEND CMAKE_PREFIX_PATH "${spartan_SOURCE_DIR}/third_party/linux/lib/cmake/FreeImage")
find_package(FreeImage REQUIRED)
message(STATUS "[spartan] FreeImage-${FreeImage_VERSION}")
find_package(meshoptimizer REQUIRED)
message(STATUS "[spartan] meshoptimizer-${meshoptimizer_VERSION}")
find_package(OpenImageDenoise REQUIRED)
message(STATUS "[spartan] OpenImageDenoise-${OpenImageDenoise_VERSION}")
find_package(SDL3 REQUIRED CONFIG) #REQUIRED COMPONENTS SDL3-shared)
message(STATUS "[spartan] SDL3-${SDL3_VERSION}")
find_package(spirv_cross_c_shared REQUIRED)
find_package(Vulkan REQUIRED REQUIRED COMPONENTS glslc dxc)

foreach(DIR Commands Core Display FileSystem Font Game Geometry Input IO Logging Math Physics Profiling Rendering Resource World)
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

if (MSVC)
    set(LIBRARY_TYPE STATIC)
else()
    set(LIBRARY_TYPE SHARED)
endif()
add_library(runtime ${LIBRARY_TYPE} ${RUNTIME_SRC} ${RUNTIME_HEADER})

target_precompile_headers(runtime PRIVATE Core/pch.h)

target_include_directories(runtime
  PUBLIC
    Core
    ${CMAKE_SOURCE_DIR}/third_party/linux/include
    ${spartan_SOURCE_DIR}/third_party/free_image # fucking FreeImage/Utilities.h is not in upstream install, do not know why
  PRIVATE
    ${BULLET_INCLUDE_DIR}

)
#TODO add parameter for API_GRAPHICS_D3D12 instead of API_GRAPHICS_VULKAN
target_compile_definitions(runtime PUBLIC
  $<$<CONFIG:Debug>:DEBUG>
  API_GRAPHICS_VULKAN
)

target_compile_options(runtime PUBLIC 
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:${SPARTAN_GCC_CLANG_CXX_FLAGS}>
)

target_link_libraries(runtime
  PUBLIC
    PkgConfig::FREETYPE2
    SDL3::SDL3
  PRIVATE
    assimp::assimp
    ${BULLET_LIBRARIES}
    ${COMPRESSONATOR_LIB}
    ${DXC_LIB}
    FreeImage::FreeImage
    meshoptimizer::meshoptimizer
    OpenImageDenoise
    spirv-cross-core
    spirv-cross-c
    spirv-cross-cpp
    spirv-cross-glsl
    spirv-cross-hlsl
    spirv-cross-c-shared
    Vulkan::Vulkan
)