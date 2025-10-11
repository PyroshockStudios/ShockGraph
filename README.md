## ShockGraph Render Graph abstraction (PyroshockStudios © 2025)

[![Windows MSVC](https://github.com/PyroshockStudios/ShockGraph/actions/workflows/cmake-windows-msvc.yml/badge.svg)](https://github.com/PyroshockStudios/ShockGraph/actions/workflows/cmake-windows-msvc.yml)
[![Linux GCC & Clang](https://github.com/PyroshockStudios/ShockGraph/actions/workflows/cmake-linux.yml/badge.svg)](https://github.com/PyroshockStudios/ShockGraph/actions/workflows/cmake-linux.yml)
[![macOS ARM/x86](https://github.com/PyroshockStudios/ShockGraph/actions/workflows/cmake-macos.yml/badge.svg)](https://github.com/PyroshockStudios/ShockGraph/actions/workflows/cmake-macos.yml)

*Based on the Pyroshock Render Hardware Interface*

### Linux Build Steps
On linux, building the project only requires the vulkan SDK (latest 1.3 or newer)
For building the visual tests with *PyroPlatform*, X11 is used, and the following package is required:
`sudo apt-get install libxkbcommon-dev`

Finally for building the project + tests
```
cmake -B build -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release -DPYRO_COMMON_SHARED_LIBRARY=ON -DPYRO_PLATFORM_SHARED_LIBRARY=ON -DPYRO_RHI_BUILD_VULKAN=ON -DSHOCKGRAPH_SHARED_LIBRARY=ON -DSHOCKGRAPH_USE_PYRO_PLATFORM=ON -DSHOCKGRAPH_BUILD_VISUAL_TESTS=ON -DSHOCKGRAPH_BUILD_RENDERGRAPH_TESTS=ON -S .
cd build
make
```
> -DSHOCKGRAPH_USE_PYRO_PLATFORM=ON ` can be ommitted if not using *PyroPlatform*.