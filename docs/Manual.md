# Manual

This is a manual - a step-by-step guide to introduce you to various aspects of the engine. More specific documentation can be always found in the class/function/variable documentation in the source code or on this page (use side panel for navigation), every code entity is documented so you should not get lost.

This manual expects that you have a solid knowledge in writing programs using C++ language.

# Introduction to the engine

## Prerequisites

First of all, read the repository's `README.md` file, specifically the `Prerequisites` section, make sure you have all required things installed.

The engine relies on CMake as its build system. CMake is a very common build system for C++ projects so generally most of the developers should be familiar with it. If you don't know what CMake is or haven't used it before it's about the time to learn the basics of CMake right now. Your project will be represented as several CMake targets: one target for standalone game, one target for tests, one target for editor and etc. So you will work on `CMakeLists.txt` files as with usual C++!

## Project manager

See our project manager: https://github.com/Flone-dnb/low-end-engine-project-manager

Note for Visual Studio users:
> If you use Visual Studio the proper way to work on `CMakeLists.txt` files as C++ projects is to open up Visual Studio without any code then press `File` -> `Open` -> `Cmake` and select the `CMakeLists.txt` file in the root directory (may be changed in the new VS versions). Then a tab called `CMake Overview Pages` should be opened in which you might want to click `Open CMake Settings Editor` and inside of that change `Build root` to just `${projectDir}\build\` to store built data inside of the `build` directory (because by default Visual Studio stores build in an unusual path `out/<build mode>/`). When you open `CMakeLists.txt` in Visual Studio near the green button to run your project you might see a text `Select Startup Item...`, you should press a litte arrow near this text to expand a list of available targets to use. Then select a target that you want to build/use and that's it, you are ready to work on the project.

Note for Windows users:
> Windows 10 users need to run their IDE with admin privileges when building the project for the first time (only for the first build) when executing a post-build script the engine creates a symlink next to the built executable that points to the directory with engine/editor/game resources (called `res`). Creating symlinks on Windows requires admin privileges. When releasing your project we expect you to put an actual copy of your `res` directory next to the built executable but we will discuss this topic later in a separate section.

## Build mode

By default we recommend you to use "RelWithDebInfo" build mode instead of the "Debug" build mode in order to improve performance of deserialization and level loading so that there's less time to wait until your game/level is loaded during development. When releasing your game you will use the usual "Release" build mode.

In case you want to debug a specific target's code (your game's code for example) you need to disable optimizations for that specific target but since you often don't want to commit such changes to your version control you can add the following to your game's `CMakeLists.txt` file:

```Cpp
# Debug helper.
if (NOT IS_RELEASE_BUILD AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/debug.cmake")
    message(STATUS "Disabling optimizations because the file \"debug.cmake\" exists.")
    if (MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE -Od)
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE -O0)
    endif()
