# Manual

This is a manual - a step-by-step guide to introduce you to various aspects of the engine. More specific documentation can be always found in the class/function/variable documentation in the source code or on this page (see left panel for navigation), every code entity is documented so you should not get lost.

Note that this manual contains many sections and instead of scrolling this page you can click on a little arrow button in the left (navigation) panel, the little arrow button becomes visible once you hover your mouse cursor over a section name, by clicking on that litte arrow you can expand the section and see its subsections and quickly jump to needed sections.

This manual expects that you have a solid knowledge in writing programs using C++ language.

# Introduction to the engine

## Prerequisites

First of all, read the repository's `README.md` file, specifically the `Prerequisites` section, make sure you have all required things installed.

The engine relies on CMake as its build system. CMake is a very common build system for C++ projects so generally most of the developers should be familiar with it. If you don't know what CMake is or haven't used it before it's about the time to learn the basics of CMake right now. Your project will be represented as several CMake targets: one target for standalone game, one target for tests, one target for editor and etc. So you will work on `CMakeLists.txt` files as with usual C++!

## Project generator

Currently we don't have a proper project generator but it's planned. Once the project generator will be implemented this section will be updated.

Right now you can clone the repository and don't forget to update submodules. Once you have a project with `CMakeLists.txt` file in the root directory you can open it in your IDE. For example Qt Creator or Visual Studio allow opening `CMakeLists.txt` files as C++ projects, other IDEs also might have this functionality.

