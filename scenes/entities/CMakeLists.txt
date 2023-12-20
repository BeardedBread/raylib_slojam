add_library(lib_entities STATIC
    player.c
    bullet.c
    enemies.c
    collectibles.c
)
target_include_directories(lib_entities
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)
target_link_libraries(lib_entities
    PRIVATE
    lib_engine
)