endif()
```

the paths `src/*/debug.cmake` are already in our gitignore.

## Which header files to include and which not to include

### General

`src/engine_lib` is divided into 2 directories: public and private. You are free to include anything from the `public` directory, you can also include header files from the `private` directory in some special/advanced cases but generally there should be no need for that. Note that this division (public/private) is only conceptual, your project already includes both directories because some engine `public` headers use `private` headers and thus both included in cmake targets that use the engine (in some cases it may cause your IDE to suggest lots of headers when attempting to include some header from some directory so it may be helpful to just look at the specific public directory to look for the header name if you feel overwhelmed).

Inside of the `public` directory you will see other directories that group header files by their purpose, for example `io` directory stands for `in/out` which means that this directory contains files for working with disk (loading/saving configuration files, logging information to log files that are stored on disk, etc.).

You might also notice that some header files have `.h` extension and some have `.hpp` extension. The difference here is only conceptual: files with `.hpp` extension don't have according `.cpp` file, this is just a small hint for developers.

You are not required to use the `private`/`public` directory convention, moreover directories that store executable cmake targets just use `src` directory (you can look into `engine_tests` or `editor` directories to see an example) so you can group your source files as you want.

### Math headers

The engine uses GLM (a well known math library, hosted at https://github.com/g-truc/glm). Although you can include original GLM headers it's highly recommended to include the header `math/GLMath.hpp` instead of the original GLM headers when math is needed. This header includes main GLM headers and defines engine specific macros so that GLM will behave as the engine expects.

You should always prefer to include `math/GLMath.hpp` instead of the original GLM headers and only if this header does not have needed functionality you can include original GLM headers afterwards.

## Code formatting

The engine uses `clang-format` for formatting the code.

`clang-format` can be used in your IDE to automatically format your code (for example) each time you press Ctrl+S. If you want to make sure that your IDE is using our `.clang-format` config you can do the following check: in your source code create 2 or more consecutive empty lines, since our `.clang-format` config contains a rule `MaxEmptyLinesToKeep: 1` after you format the file only 1 empty line should remain. The action with which you format your source code depends on your IDE settings that you might want to configure, generally IDEs have a shortcut to "format" your source code but some have option to automatically use "format" action when you are saving your file.

## GUI applications

In case you would want to create GUI-only applications (not games) using this engine there are some special settings that you might find useful:

1. ENGINE_UI_ONLY cmake option - when enabled the engine only focuses on rendering UI elements and does not create resources used to render 3D geometry or process physics, this can reduce RAM usage for simple GUI applications. In order to enable this option for your project (instead of passing -DENGINE_UI_ONLY=ON every time) ) in the `engine_settings.cmake` file (which is located next to the top-level `CMakeLists.txt` file of the engine) add `set(ENGINE_UI_ONLY ON)`.
2. Render only after user input - when running your application's window using `Window::processEvents` specify `true` to make sure new frames are rendered only after the user input was received or if some period of time has passed, this can reduce CPU usage.

Please note that this engine is not a GUI toolkit, it's not desined to create complicated GUI applications. Although you can create GUI-only applications with this engine the GUI is expected to be very simple.

## Node system

Have you used Godot game engine? If the answer is "yes" you're in luck, this engine uses a similar node system for game entities. We have a base class `Node` and derived nodes like `SpatialNode`, `MeshNode` and etc.

If you don't know what node system is or haven't used Godot, here is a small introduction to our node system:
> Generally game engines have some sort of ECS (Entity component system) in use. There are various forms of ECS like data-driven, object-oriented and etc. Entities can be represented in different ways in different engines: an entity might be a complex class like Unreal Engine's `Actor` that contains both data and logic or an entity might be just a number (a unique identifier). Components can also be represented in different ways: a component might be a special class that implements some specific logic like Unreal Engine's `CharacterMovementComponent` that implements functionality for your entity to be able to move, swim, fly and etc. so it contains both data and logic or a component may just group some data and have no logic at all. Node system is an ECS-like system that is slightly similar to how Unreal Engine's entity-component framework works. If you worked with Unreal Engine imagine that there are no actors but only components and world consists of only components - that's roughly how node system works. In node system each entity in the game is a node. A node contains both logic and data. The base class `Node` implements some general node functionality such as: being able to attach child nodes or attach to some parent node, determining if the node should receive user input or if it should be called every frame to do some per-frame logic and etc. We have various types of nodes like `SpatialNode` that has a location/rotation/scale in 3D space or `MeshNode` that derives from `SpatialNode` but adds functionality to display 3D geometry. Nodes attach other nodes as child nodes thus creating a node hierarchy or a node tree. A node tree is a game level or a game map. Your game's character is also a node tree because it will most likely be built of a mesh node, a camera node, a collision node, maybe your custom derived node that handles some character specific logic and maybe something else. Your game's UI is also a node tree because it will most likely have a container node, various button nodes and text nodes. When you combine everything together you attach your character's node tree and your UI node tree to your game map's node tree thus creating a game level (which is just a big node tree). That's how our node system works.

Nodes are generally used to take place in the node hierarchy, to be part of some parent node for example but not everything should be a node. If you want to implement some class that does not need to take a place in the node hierarchy, does not belong to some node or does not interact with nodes, for example a class to send bug reports, there's really no point in deriving your bug reporter class from `Node`, although nobody is stopping you from using nodes for everything and it's perfectly fine to do so, right now we want to smoothly transition to other important thing in your game. Your object might exist apart from node system and you can use it outside of the node system. For example, you can store your object in a `GameInstance`.

Note that this engine does not have a garbage collector so nodes have a specific ownership rule: parent nodes own child nodes (using `std::unique_ptr`) and child nodes reference parent nodes using raw pointers. You are free to move nodes in the hierarchy (even at runtime) the nodes will handle the ownership change. You will see how this works in more details in one of the next sections.

## Game instance

> If you used Unreal Engine the `GameInstance` class works similarly (but not exactly) to how Unreal Engine's `UGameInstance` class works.

In order to start your game you create a `Window`, a window creates `GameManager` - the heart of your game, the most important manager of your game, but generally you don't interact with `GameManager` directly since it's located in the `src/engine_lib/private` directory, the `GameManager` creates essential game objects such as renderer, physics engine, audio manager and your `GameInstance`. While the window that your have created is not closed the `GameInstance` lives and your game runs. Once the user closes your window or your game code submits a "close window" command the `GameManager` starts to destroy all created systems: `World` (which despawns and destroys all nodes), `GameInstance`, renderer and other things.

When `GameInstance` is created and everything is setup for the game to start `GameInstance::onGameStarted` is called. In this function you generally create/load a game world and spawn some nodes. Generally there is no need to store pointers to nodes in your `GameInstance` since nodes are self-contained and each node knows what to do except that nodes sometimes ask `GameInstance` for various global information by using the static function `Node::getGameInstance`.

So "global" (world independent) classes/objects are generally stored in `GameInstance` but sometimes they can be represented by a node.

## Error handling

The engine uses the `Error` class (`misc/Error.h`) to handle and propagate errors. Some engine functions return it and you should know how to use them.

The `Error` class stores 2 things:
  - initial error message (description of what went wrong)
  - "error call stack" (not an actual call stack, see below)

When you construct an `Error` object it will use `std::source_location` to capture the name of the source file it was constructed from, plus a line where this `Error` object was constructed.

After constructing an `Error` object you can use `Error::addEntry` to capture the name of the source file this function is called from, plus a line where this function was called from.

To get the string that contains the initial error message and all captured source "entries" you can use `Error::getFullErrorMessage`.

To show a message box with the full error message and additionally print it to the log you can use `Error::showError`.

Let's see how this works together in an example of window creation:

```Cpp
// inside of your main.cpp

auto result = WindowBuilder().title("my game").fullscreen().build();
if (std::holds_alternative<Error>(result)) {
    // An error occurred while creating the window.
    Error error = std::get<Error>(std::move(result));
    error.addCurrentLocationToErrorStack(); // add this line to the error stack

    // Since here we won't propagate the error up (because we are in the `main.cpp` file) we show an error message.
    // If we want to propagate the error up we can return the error object after using the `Error::addCurrentLocationToErrorStack`.
    error.showErrorAndThrowException();
}

// Window was created successfully.
const std::unique_ptr<Window> pMainWindow = std::get<std::unique_ptr<Window>>(std::move(result));
```

## Logging

The engine has a simple `Logger` class (`io/Logger.h`) that you can use to write to log files and to console (note that logging to console is disabled in release builds).

On Windows log files are located at `%%localappdata%/low-end-engine/*yourtargetname*/logs`.
On Linux log files are located at `~/.config/low-end-engine/*yourtargetname*/logs`.

Here is an example of `Logger` usage:

```Cpp
#include "misc/Logger.h"

