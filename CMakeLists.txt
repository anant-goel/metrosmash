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
        VC_EXTRA_LEAN
    )
    add_compile_options(
        /permissive-
        /utf-8
        /W3
        # Force pch.h included first in every translation unit.
        # This guarantees <math.h> is seen before <windows.h> from SFML,
        # fixing the cmath/tgammaf/sinf/etc errors on MSVC.
        /FI"${CMAKE_CURRENT_SOURCE_DIR}/src/pch.h"
    )
endif()

find_package(SFML 3 COMPONENTS Graphics Window System Audio REQUIRED)
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
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Audio
    OpenGL::GL
)

add_custom_command(TARGET MetroSmash POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:MetroSmash>/assets)
