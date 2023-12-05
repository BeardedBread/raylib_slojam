add_library(lib_scenes STATIC
    level_scene.c
)
target_include_directories(lib_scenes
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(lib_scenes
    PUBLIC
    lib_engine
)