void foo(){
    Logger::get().info("some information");
    Logger::get().warn("some warning");
    Logger::get().error("some error");
}
```

You can also combine `std::format` with logger:

```Cpp
#include "misc/Logger.h"
#include <format>

void bar(){
    int iAnswer = 42;
    Logger::get().info(std::format("the value of the variable is {}", iAnswer));
}
```

Then your log file might look like this:

```
[16:14:49] [info] [MyFile.cpp, 31] some information
[16:14:49] [warning] [MyFile.cpp, 32] some warning
[16:14:49] [error] [MyFile.cpp, 33] some error
[16:14:49] [info] [MyFile.cpp, 46] the value of the variable is 42
```

As you can see each log entry has a file name and a line number included.

Looking at your logs and finding if there were any warnings/errors might be tiresome if you have a big game with lots of systems (even if you just use Ctrl+F), to instantly identify if your game had warnings/errors in the log when you close your game the last log entry will be summary of total warnings/errors logged (if there was any, if there was no warnings/errors nothing will be logged in the end), it might look like this:

```
[16:14:50] [info]
---------------------------------------------------
Total WARNINGS produced: 1.
Total ERRORS produced: 1.
---------------------------------------------------
```

You will see this message in the console and in the log files as the last message if there were any warnings and/or errors logged. So pay attention to the logs/console after closing your game.

## Project paths

In order for your game to be able to access files in the `res` directory when you build your project the engine creates symlinks to the `res` directory next to the build binaries of you game. This means that if you need to access some file from the `res` directory, in your app you can just type `"res/game/somepath"`. For release builds the engine will not create symlinks and will require you to copy your `res` directory manually but we will talk about this in more details in one of the next sections.

Instead of hardcoding you paths like `"res/game/somepath"` you should use `ProjectPaths` (`misc/ProjectPaths.h`). This class provides static functions to access various paths, here are a few examples:

```Cpp
// same as "res/game/myfile.png"
const auto absolutePathToMyFile = ProjectPaths::getPathToResDirectory(ResourceDirectory::GAME) / "myfile.png";

// same as "%localappdata%/low-end-engine/*targetname*/logs" on Windows
const auto pathToLogsDir = ProjectPaths::getPathToLogsDirectory();

