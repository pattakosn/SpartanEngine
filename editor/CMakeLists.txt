#include(${spartan_SOURCE_DIR}/cmake/editor-file-list.cmake)
file(GLOB_RECURSE EDITOR_SRC CONFIGURE_DEPENDS *.cpp)
file(GLOB_RECURSE EDITOR_HEADER CONFIGURE_DEPENDS *.h *hpp)
#set(EDITOR_SRC
#  Editor.cpp
#  main.cpp
#  ${EDITOR_SRC_IMGUI}
#  ${EDITOR_SRC_WIDGETS}
#  ${EDITOR_SRC_WIDGETSDEFERRED}
#)
#set(EDITOR_HEADER
#  Editor.h
#  ${EDITOR_HEADER_IMGUI}
#  ${EDITOR_HEADER_WIDGETS}
#  ${EDITOR_HEADER_WIDGETSDEFERRED}
#)

add_executable(spartan_vulkan WIN32 ${EDITOR_SRC} ${EDITOR_HEADER})
#add_library(spartan_vulkan ${EDITOR_SRC} ${EDITOR_HEADER})
target_compile_definitions(spartan_vulkan PRIVATE API_GRAPHICS_VULKAN)
target_include_directories(spartan_vulkan
  PUBLIC
    ${spartan_SOURCE_DIR}/runtime
    ${spartan_SOURCE_DIR}/runtime/Core
)
target_link_libraries(spartan_vulkan
  PRIVATE 
    runtime
    PkgConfig::FREETYPE2
    Vulkan::Vulkan
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    assimp::assimp
    ${BULLET_LIBRARIES}
)
