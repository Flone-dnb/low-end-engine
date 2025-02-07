# Manual

This is a manual - a step-by-step guide to introduce you to various aspects of the engine. More specific documentation can be always found in the class/function/variable documentation in the source code or on this page (see left panel for navigation), every code entity is documented so you should not get lost.

Note that this manual contains many sections and instead of scrolling this page you can click on a little arrow button in the left (navigation) panel, the little arrow button becomes visible once you hover your mouse cursor over a section name, by clicking on that litte arrow you can expand the section and see its subsections and quickly jump to needed sections.

This manual expects that you have a solid knowledge in writing programs using C++ language.

# Introduction to the engine

## Prerequisites

First of all, read the repository's `README.md` file, specifically the `Prerequisites` section, make sure you have all required things installed.

The engine relies on CMake as its build system. CMake is a very common build system for C++ projects so generally most of the developers should be familiar with it. If you don't know what CMake is or haven't used it before it's about the time to learn the basics of CMake right now. Your project will be represented as several CMake targets: one target for standalone game, one target for tests, one target for editor and etc. So you will work on `CMakeLists.txt` files as with usual C++! There are of course some litte differences compared to the usual CMake usage, for example if you want to define external dependencing in your CMake file you need to do one additional step (compared to the usual CMake usage) that will be covered later in a separate section.

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

# Building on ARM64 Linux devices

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
Because of the issue https://github.com/ubuntu/wsl-setup/issues/11 we need an additional step: clear or remove the files:
    - /usr/lib/systemd/system/systemd-binfmt.service.d/wsl.conf
    - /etc/systemd/system/systemd-binfmt.service.d/00-wsl.con
Then reboot and use `wsl --shutdown` just in case.
Then continue:
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
sudo apt install --no-install-recommends build-essential doxygen git wget libdrm-dev python3 python3-pip python3-setuptools python3-wheel ninja-build libopenal-dev premake4 autoconf libevdev-dev ffmpeg libboost-tools-dev libboost-thread-dev libboost-all-dev pkg-config zlib1g-dev libsdl-mixer1.2-dev libsdl1.2-dev libsdl-gfx1.2-dev libsdl2-mixer-dev clang cmake cmake-data libarchive13 libcurl4 libfreetype6-dev librhash0 libuv1 libgbm-dev libsdl-image1.2-dev
rm /usr/lib/aarch64-linux-gnu/libSDL2.*
rm -rf /usr/lib/aarch64-linux-gnu/libSDL2-2.0.so*
```
We removed SDL2 libraries because now we want to install the specific version (tag) of the SDL that we use from the `ext` directory:
```
wget https://github.com/libsdl-org/SDL/archive/refs/tags/release-<version>.tar.gz
tar xfv release-<version>.tar.gz
cd SDL-release-<version>/
./configure --prefix=/usr
make -j8
make install
/sbin/ldconfig
exit
```
Copy your game using the Windows explorer into a new directory at ~/arm64ubuntu/tmp/game (directory inside of the chroot).
Inside of you game directory:
```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DDISABLE_DEV_STUFF=ON ..
cmake --build . --target game
```
Then copy the resulting binary (from `build/OUTPUT/game`) to your ARM64 Linux device.
Inside of your ARM64 Linux device launch the game using some file expolorer or a console.