// same as "%localappdata%/low-end-engine/*targetname*/progress" on Windows
const auto pathToLogsDir = ProjectPaths::getPathToPlayerProgressDirectory();
```

See `misc/ProjectPaths.h` for more.

## Memory leak checks

### Non-release builds

By default in non-release builds memory leak checks are enabled, look for the output/debugger output tab of your IDE after running your project. If any leaks occurred it should print about them. You can test whether the memory leak checks are enabled or not by doing something like this:
```Cpp
new int(42);
// don't `delete`
```
run your program that runs this code and after your program is finished you should see a message about the memory leak in the output/debugger output tab of your IDE.

### About raw pointers

Some engine functions return raw pointers. Generally, when the engine returns a raw pointer to you this means that you should not free/delete it and it is guaranteed to be valid for the (most) time of its usage. For more information read the documentation for the functions you are using.

## Creating a new world

### World axes and world units

The engine uses a left handed coordinate system. +X is world "forward" direction, +Y is world "right" direction and +Z is world "up" direction. These directions are stored in `Globals::WorldDirection` (`misc/Globals.h`).

Rotations are applied in the following order: ZYX, so "yaw" is applied first, then "pitch" and then "roll". If you need to do math with rotations you can use `MathHelpers::buildRotationMatrix` that builds a rotation matrix with the correct rotation order.

1 world unit is expected to be equal to 1 meter in your game.

### World and node tree

As you might already know a node tree can represent a game level (a game world) but node tree is not considered a game world by itself. The engine has a special object of type `World` it manages things like node spawning, cameras that can render the world and so on. When you give your not spawned node tree to the `World` object (or a path to a node tree file) the `World` creates world-related objects and spawns root node of your tree (which causes all other nodes of the tree to also be spawned). Such spawned node tree is considered a world because it can be rendered (viewed on the screen) and controlled by the user.

### Creating a node tree using the editor

Run the `editor` CMake target and in the content browser (left-bottom panel) right click on the "game" directory (which is your `res/game` directory) and select an option to create a new node tree in the opened context menu. Then left click on the created file to load it.

In the opened node tree you should have only a single node - root node (see top-left panel - node tree inspector). To add more nodes right click on any node in the node tree inspector and select the appropriate option.

Use WASD keys while holding right mouse button to move viewport camera.

### Creating a world using C++

You can create node tree using the editor or the code but a world can only be created using the code. Your generated project should already have a call to `createWorld` in `GameInstance::onGameStarted` which creates a world with a single node - root node. In order to load a node tree as a new world you should call `GameInstance::loadNodeTreeAsWorld`.

After creating the world you would also need a camera to see your world. Here is the most simple to create an active camera after your world is created/loaded:

```Cpp
const auto pCamera = pWorldRootNode->addChildNode(std::make_unique<CameraNode>());
pCamera->makeActive();
```

Default `CameraNode` class does not support input so it will be a static camera (probably located in 0, 0, 0). To be able to control your camera's position/rotation you need to create a new camera node class derived from `CameraNode`. As a starting point look at the editor's `EditorCameraNode` class and how it's implemented, it supports both keyboard+mouse and gamepad but there's not much code so you can easily adapt the code for your game's needs. See one of the next sections in this manual on how to work with user input.

## Handling user input

### Input events

The usual way to handle user input is by binding to action/axis events and doing your processing once these events are triggered.

Action event allows binding buttons that have only 2 states (pressed/not pressed) to an ID. When one of the specified buttons is pressed you will receive an input action event with the specified ID. This way, for example, you can have an action "jump" bound to a "space bar" button.

Axis events operate on pairs of 2 buttons where one triggers an input value `+1.0` and the other `-1.0`. Input devices with "floating" values (such as gamepad thumbsticks/triggers) are a perfect fit this this type of input events. For example, a gamepad thumbstick can be used in 2 axis events: "move forward" and "move right". But you can also bind keyboard buttons to `+1.0` and `-1.0` input, this way you will support both keyboard and gamepad users: an axis event "move forward" will typically have these triggers: "W" button for `+1.0`, "S" button for `-1.0` and gamepad thumbstick axis Y for `+1.0` (up) and `-1.0` (down).

Note
> A so-called "repeat" input is disabled in the engine. "Repeat" input happens when use hold some button, while you hold it the window keeps receiving "repeat" input events with this button but the engine will ignore them and so if you will hold a button only pressed/released events will be received by your code.

Mouse movement is handled using `GameInstance::onMouseMove` function or `Node::onMouseMove` function. There are other mouse related functions like `onMouseScrollMove` that you might find useful.

### Input event IDs

In the next sections you will learn that you can bind to input events in game instance and in nodes. Input events use IDs to be distinguished. This means that you need to have an application-global collection of unique IDs for input events.

You can describe input event IDs of your application in a separate file using enums, for example:

```Cpp
// GameInputEventIds.hpp

#pragma once

/** Stores unique IDs of input events. */
struct GameInputEventIds {
    /** Groups action events. */
    enum class Action : unsigned int {
        CAPTURE_MOUSE_CURSOR = 0, //< Capture mouse cursor.
        INCREASE_CAMERA_SPEED,    //< Increase camera's speed.
        DECREASE_CAMERA_SPEED,    //< Decrease camera's speed.
    };

    /** Groups axis events. */
     enum class Axis : unsigned int {
        MOVE_CAMERA_FORWARD = 0, //< Move camera forward/back.
        MOVE_CAMERA_RIGHT,       //< Move camera right/left.
        MOVE_CAMERA_UP,          //< Move camera up/down.
    };
};
```

This file will store all input IDs that your game needs, even if you have switchable controls like "walking" or "in vehice" all input event IDs should be generally stored like that to make sure they all have unique IDs because all your input events will be registered in the same input manager.

### Registering input events and binding to them

Register your game's input events in your game instance like so:

```Cpp
// Register action event.
auto optionalError = getInputManager()->addActionEvent(
    static_cast<unsigned int>(GameInputEventIds::Action::INCREASE_CAMERA_SPEED),
    {KeyboardButton::LEFT_SHIFT}
);
if (optionalError.has_value()) {
    TODO;
}

// Register axis event.
optionalError = getInputManager()->addAxisEvent(
    static_cast<unsigned int>(GameInputEventIds::Axis::MOVE_CAMERA_FORWARD),
    {{KeyboardButton::W, KeyboardButton::S}},
    {GamepadAxis::LEFT_STICK_Y}
);
if (optionalError.has_value()) {
    TODO;
}
```

Note
> You can register/bind input events even when world does not exist or not created yet.

At this point you will be able to trigger registered action/axis events by pressing the specified keyboard/gamepad buttons. Now we need to write functions that will react to these input events. You can create these functions in your game instance or in nodes (the approach is the same). For this example we bind to input events in constructor of our camera node class like so:

```Cpp
{
    // Bind to registered action events.
    auto& mtxActionEvents = getActionEventBindings();
    std::scoped_lock guard(mtxActionEvents.first);

    // Bind to increase movement speed.
    mtxActionEvents
        .second[static_cast<unsigned int>(GameInputEventIds::Action::INCREASE_CAMERA_MOVEMENT_SPEED)] =
        [this](KeyboardModifiers modifiers, bool bIsPressed) {
            if (bIsPressed) {
                currentMovementSpeedMultiplier = speedIncreaseMultiplier;
            } else {
                currentMovementSpeedMultiplier = 1.0F;
            }
        };
}

