


add_executable(editor ${EDITOR_SRC} ${EDITOR_HEADERS})
add_target_libraries(editor SpartanEngine::runtime)
