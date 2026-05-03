cmake_minimum_required(VERSION 3.16)
project(MetroSmash VERSION 3.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(SFML 3 COMPONENTS Graphics Window System Audio REQUIRED)
find_package(OpenGL REQUIRED)

# Source files
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
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Audio
    OpenGL::GL
)

# Copy assets on build
add_custom_command(TARGET MetroSmash POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:MetroSmash>/assets
)