Note for Visual Studio users:
> If you use Visual Studio the proper way to work on `CMakeLists.txt` files as C++ projects is to open up Visual Studio without any code then press `File` -> `Open` -> `Cmake` and select the `CMakeLists.txt` file in the root directory (may be changed in the new VS versions). Then a tab called `CMake Overview Pages` should be opened in which you might want to click `Open CMake Settings Editor` and inside of that change `Build root` to just `${projectDir}\build\` to store built data inside of the `build` directory (because by default Visual Studio stores build in an unusual path `out/<build mode>/`). When you open `CMakeLists.txt` in Visual Studio near the green button to run your project you might see a text `Select Startup Item...`, you should press a litte arrow near this text to expand a list of available targets to use. Then select a target that you want to build/use and that's it, you are ready to work on the project.

Note for Windows users:
> Windows 10 users need to run their IDE with admin privileges when building the project for the first time (only for the first build) when executing a post-build script the engine creates a symlink next to the built executable that points to the directory with engine/editor/game resources (called `res`). Creating symlinks on Windows requires admin privileges. When releasing your project we expect you to put an actual copy of your `res` directory next to the built executable but we will discuss this topic later in a separate section.

Before you go ahead and dive into the engine yourself make sure to read a few more sections, there is one really important section further in the manual that you have to read, it contains general tips about things you need to keep an eye out!

## Which header files to include and which not to include

### General

`src/engine_lib` is divided into 2 directories: public and private. You are free to include anything from the `public` directory, you can also include header files from the `private` directory in some special/advanced cases but generally there should be no need for that. Note that this division (public/private) is only conceptual, your project already includes both directories because some engine `public` headers use `private` headers and thus both included in cmake targets that use the engine (in some cases it may cause your IDE to suggest lots of headers when attempting to include some header from some directory so it may be helpful to just look at the specific public directory to look for the header name if you feel overwhelmed).

Inside of the `public` directory you will see other directories that group header files by their purpose, for example `io` directory stands for `in/out` which means that this directory contains files for working with disk (loading/saving configuration files, logging information to log files that are stored on disk, etc.).

You might also notice that some header files have `.h` extension and some have `.hpp` extension. The difference here is only conceptual: files with `.hpp` extension don't have according `.cpp` file, this is just a small hint for developers.

You are not required to use the `private`/`public` directory convention, moreover directories that store executable cmake targets just use `src` directory (you can look into `engine_tests` or `editor` directories to see an example) so you can group your source files as you want.

### Math headers

The engine uses GLM (a well known math library, hosted at https://github.com/g-truc/glm). Although you can include original GLM headers it's highly recommended to include the header `math/GLMath.hpp` instead of the original GLM headers when math is needed. This header includes main GLM headers and defines engine specific macros so that GLM will behave as the engine expects.

You should always prefer to include `math/GLMath.hpp` instead of the original GLM headers and only if this header does not have needed functionality you can include original GLM headers afterwards.

## Automatic code formatters and static analyzers

The engine uses `clang-format` and `clang-tidy` a classic pair of tools that you will commonly find in C++ projects. If you don't know what they do it's a good time to read about them on the Internet.

The engine does not require you to use them but their usage is highly recommended.

`clang-format` can be used in your IDE to automatically format your code (for example) each time you press Ctrl+S. If you want to make sure that your IDE is using our `.clang-format` config you can do the following check: in your source code create 2 or more consecutive empty lines, since our `.clang-format` config contains a rule `MaxEmptyLinesToKeep: 1` after you format the file only 1 empty line should remain. The action with which you format your source code depends on your IDE settings that you might want to configure, generally IDEs have a shortcut to "format" your source code but some have option to automatically use "format" action when you are saving your file.

`clang-tidy` has a lot of checks enabled and is generally not that fast as you might expect, because of this we have `clang-tidy` enabled only in release builds to speed up build times for debug builds. This means that if your game builds in debug mode it may very well fail to build in release mode due to some `clang-tidy` warnings that are treated as errors. Because of this it's highly recommended to regularly (say once or twice a week) build your project in release mode to check for `clang-tidy` warnings/errors.

## Node system

Have you used Godot game engine? If the answer is "yes" you're in luck, this engine uses a similar node system for game entities. We have a base class `Node` and derived nodes like `SpatialNode`, `MeshNode` and etc.

If you don't know what node system is or haven't used Godot, here is a small introduction to the node system:
> Generally game engines have some sort of ECS (Entity component system) in use. There are various forms of ECS like data-driven, object-oriented and etc. Entities can be represented in different ways in different engines: an entity might be a complex class like Unreal Engine's `Actor` that contains both data and logic or an entity might be just a number (a unique identifier). Components can also be represented in different ways: a component might be a special class that implements some specific logic like Unreal Engine's `CharacterMovementComponent` that implements functionality for your entity to be able to move, swim, fly and etc. so it contains both data and logic or a component may just group some data and have no logic at all. Node system is an ECS-like system that is slightly similar to how Unreal Engine's entity-component framework works. If you worked with Unreal Engine imagine that there are no actors but only components and world consists of only components - that's roughly how node system works. In node system each entity in the game is a node. A node contains both logic and data. The base class `Node` implements some general node functionality such as: being able to attach child nodes or attach to some parent node, determining if the node should receive user input or if it should be called every frame to do some per-frame logic and etc. We have various types of nodes like `SpatialNode` that has a location/rotation/scale in 3D space or `MeshNode` that derives from `SpatialNode` but adds functionality to display a 3D geometry. Nodes attach other nodes as child nodes thus creating a node hierarchy or a node tree. A node tree is a game level or a game map. Your game's character is also a node tree because it will most likely be built of a mesh node, a camera node, a collision node, maybe your custom derived node that handles some character specific logic and maybe something else. Your game's UI is also a node tree because it will most likely have a container node, various button nodes and text nodes. When you combine everything together you attach your character's node tree and your UI node tree to your game map's node tree thus creating a game level. That's how node system works.

Nodes are generally used to take place in the node hierarchy, to be part of some parent node for example. Nodes are special and they use special garbage collected smart pointers (we will talk about them in one of the next sections) but not everything should be a node. If you want to implement some class that does not need to take a place in the node hierarchy, does not belong to some node or does not interact with nodes, for example a class to send bug reports, there's really no point in deriving you bug reporter class from `Node`, although nobody is stopping you from using nodes for everything and it's perfectly fine to do so, right now we want to smoothly transition to other important thing in your game. Your class/object might exist apart from node system and you can use it outside of the node system. For example, you can store your class/object in a `GameInstance`.

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

auto result = Window::create("My window");
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

## Saving and loading user's progress data

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

## Saving and loading user's input settings

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

## Simulating input for automated tests

Your game has a `..._tests` target for automated testing (which relies on https://github.com/catchorg/Catch2) and generally it will be very useful to simulate user input. Here is an example on how to do that:

```Cpp
// Simulate input.
getWindow()->onKeyboardInput(KeyboardKey::KEY_A, KeyboardModifiers(0), true); // simulate keyboard "A" pressed
getWindow()->onKeyboardInput(KeyboardKey::KEY_A, KeyboardModifiers(0), false); // simulate keyboard "A" released
```

Once such function is called it will trigger register input bindings in your game instance and nodes.

There are also other `on...` function in `Window` that you might find handy in simulating user input.

# Building on ARM64 Linux devices

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
sudo apt install --no-install-recommends build-essential doxygen git wget libdrm-dev libopenal-dev premake4 autoconf libevdev-dev pkg-config zlib1g-dev cmake cmake-data libarchive13 libcurl4 libfreetype6-dev librhash0 libuv1 libgbm-dev
```

## Steps for each release of your game

Copy your game using the Windows explorer into a new directory at ~/arm64ubuntu/tmp/game (directory inside of the chroot). Then back into the WSL:
```
sudo ./chroot.sh
cd tmp/game
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DDISABLE_DEV_STUFF=ON ..
cmake --build . --target game
```
Then copy the resulting binary (from `build/OUTPUT/game`) to your ARM64 Linux device. We don't worry about installing the SDL2 libraries because in most cases they are already installed the OS your ARM64 device is running. Inside of your ARM64 Linux device launch the game using some file explorer or a console.