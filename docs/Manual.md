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