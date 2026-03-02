# Marching Cubes Terrain

![terrain_from_3d_noise](https://github.com/maj0m/Marching-Cubes-Terrain/assets/112952510/c0751c2e-c61a-473e-8865-506a7ba823e9)

## Project layout

- `Terrain Generator/` — source code for the terrain app and local headers.
- `libs/FastNoiseLite/` — FastNoiseLite header dependency.
- `scripts/build.sh` — Linux build helper script.

## Linux build

The project now builds with CMake and automatically fetches:

- GLFW
- GLEW (OpenGL loader)
- Dear ImGui

### Prerequisites (Ubuntu/Debian)

```bash
sudo apt-get install -y cmake g++ pkg-config libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev libglew-dev
```

### Build

```bash
./scripts/build.sh Release
```

Binary output:

- `build/Terrain Generator/marching_cubes_terrain`

## Linux Wayland notes (GLEW + GLFW)

On Linux, this project uses GLEW, which initializes extensions through GLX. In
native Wayland mode that can fail with errors like `No GLX display`.

To keep GLEW and avoid switching loaders, the app now requests the GLFW X11
platform at startup (`glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11)`), so it
runs through XWayland when available.

If your session has XWayland enabled (common on KDE/GNOME), launching normally
is enough:

```bash
./build/Terrain\ Generator/marching_cubes_terrain -verbose
```

If needed, ensure `DISPLAY` is present in the environment before launching.
