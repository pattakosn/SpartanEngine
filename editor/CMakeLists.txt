file(GLOB_RECURSE EDITOR_SRC CONFIGURE_DEPENDS *.cpp)
file(GLOB_RECURSE EDITOR_HEADER CONFIGURE_DEPENDS *.h *hpp)

add_executable(spartan_vulkan WIN32 ${EDITOR_SRC} ${EDITOR_HEADER})
target_include_directories(spartan_vulkan PUBLIC ${spartan_SOURCE_DIR}/runtime)
target_link_libraries(spartan_vulkan PRIVATE runtime)