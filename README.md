# MetroSmash v3.0 — Ultimate Destruction Sandbox
## Build Instructions

### Requirements
- CMake 3.16+
- SFML 2.5+ (graphics, window, system, audio)
- OpenGL (comes with your GPU drivers)
- C++17 compiler (MSVC 2019+, GCC 9+, or Clang 10+)

---

### Windows (Recommended: vcpkg)

**1. Install vcpkg and SFML:**
```bash
git clone https://github.com/microsoft/vcpkg
cd vcpkg
bootstrap-vcpkg.bat
vcpkg install sfml:x64-windows
```

**2. Configure and build:**
```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake -A x64
cmake --build build --config Release
```

**3. Run:**
```
build/Release/MetroSmash.exe
```

---

### Windows (Manual SFML)

1. Download SFML 2.6.x from https://www.sfml-dev.org/download.php (Visual C++ 17 64-bit)
2. Extract to e.g. `C:/SFML`
3. Configure:
```bash
cmake -B build -S . -DSFML_DIR="C:/SFML/lib/cmake/SFML" -A x64
cmake --build build --config Release
```
4. Copy SFML DLLs from `C:/SFML/bin/` next to `MetroSmash.exe`

---

### Linux

```bash
sudo apt install libsfml-dev cmake build-essential
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/MetroSmash
```

---

### macOS

```bash
brew install sfml cmake
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/MetroSmash
```

---

## Controls

| Key | Action |
|-----|--------|
| WASD | Move / Drive |
| Mouse | Look / Camera |
| Space | Jump |
| F | Enter/Exit Vehicle |
| E | Place Explosive |
| Q | Detonate All Explosives |
| R | Rebuild City |
| ESC | Pause |

## Project Structure

```
MetroSmash/
├── CMakeLists.txt
├── src/
│   ├── main.cpp        — Entry point
│   ├── Game.h/.cpp     — Main loop, input handling
│   ├── World.h/.cpp    — Scene management
│   ├── Building.h/.cpp — Destructible buildings
│   ├── Player.h/.cpp   — Player movement, explosives
│   ├── Vehicle.h/.cpp  — Car, Tank, Bulldozer
│   ├── Physics.h/.cpp  — AABB physics, explosions
│   ├── Renderer.h/.cpp — OpenGL rendering
│   ├── Camera.h        — 3rd-person camera
│   ├── UI.h/.cpp       — HUD, menus
│   └── GameMath.h          — Vec3, Mat4 utilities
└── assets/
    └── fonts/          — Put font.ttf here (optional)
```

## Architecture Notes

- **Physics**: Custom AABB rigid body system with explosion force propagation
- **Buildings**: Voxel-style block grid; blocks detach when unsupported
- **Rendering**: Fixed-function OpenGL 2.1 via SFML context
- **Vehicles**: 3 types — Car, Tank (heavy), Bulldozer (max ram damage)
- **Explosives**: Place unlimited markers, detonate all at once with Q
