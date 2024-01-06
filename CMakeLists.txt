set(PROJECT_NAME slojam)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_COMPILER clang)
set(CMAKE_C_FLAGS "-Wall -Wextra")
cmake_minimum_required(VERSION 3.22.1)
project(${PROJECT_NAME} C)
set(CMAKE_C_STANDARD 99)
set(RAYLIB_DIR /usr/local/lib CACHE FILEPATH "directory to Raylib")

if (EMSCRIPTEN)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DPLATFORM_WEB")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s ASYNCIFY  -s TOTAL_MEMORY=33554432 -s TOTAL_STACK=2097152 --preload-file ./res --shell-file ./in.html")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif ()


add_subdirectory(engine)
add_subdirectory(scenes)
set(GAME_LIBS
    lib_scenes
)

add_executable(${PROJECT_NAME}
    main.c
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

if (NOT EMSCRIPTEN)
target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address -gdwarf-4)
target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address -gdwarf-4)
endif ()

target_link_libraries(${PROJECT_NAME}
    ${GAME_LIBS}
)
