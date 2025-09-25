conan:
    assimp
    bullet
    shaderc 2024.4-1 glslc
    meshoptimizer-0.22-2 from AUR

directx-shader-compiler 1.8.2407-1 for vulkan_dxc lib
sdl3 - vcpkg
freetype2 - vcpkg
compressonator-cli-bin 4.5.52-1 from AUR

sudo btrfs balance start --full-balance /
sudo btrfs fi usage /
sudo btrfs filesystem df /

docker buildx build  -f Dockerfile-arch -t building-img:latest .
docker run --memory=8g --cpus=16 --rm -it building-img:latest
docker run -d --name my_container building-img:latest
docker cp my_container:/spartan-deps ../third_party/linux/
docker stats
docker system prune
docker container prune -f
docker image prune -a -f
docker builder prune -a -f

include-what-you-use:
iwyu-tool -p . -j $(nproc) > iwyu.out
wyu-fix-includes < iwyu.out

glxinfo | grep "OpenGL renderer"    : OpenGL renderer string: AMD Radeon Graphics (radeonsi, renoir, LLVM 19.1.7, DRM 3.60, 6.13.3-arch1-1)
lspci | grep VGA                    : 07:00.0 VGA compatible controller: Advanced Micro Devices, Inc. [AMD/ATI] Cezanne [Radeon Vega Series / Radeon Vega Mobile Series] (rev d1)
vulkaninfo | grep deviceName        : deviceName        = AMD Radeon Graphics (RADV RENOIR)
Radeon Vega 8 8CUs (GCN 5th generation) 512:32:8 8 CUs Unified shaders : texture mapping units : render output units and compute units (CU)
AMD Ryzen 7 PRO 5850U
https://github.com/PanosK92/SpartanEngine/blob/master/runtime/Core/Debugging.h
    inline static bool m_validation_layer_enabled        = false;
    inline static bool m_gpu_assisted_validation_enabled = false;
    inline static bool m_logging_to_file_enabled         = false;

    gdb segfault investigation
bt full
info sharedlibrary
thread apply all bt

sudo nano /etc/ld.so.conf.d/opt-installed.conf
/opt/installed/lib
sudo ldconfig
