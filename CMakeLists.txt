cmake_minimum_required(VERSION 3.16)
project(MetroSmash VERSION 3.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SFML 2.6 COMPONENTS graphics window system audio REQUIRED)
find_package(OpenGL REQUIRED)

set(SOURCES
    src/main.cpp
    src/Game.cpp
    src/World.cpp
    src/Player.cpp
    src/Vehicle.cpp
    src/Building.cpp
    src/Physics.cpp
    src/Renderer.cpp
    src/UI.cpp
)

add_executable(MetroSmash ${SOURCES})

target_include_directories(MetroSmash PRIVATE src)

target_link_libraries(MetroSmash
    sfml-graphics
    sfml-window
    sfml-system
    sfml-audio
    OpenGL::GL
)

if(EXISTS ${CMAKE_SOURCE_DIR}/assets)
    add_custom_command(TARGET MetroSmash POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:MetroSmash>/assets
    )
endif()
