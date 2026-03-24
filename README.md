# ShockGraph

ShockGraph is a C++23 render-graph layer built on top of [PyroRHI](https://github.com/PyroshockStudios/PyroRHI). It provides task-based scheduling for graphics, compute, transfer, and custom command-buffer work, plus a resource manager for buffers, images, pipelines, render targets, swap chains, and shader views.

The repository also includes `SGVisualTests`, a small sample application that exercises the graph with rendering features such as MSAA, blending, geometry/tessellation shaders, UAV writes, and ray queries.

## Features

- Task graph API for graphics, compute, transfer, and custom tasks
- Automatic dependency tracking through declared buffer, image, and acceleration-structure usage
- Resource management for persistent GPU resources and transient view objects
- Swap-chain integration, including optional `PyroPlatform` window abstraction support
- GPU timing queries for individual tasks, graph execution, and per-frame flushes
- Visual test application for validating behavior across supported RHIs

## Repository Layout

- `ShockGraph/`: core library sources
- `VisualTests/`: sample application and test scenes
- `resources/`: Slang shaders and shared shader includes
- `vendor/`: dependency entry points for `PyroRHI` and `PyroPlatform`
- `.github/`: CI configuration

## Dependencies

ShockGraph expects the following dependencies:

- CMake 3.28 or newer
- A C++23-capable compiler
- [PyroRHI](https://github.com/PyroshockStudios/PyroRHI)
- [PyroPlatform](https://github.com/PyroshockStudios/PyroPlatform) when building windowed visual tests or using the platform-backed swap-chain path
- A recent Vulkan SDK when building Vulkan-backed configurations

The repository is configured to use submodules by default:

```bash
git clone --recursive https://github.com/PyroshockStudios/ShockGraph.git
```

If you already cloned without submodules:

```bash
git submodule update --init --recursive
```

`SGVisualTests` downloads the Slang SDK at configure time, so CMake needs network access when that target is enabled.

## CMake Options

| Option | Default | Description |
| --- | --- | --- |
| `SHOCKGRAPH_BUILD_VISUAL_TESTS` | `OFF` | Build the `SGVisualTests` executable |
| `SHOCKGRAPH_SHARED_LIBRARY` | `OFF` | Build ShockGraph as a shared library |
| `SHOCKGRAPH_USE_PYRO_PLATFORM` | `ON` | Use `PyroPlatform` for swap-chain/window integration |

Enabling `SHOCKGRAPH_BUILD_VISUAL_TESTS` forces `SHOCKGRAPH_USE_PYRO_PLATFORM=ON`.

Additional RHI backend options such as `PYRO_RHI_BUILD_VULKAN` or `PYRO_RHI_BUILD_DX12` are provided by `PyroRHI`.

## Build

### Linux

For Vulkan-based builds, install a recent Vulkan SDK. For `SGVisualTests`, X11 keyboard support is also required:

```bash
sudo apt-get install libxkbcommon-dev
```

Configure and build:

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DPYRO_COMMON_SHARED_LIBRARY=ON \
  -DPYRO_PLATFORM_SHARED_LIBRARY=ON \
  -DPYRO_RHI_BUILD_VULKAN=ON \
  -DSHOCKGRAPH_SHARED_LIBRARY=ON \
  -DSHOCKGRAPH_USE_PYRO_PLATFORM=ON \
  -DSHOCKGRAPH_BUILD_VISUAL_TESTS=ON

cmake --build build --config Release
```

### Windows

Example Visual Studio generator build with Vulkan enabled:

```powershell
cmake -S . -B build `
  -G "Visual Studio 17 2022" `
  -A x64 `
  -DPYRO_RHI_BUILD_VULKAN=ON `
  -DSHOCKGRAPH_BUILD_VISUAL_TESTS=ON

cmake --build build --config Release
```

### macOS

The project has CI coverage for macOS. Use the same CMake flow as above with a generator available on your machine and the `PyroRHI` backend configuration you want to test.

## Running Visual Tests

When `SHOCKGRAPH_BUILD_VISUAL_TESTS=ON`, the `SGVisualTests` executable is produced in `build/bin`.

The current test app exposes these controls:

- `+`: next RHI
- `-`: previous RHI
- `>`: next test
- `<`: previous test
- `P`: print task timings
- `R`: reload the active test

The visual-test suite currently registers examples for:

- Hello triangle and textured rendering
- Vertex, index, instance, uniform, and update-buffer workflows
- Blending, alpha-to-coverage, wireframe, and MSAA
- Geometry and tessellation shaders
- Compute UAV usage
- Blit operations
- Ray query tests

## Basic Usage

The core workflow is:

1. Create a `TaskResourceManager` from your `PyroRHI` context and device.
2. Allocate persistent resources such as buffers, images, pipelines, and targets.
3. Create a `TaskGraph`.
4. Add tasks and declare all resource usage inside `SetupTask()`.
5. Build the graph once the task set is ready.
6. Execute `BeginFrame()`, `Execute()`, and `EndFrame()` every frame.

Minimal example:

```cpp
#include <ShockGraph/TaskGraph.hpp>
#include <ShockGraph/TaskResourceManager.hpp>

using namespace PyroshockStudios::Renderer;

TaskResourceManager resources({
    .rhi = rhiContext,
    .device = device,
    .framesInFlight = 2,
});

TaskGraph graph({
    .resourceManager = &resources,
});

auto colorImage = resources.CreatePersistentImage({
    .format = Format::RGBA8Unorm,
    .size = { width, height, 1 },
    .usage = ImageUsageFlagBits::COLOR_TARGET | ImageUsageFlagBits::SHADER_RESOURCE,
    .name = "Color",
});

auto colorTarget = resources.CreateColorTarget({
    .image = colorImage,
    .slice = {},
    .name = "MainColor",
});

graph.AddTask(new GraphicsCallbackTask(
    {
        .name = "Main Pass",
    },
    [colorTarget](GraphicsTask& task) {
        task.BindColorTarget({
            .target = colorTarget,
            .clear = ColorClearValue{ 0.1f, 0.1f, 0.1f, 1.0f },
        });
    },
    [](TaskCommandList& cmd) {
        // Encode draw calls here.
    }));

graph.Build();

graph.BeginFrame();
graph.Execute();
graph.EndFrame();
```

See `VisualTests/` for end-to-end examples that build real pipelines, compile Slang shaders, and present through a swap chain.

## Integration Notes

- The main CMake target is `ShockGraph::ShockGraph`.
- Public headers are exposed from the repository root include path, for example `#include <ShockGraph/TaskGraph.hpp>`.
- The library links against `PyroRHI::PyroRHI`, and optionally `PyroPlatform::PyroPlatform`.
- Output binaries are placed under `build/bin`, libraries under `build/lib`.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE).
