add_subdirectory(sc)
add_library(lib_engine STATIC
    mempool.c
    entManager.c
    engine.c
    particle_sys.c
    assets.c
)
target_include_directories(lib_engine
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${RAYLIB_DIR}/include
)
target_link_directories(lib_engine
    PUBLIC
    ${RAYLIB_DIR}/lib
)
target_link_libraries(lib_engine
    PUBLIC
    raylib
    sc_queue
    sc_map
    sc_array
    m
)
