# low-end engine

This is a cross-platform 3D game engine that targets low-end retro handhelds (such as Anbernic RG35XX H, Powkiddy RGB30 and similar) but the engine also supports desktop Windows and Linux. 

This engine is most suited for small non open world 3D games with low-poly-ish graphics. This engine is small (fast to learn) and intuitive to use.

> Running games made with this engine on `libmali` drivers (which your handheld's OS might use) may cause issues and crashes (for example loading a texture may cause black screen and/or out of memory error) instead prefer to use `panfrost` drivers if your OS provides them. Most of the testing is done on Rocknix which provides an option to change used driver in the settings.

# Roadmap

- [X] Node system (Godot-like ECS alternative)
- [X] Handling user input events
- [X] Config management (progress, settings, etc.)
- [X] Type reflection
- [X] Forward renderer using OpenGL ES 3.1
- [X] Dynamic light sources
- [X] GLTF/GLB import
- [X] Profiler
- [X] Post-processing effects
- [X] GUI
- [X] Audio (using [SFML](https://github.com/SFML/SFML))
- [X] Simple editor
- [X] Physics engine (using [Jolt](https://github.com/jrouwe/JoltPhysics))
- [ ] Skeletal animations
- [ ] Occlusion culling using portals
- [ ] Dynamic shadows
- [ ] Instancing
- [ ] AI and pathfinding
- [ ] Particle effects

# Documentation

Documentation for this engine consists of 2 parts: API reference (generated from C++ comments) and the manual (at `docs/Manual.md`), we use Doxygen to generate documentation in the HTML format, it includes both API reference and the manual (copied from `docs/Manual.md`).

If you're are game developer you generally don't need Doxygen, the only thing that you need is the manual, you can read it at `docs/Manual.md`.

Because our Doxygen is configured to turn warnings into errors any missing documentation will make Doxygen fail with an error. We don't run Doxygen locally but instead have it in the CI to monitor in case we missed some docs. If you want to generate documentation locally you need to execute the `doxygen` command while being in the `docs` directory. Generated documentation will be located at `docs/gen/html`, open the `index.html` file from this directory to view the documentation.

# Prerequisites

- Compiler that supports C++20
- [CMake](https://cmake.org/download/)

Optional but highly recommended:
- [LLVM](https://github.com/llvm/llvm-project/releases/latest) for clang-format

# Project manager

In order to create new games/projects using this engine we use a special project manager. Read the manual to learn more about it.
