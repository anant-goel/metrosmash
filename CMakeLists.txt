cmake_minimum_required(VERSION 3.16)
project(MetroSmash VERSION 3.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── MSVC-specific flags ───────────────────────────────────────────────────────
if(MSVC)
    # /MP  = parallel compilation
    # /W3  = moderate warnings
    # /wd4244 /wd4267 /wd4996 = suppress common SFML/miniaudio noise
    add_compile_options(/MP /W3 /wd4244 /wd4267 /wd4996 /wd4018 /wd4305)

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

add_custom_command(TARGET MetroSmash POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:sfml-graphics>
        $<TARGET_FILE:sfml-window>
        $<TARGET_FILE:sfml-system>
        $<TARGET_FILE:sfml-audio>
        $<TARGET_FILE_DIR:MetroSmash>
)

# Copy assets next to the exe after build
if(EXISTS ${CMAKE_SOURCE_DIR}/assets)
    add_custom_command(TARGET MetroSmash POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:MetroSmash>/assets
    )
endif()

# Copy required DLLs on Windows (vcpkg handles this, but belt-and-suspenders)
if(WIN32)
    add_custom_command(TARGET MetroSmash POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Build complete: MetroSmash.exe"
    )
endif()
