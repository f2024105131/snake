cmake_minimum_required(VERSION 3.16)
project(snake VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_library(snake_core
    src/Board.cpp
    src/Config.cpp
    src/Game.cpp
    src/HighScore.cpp
    src/InputHandler.cpp
    src/Renderer.cpp
    src/Snake.cpp
    src/Terminal.cpp
)

target_include_directories(snake_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

if(MSVC)
    target_compile_options(snake_core PRIVATE /W4)
else()
    target_compile_options(snake_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(snake src/main.cpp)
target_link_libraries(snake PRIVATE snake_core)

enable_testing()
add_executable(snake_tests tests/test_game_logic.cpp)
target_link_libraries(snake_tests PRIVATE snake_core)
add_test(NAME game_logic COMMAND snake_tests)

install(TARGETS snake RUNTIME DESTINATION bin)