{
    // Bind to registered axis events.
    auto& mtxAxisEvents = getAxisEventBindings();
    std::scoped_lock guard(mtxAxisEvents.first);

    // Bind to move forward.
    mtxAxisEvents
        .second[static_cast<unsigned int>(GameInputEventIds::Axis::MOVE_CAMERA_FORWARD)] =
        [this](KeyboardModifiers modifiers, float input) {
            lastKeyboardInputDirection.x = input; // example code
        };
}
```

Note
> Although you can bind to registered input events in `GameInstance` it's not really recommended because input events should generally be processed in nodes such as in your character node.

## Loading font

In order to load a font you need to have a .ttf file to load (there is a default .ttf in the res/engine/font). To load the font file you need to do the following at the start of your game:

```Cpp
void MyGameInstance::onGameStarted() {
    getRenderer()->getFontManager().loadFont(
        ProjectPaths::getPathToResDirectory(ResourceDirectory::ENGINE) / "font" / "font.ttf");

    // ... create world, nodes, etc. ...
}
```

## Importing meshes

Note
> We only support import from GLTF/GLB format.

### Export settings in Blender

Usually the only thing that you need to do is to untick the "+Y up" checkbox in "Transform" section (since we use +Z as our UP axis).

### Import in the engine using C++

In order to import your file you need to use `GltfImporter` like so:

```Cpp
#include "io/GltfImporter.h"

auto optionalError = GltfImporter::importFileAsNodeTree(
    "C:\\models\\sword.glb",       // importing GLB as an example, you can import GLTF in the same way
    "game/models",                 // path to the output directory relative `res` (should exist)
    "sword",                       // name of the new directory that will be created (should not exist yet)
    [](std::string_view sState) {
        Logger::get().info(sState);
    });
if (optionalError.has_value()) [[unlikely]] {
    // ... handle error ...
}
```

If the import process went without errors you can then find your imported model(s) in form of a node tree inside of the resulting directory. You can then deserialize that node tree and use it in your game as any other node tree:

```Cpp
// Deserialize node tree.
auto result = Node::deserializeNodeTree(
    ProjectPaths::getPathToResDirectory(ResourceDirectory::GAME) / "models/sword/sword.toml");
if (std::holds_alternative<Error>(result)) [[unlikely]] {
    // ... process errorr ...
}

// Spawn node tree.
const auto pImportedRootNode = getWorldRootNode()->addChildNode(std::get<std::unique_ptr<Node>>(std::move(result)));
```

## Textures and texture filtering

There's no texture import or anything like that, if you want your mesh to have a texture just tell it the path to the texture like so:

```Cpp
auto pCube = std::make_unique<MeshNode>();
pCube->getMaterial().setPathToDiffuseTexture("game/textures/cube.png"); // located at `res/game/...`
```

Of course when you import a GLTF file meshes and textures will be automatically imported (copied) in the `res` directory and meshes that use texture will have a path to a texture saved in the TOML file.

By default the engine uses point filtering for all textures if you want to use linear filtering use the following:

```Cpp
#include "material/TextureManager.h"

void MyGameInstance::onGameStarted() {
    getRenderer()->getTextureManager().setUsePointFiltering(false);

    // ... your game code here ...
}
```

## Post-processing

You can configure post processing parameters such as sky, ambient light color, distance fog and etc. by using the `PostProcessManager` class, you can access it from the camera manager: `Node::getWorldWhileSpawned()->getCameraManager().getPostProcessManager()`.

## Reflection basics

Reflection comes down to writing additional code to your class, this additional code allows other systems to see/analyze your class/objects (see functions/fields).

Let's consider an example: you want to have a save file for your game where you store player's progress. For this you can use `ConfigManager` but we won't cover it in this section instead we will handle the config data using reflection:

```Cpp
// PlayerSaveData.h

// A very simple player save data class.
class PlayerSaveData
{
public:
    void setExperience(unsigned int iExperience) { this->iExperience = iExperience; }
    unsigned int getExperience() const { return iExperience; }

    void setName(const std::string& sName) { this->sName = sName; }
    std::string getName() const { return sName; }

private:
    unsigned int iExperience = 0;
    std::string sName;
};
```

In order to save player's data we need to derive this class from `Serializable` and add a few functions:

```Cpp
// PlayerSaveData.h

#include "io/Serializable.h"

class PlayerSaveData : public Serializable
{
public:
    PlayerSaveData() = default;
    virtual ~PlayerSaveData() override = default;

    static std::string getTypeGuidStatic();
    virtual std::string getTypeGuid() const override;

    static TypeReflectionInfo getReflectionInfo();

    // ... getter/setter for private fields here ...
};
```

Then in the .cpp file implement new functions:

```Cpp
// PlayerSaveData.cpp

#include "nameof.hpp"

namespace {
    constexpr std::string_view sTypeGuid = "a8e1c213-05a4-4615-bf09-4e187772d359" // some random GUID
}
std::string PlayerSaveData::getTypeGuidStatic() { return sTypeGuid.data(); }
std::string PlayerSaveData::getTypeGuid() const { return sTypeGuid.data(); }

