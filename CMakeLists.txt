cmake_minimum_required(VERSION 3.16)
project(MetroSmash VERSION 3.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_definitions(
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _USE_MATH_DEFINES
        _CRT_SECURE_NO_WARNINGS
    )
    add_compile_options(/permissive- /utf-8 /W3)
endif()

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
    src/Weapons.cpp
    src/Achievements.cpp
    src/Animation.cpp
    src/AudioManager.cpp
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

if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
    add_custom_command(TARGET MetroSmash POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:MetroSmash>/assets")
endif()
