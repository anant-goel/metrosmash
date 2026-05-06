cmake_minimum_required(VERSION 3.16)
project(MetroSmash VERSION 3.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/MP /W3 /wd4244 /wd4267 /wd4996 /wd4018 /wd4305 /wd4100)
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
    src/AudioManager.cpp
    src/Weapons.cpp
    src/Animation.cpp
    src/Achievements.cpp
    src/GfxShaders.cpp          # ← Advanced graphics pipeline (NEW)
)

# Windows: embed icon + version info from the resource script
if(WIN32)
    list(APPEND SOURCES src/MetroSmash.rc)
endif()

add_executable(MetroSmash WIN32 ${SOURCES})
target_include_directories(MetroSmash PRIVATE src)

if(MSVC)
    target_compile_options(MetroSmash PRIVATE /FI"${CMAKE_SOURCE_DIR}/src/MathFix.h")
endif()

target_link_libraries(MetroSmash
    sfml-graphics
    sfml-window
    sfml-system
    sfml-audio
    OpenGL::GL
    $<$<BOOL:${WIN32}>:sfml-main>   # bridges WinMain → main() for WIN32 subsystem
)

if(EXISTS ${CMAKE_SOURCE_DIR}/assets)
    add_custom_command(TARGET MetroSmash POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:MetroSmash>/assets)
endif()