TypeReflectionInfo PlayerSaveData::getReflectionInfo() {
    ReflectedVariables variables;

    // TODO

    return TypeReflectionInfo(
        "", // parent GUID, if your parent provides function `T::getTypeGuidStatic()` then use it here
        NAMEOF_SHORT_TYPE(PlayerSaveData).data(), // type name
        []() -> std::unique_ptr<Serializable> { return std::make_unique<PlayerSaveData>(); }, // function to create your object
        std::move(variables)); // reflected variables
}
```

Before we fill the information about reflected variables you need to explicitly register your type, for example in your GameInstance's constructor:

```Cpp
#include "misc/ReflectionTypeDatabase.h"

MyGameInstance::MyGameInstance(Window* pWindow) : GameInstance(pWindow) {
    registerType(PlayerSaveData::getTypeGuidStatic(), PlayerSaveData::getReflectionInfo());
}
```

Now we only need to fill the information about the reflected variables:

```Cpp
TypeReflectionInfo PlayerSaveData::getReflectionInfo() {
    ReflectedVariables variables;

    variables.unsignedInts[NAMEOF_MEMBER(&PlayerSaveData::iExperience).data()] =
        ReflectedVariableInfo<unsigned int>{
            .setter = [](Serializable* pThis, const unsigned int& iNewValue) {
                    reinterpret_cast<PlayerSaveData*>(pThis)->setExperience(iNewValue);
                },
            .getter = [](Serializable* pThis) -> unsigned int {
                return reinterpret_cast<PlayerSaveData*>(pThis)->getExperience();
            }};

    variables.strings[NAMEOF_MEMBER(&PlayerSaveData::sName).data()] =
        ReflectedVariableInfo<std::string>{
            .setter = [](Serializable* pThis, const std::string& sNewValue) {
                    reinterpret_cast<PlayerSaveData*>(pThis)->setName(sNewValue);
                },
            .getter = [](Serializable* pThis) -> std::string {
                return reinterpret_cast<PlayerSaveData*>(pThis)->getName();
            }};

    return TypeReflectionInfo(
        "",
        NAMEOF_SHORT_TYPE(PlayerSaveData).data(),
        []() -> std::unique_ptr<Serializable> { return std::make_unique<PlayerSaveData>(); },
        std::move(variables));
}
```

As you can see all you need is to provide a getter and a setter for a variable. Thanks to this such variables will now be displayed in the editor (if you are changing these values from the editor UI) and they will be serialized and deserialized. To make this section complete let's save and load this player data:

```Cpp
// have `std::unique_ptr<PlayerSaveData> pPlayerSaveData` somewhere, then:

// to save the current state of the player data (and its variables):
auto optionalError = pPlayerSaveData->serialize(
    ProjectPaths::getPathToPlayerProgressDirectory() / "save1", // file will be created/overwritten
    true); // also create a backup file
if (optionalError.has_value()) [[unlikely]] {
    // handle error
}

// then later in order to deserialize:

auto result = Serializable::deserialize<PlayerSaveData>(ProjectPaths::getPathToPlayerProgressDirectory() / "save1");
if (std::holds_alternative<Error>(result)) [[unlikely]] {
    // handle error
}
auto pDeserializedSave = std::get<std::unique_ptr<PlayerSaveData>>(std::move(result));
```

## Game asset file format

### General overview

Most of the game assets are stored in the human-readable `TOML` format. This format is similar to `INI` format but has more features. This means that you can use any text editor to view or edit your asset files if you need to. Note that some assets (like mesh geometry) will be stored in a separate binary file next to the main TOML file that describes a MeshNode.

When you serialize a serializable object (an object that derives from `Serializable`) the general TOML structure will look like this (comments start with #):

```INI
## <unique_id> is an integer, used to globally differentiate objects in the file
## (in case objects have the same type (same GUID)), if you are serializing only 1 object the ID is 0 by default
["<unique_id>.<type_guid>"]       ## section that describes an object with GUID
<field_name> = <field_value>      ## not all fields will have their values stored like that
<field_name> = <field_value>


["<unique_id>.<type_guid>"]       ## some other object
<field_name> = <field_value>      ## some other field
".path_to_original" = <value>     ## keys that start with one dot are "internal attributes" they are used for storing 
                                  ## internal info
"..parent_node_id" = <unique_id>  ## keys that start with two dots are "custom attributes" (user-specified)
                                  ## that you pass into `serialize`, they are used to store additional info
                                  ## `Node` class uses custom attributes to save node hierarchy
```

### Storing only changed fields

In a case where you have serialized some object and then deserialized it, modified and serialized in a different file. In this case only changed variables will be serialized and instead of saving unchanged variables we will save a path to the "original" object to deserialize other variables.

```INI
## res/game/my_new_data.toml

["0.550ea9f9-dd8a-4089-a717-0fe4e351a687"]
iLevel = 3
".path_to_original" = ["game/my_data.toml", "0"]
```

If we open the file `res/game/my_data.toml` we will see something like this:

```INI
## res/game/my_data.toml

