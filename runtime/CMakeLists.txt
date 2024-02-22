#include(../cmake/runtime-file-list.cmake)
#include(CMakePrintHelpers)
foreach(DIR Audio Commands Core Display Input IO Logging Math Physics Profiling Rendering Resource World)
  file(GLOB_RECURSE RUNTIME_SRC_${DIR} CONFIGURE_DEPENDS ${DIR}/*.cpp)
  list(APPEND RUNTIME_SRC ${RUNTIME_SRC_${DIR}})

  file(GLOB_RECURSE RUNTIME_HEADER_${DIR} CONFIGURE_DEPENDS ${DIR}/*.h ${DIR}/*.hpp)
  list(APPEND RUNTIME_HEADER ${RUNTIME_HEADER_${DIR}})

  #  cmake_print_variables(RUNTIME_SRC_${DIR} RUNTIME_HEADER_${DIR})
endforeach()
file(GLOB RUNTIME_SRC_RHI CONFIGURE_DEPENDS RHI/*.cpp RHI/Vulkan/*.cpp)
list(APPEND RUNTIME_SRC ${RUNTIME_SRC_RHI})
file(GLOB RUNTIME_HEADER_RHI CONFIGURE_DEPENDS RHI/*.h RHI/Vulkan/*.h RHI/*.hpp RHI/Vulkan/*.hpp)
list(APPEND RUNTIME_HEADER ${RUNTIME_HEADER_RHI})
#cmake_print_variables(RUNTIME_SRC_RHI RUNTIME_HEADER_RHI)
#set(RUNTIME_SRC
#  ${RUNTIME_SRC_AUDIO}
#  ${RUNTIME_SRC_COMMANDS}
#  ${RUNTIME_SRC_CORE}
#  ${RUNTIME_SRC_DISPLAY}
#  ${RUNTIME_SRC_IO}
#  ${RUNTIME_SRC_INPUT}
#  ${RUNTIME_SRC_LOGGING}
#  ${RUNTIME_SRC_MATH}
#  ${RUNTIME_SRC_PHYSICS}
#  ${RUNTIME_SRC_PROFILING}
#  ${RUNTIME_SRC_RHI}
#  ${RUNTIME_SRC_RENDERING}
#  ${RUNTIME_SRC_RESOURCE}
#  ${RUNTIME_SRC_WORLD}
#)
#set(RUNTIME_HEADER
#  ${RUNTIME_HEADER_AUDIO}
#  ${RUNTIME_HEADER_COMMANDS}
#  ${RUNTIME_HEADER_CORE}
#  ${RUNTIME_HEADER_DISPLAY}
#  ${RUNTIME_HEADER_IO}
#  ${RUNTIME_HEADER_INPUT}
#  ${RUNTIME_HEADER_LOGGING}
#  ${RUNTIME_HEADER_MATH}
#  ${RUNTIME_HEADER_PHYSICS}
#  ${RUNTIME_HEADER_PROFILING}
#  ${RUNTIME_HEADER_RHI}
#  ${RUNTIME_HEADER_RENDERING}
#  ${RUNTIME_HEADER_RESOURCE}
#  ${RUNTIME_HEADER_WORLD}
#)

add_library(runtime ${RUNTIME_SRC} ${RUNTIME_HEADER})
target_include_directories(runtime
  PUBLIC
    Core
    ${BULLET_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/third_party/linux/include
    ${spartan_SOURCE_DIR}/third_party/amd_fidelity_fx
    ${spartan_SOURCE_DIR}/third_party/open_image_denoise
)
target_compile_definitions(runtime PRIVATE DEBUG API_GRAPHICS_VULKAN)
target_link_libraries(runtime
  PRIVATE 
    Vulkan::Vulkan
    spirv-cross-c-shared
    PkgConfig::FREETYPE2
    ${BULLET_LIBRARIES}
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
)
