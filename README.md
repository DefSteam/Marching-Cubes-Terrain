# Marching Cubes Terrain

![terrain_from_3d_noise](https://github.com/maj0m/Marching-Cubes-Terrain/assets/112952510/c0751c2e-c61a-473e-8865-506a7ba823e9)

## Project layout

- `Terrain Generator/` — source code for the terrain app and local headers.
- `libs/FastNoiseLite/` — FastNoiseLite header dependency.
- `scripts/build.sh` — Linux build helper script.

## Linux build

The project now builds with CMake and automatically fetches:

- GLFW
- GLAD (OpenGL loader, replacing GLEW)
- Dear ImGui

### Prerequisites (Ubuntu/Debian)

```bash
sudo apt-get install -y cmake g++ pkg-config libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev
```

### Build

```bash
./scripts/build.sh Release
```

Binary output:

- `build/Terrain Generator/marching_cubes_terrain`