["0.test-guid"]
iLevel = 0
iExperience = 50
vAttributes = [32, 22, 31]
```

Now when we deserialize `res/game/my_new_data.toml` we will get and object with the following properties:

```Cpp
iLevel = 3
iExperience = 50
vAttributes = [32, 22, 31]
```

This will work only if the original object was previously deserialized from a file located in the `res` directory and a new object is serialized in a different path but still in the `res` directory (for more info see `Serializable::getPathDeserializedFromRelativeToRes`).

### Referencing external node tree

Imagine you had a serialized node tree (for example a character node tree that has character's mesh, camera and etc.) then you deserialize it and in the engine/editor add it to a node of some other node tree (for example a game level, we'll call it a parent node tree), thus the parent node tree is seeing your previously deserialized node tree (that you attached) as an extrenal node tree.

During the serialization of the node tree that uses an external node tree this external node tree is saved in a special way, that is, only the root node of the external node tree is saved with the parent node tree and the information about external node tree's child nodes is stored as a path to the external node tree file.

This means that when we reference an external node tree, only changes to external node tree's root node will be saved.

## Saving and loading config files

Although you can use reflection and `Serializable` types for saving and loading data you can also use `ConfigManager` for such purposes.

### User's progress data

`ConfigManager` uses `TOML` file format so if you don't know how this format looks like you can search it right now on the Internet. `TOML` format generally looks like `INI` format but with more features.

Here is an example of how you can save and load data using `ConfigManager`:

```Cpp
#include "io/ConfigManager.h"

// Write some data.
ConfigManager manager;
manager.setValue<std::string>("section name", "key", "value", "optional comment");
manager.setValue<bool>("section name", "my bool", true, "this should be true");
manager.setValue<double>("section name", "my double", 3.14159, "this is a pi value");
manager.setValue<int>("section name", "my long", 42); // notice no comment here

// Save to file.
auto optionalError = manager.saveFile(ConfigCategory::SETTINGS, "my file");
if (optionalError.has_value()) {
    // ... handle error ...
}

// -----------------------------------------
// Let's say that somewhere in other place of your game you want to read these values:

ConfigManager manager;
auto optionalError = manager.loadFile(ConfigCategory::SETTINGS, "my file");
if (optionalError.has_value()) {
    // ... handle error ...
}

// Read string.
const auto realString = manager.getValue<std::string>("section name", "key", "default value");
assert(realString == "value");

// Read bool.
const auto realBool = manager.getValue<bool>("section name", "my bool", false);
assert(realBool == true);

const auto realDouble = manager.getValue<double>("section name", "my double", 0.0);
assert(realDouble >= 3.13);

const auto realLong = manager.getValue<int>("section name", "my long", 0);
assert(realLong == 42);
```

As you can see `ConfigManager` is a very simple system for very simple tasks. Generally only primitive types and some STL types are supported, you can of course write a serializer for some STL type by using documentation from https://github.com/ToruNiina/toml11 as `ConfigManager` uses this library under the hood.

`ConfigManager` also has support for backup files and some other interesting features (see documentation for `ConfigManager`).

### User's input settings

`InputManager` allows creating input events and axis events, i.e. allows binding names with multiple input keys. When working with input the workflow for creating, modifying, saving and loading inputs goes like this:

```Cpp
void MyGameInstance::onGameStarted(){
    // On each game start, create action/axis events with default keys.
    const std::vector<std::pair<KeyboardKey, KeyboardKey>> vMoveForwardKeys = {
        std::make_pair<KeyboardKey, KeyboardKey>(KeyboardKey::KEY_W, KeyboardKey::KEY_S),
        std::make_pair<KeyboardKey, KeyboardKey>(KeyboardKey::KEY_UP, KeyboardKey::KEY_DOWN)
    };

    auto optionalError = getInputManager()->addAxisEvent(
        static_cast<unsigned int>(GameInputEventIds::Axis::MOVE_FORWARD),
        vMoveForwardKeys);
    if (optionalError.has_value()){
        // ... handle error ...
    }

    // ... add more default events here ...

    // After we defined all input events with default keys:
    // Load modifications that the user previously saved (if exists).
    constexpr auto pInputFilename = "input"; // filename "input" is just an example, you can use other filename
    const auto pathToFile
        = ConfigManager::getCategoryDirectory(ConfigCategory::SETTINGS) / pInputFilename
            + ConfigManager::getConfigFormatExtension();
    if (std::filesystem::exists(pathToFile)){
        optionalError = getInputManager()->loadFromFile(pInputFilename);
        if (optionalError.has_value()){
            // ... handle error ...
        }
    }

    // Finished.
}

// Later, the user modifies some keys:
optionalError = getInputManager()->modifyAxisEventKey(
    static_cast<unsigned int>(GameInputEventIds::Axis::MOVE_FORWARD),
    oldKey,
    newKey);
if (optionalError.has_value()){
    // ... handle error ...
}

// The key is now modified in the memory (in the input manager) but if we restart our game the modified key will
// be the same (non-modified), in order to fix this:

// After a key is modified we save the modified inputs to the disk to load modified inputs on the next game start.
optionalError = getInputManager()->saveToFile(pInputFilename);
if (optionalError.has_value()){
    // ... handle error ...
}
```

As it was shown `InputManager` can be acquired using `GameInstance::getInputManager()`, so both game instance and nodes (using `getGameInstance()->getInputManager()`) can work with the input manager.

## Using profiler

The engine uses [tracy profiler](https://github.com/wolfpld/tracy), by default it's enabled in non-release builds.

Include `misc/Profiler.hpp` in order to use the profiler macros:

- `PROFILE_FUNC` add in the beginning of your function to track it in the profiler
- `PROFILE_SCOPE(name)` same as above but works only in a scope
- `PROFILE_ADD_SCOPE_TEXT(text, size)` copies the specified string and adds it to the last profiler mark (func or scope).

In release builds these macros will do nothing. Here are some examples:

```Cpp
void MyClass::myFunc() {
    PROFILE_FUNC

    // some code

    for (size_t i = 0; i < iMyObjCount; i++) {
        PROFILE_SCOPE("my scope")
        PROFILE_ADD_SCOPE_TEXT(myObjects[i].sName.c_str(), myObjects[i].sName.size()) // using `std::string sName`

        // some code
    }
}
```

In order to view profiled data you need to build and launch the tracy server app. In the `ext/tracy` directory run the following commands:

```
cmake -B profiler/build -S profiler -DCMAKE_BUILD_TYPE=Release
cmake --build profiler/build --config Release --parallel
```

Then built tracy server will be located at `ext/tracy/profiler/build/Release/tracy-profiler.exe`, open it and connect to your game.

## Writing custom shaders

Materials can use custom GLSL shaders, here is an example of setting a custom fragment shader to a mesh.

```Cpp
pMeshNode->getMaterial().setPathToCustomFragmentShader("game/shaders/myshader.glsl"); // located in the `res/game/...` directory

// then spawn your mesh node
```

See the directory `res/engine/shaders/node` for reference implementation.

## Simulating input for automated tests

Your game has a `..._tests` target for automated testing (which relies on https://github.com/catchorg/Catch2) and generally it will be very useful to simulate user input. Here is an example on how to do that:

```Cpp
// Simulate input.
getWindow()->onKeyboardInput(KeyboardKey::KEY_A, KeyboardModifiers(0), true); // simulate keyboard "A" pressed
getWindow()->onKeyboardInput(KeyboardKey::KEY_A, KeyboardModifiers(0), false); // simulate keyboard "A" released
```

Once such function is called it will trigger register input bindings in your game instance and nodes.

There are also other `on...` function in `Window` that you might find handy in simulating user input.

# Building your game for a retro handhelds (ARM64 Linux devices)

## Setup

This section describes the build process for such devices as Anbernic RG35XX H or similar.

- Based on https://github.com/Cebion/Portmaster_builds

Commands for WSL2 Ubuntu 24.04.1 LTS:
```
sudo apt update && sudo apt upgrade -y
sudo reboot now
```
Wait for console to close, then in Windows console:
```
wsl --shutdown
```
Then back in the Ubuntu:
```
sudo apt install -y build-essential binfmt-support daemonize libarchive-tools qemu-system qemu-user qemu-user-static gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
wget "https://cloud-images.ubuntu.com/releases/24.04/release/ubuntu-24.04-server-cloudimg-arm64-root.tar.xz"
mkdir arm64ubuntu
sudo bsdtar -xpf ubuntu-24.04-server-cloudimg-arm64-root.tar.xz -C arm64ubuntu
```
Because of the issue https://github.com/ubuntu/wsl-setup/issues/11 we need an additional step:
```
sudo rm /usr/lib/systemd/system/systemd-binfmt.service.d/wsl.conf
sudo rm /etc/systemd/system/systemd-binfmt.service.d/00-wsl.con
```
Then use `wsl --shutdown` just in case. Continue:
```
sudo cp /usr/bin/qemu-aarch64-static arm64ubuntu/usr/bin
sudo daemonize /usr/bin/unshare -fp --mount-proc /lib/systemd/systemd --system-unit=basic.target
sudo mount -o bind /dev arm64ubuntu/dev
sudo chroot arm64ubuntu qemu-aarch64-static /bin/bash
rm /etc/resolv.conf
echo 'nameserver 8.8.8.8' > /etc/resolv.conf
exit
mkdir -p arm64ubuntu/tmp/.X11-unix
echo '#!/bin/bash' > chroot.sh
echo 'sudo daemonize /usr/bin/unshare -fp --mount-proc /lib/systemd/systemd --system-unit=basic.target' >> chroot.sh
echo 'sudo mount -o bind /proc/ arm64ubuntu/proc/' >> chroot.sh
echo 'sudo mount --rbind /dev/ arm64ubuntu/dev/' >> chroot.sh
echo 'sudo mount -o bind /tmp/.X11-unix arm64ubuntu/tmp/.X11-unix' >> chroot.sh
echo 'sudo chroot arm64ubuntu qemu-aarch64-static /bin/bash' >> chroot.sh
chmod +x chroot.sh
sudo ./chroot.sh
```
Try `sudo apt update && sudo apt upgrade -y` if you get an error `sudo: unable to resolve host ...` write the hostname that you got in that message in the file `/etc/hostname` (replace the old one) and in the file `/etc/hosts` under the localhost string (use the same 127.0.0.1 address).
```
sudo apt update && sudo apt upgrade -y
sudo apt install --no-install-recommends build-essential git wget libdrm-dev libopenal-dev premake4 autoconf libevdev-dev pkg-config zlib1g-dev cmake cmake-data libarchive13 libcurl4 libfreetype6-dev librhash0 libuv1 libgbm-dev
```

## Steps for each release of your game

Copy your game using the Windows explorer into a new directory at ~/arm64ubuntu/tmp/game (directory inside of the chroot). Then back into the WSL:
```
sudo ./chroot.sh
cd tmp/game
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release --target game
```
Then copy the resulting binary (from `build/OUTPUT/game`) to your ARM64 Linux device. We don't worry about installing SDL2 libraries because we link SDL statically. Inside of your ARM64 Linux device launch the game using some file explorer or a console.
