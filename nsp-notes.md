conan:
    assimp
    bullet
    shaderc 2024.4-1 glslc
    meshoptimizer-0.22-2 from AUR

directx-shader-compiler 1.8.2407-1 for vulkan_dxc lib
sdl3 - vcpkg
freetype2 - vcpkg
compressonator-cli-bin 4.5.52-1 from AUR

sudo btrfs balance start /
sudo btrfs fi usage /

docker buildx build  -f Dockerfile-arch -t building-img:latest .
docker run --memory=8g --cpus=16 --rm -it building-img:latest
docker run -d --name my_container my_image
docker cp my_container:/data ./destination
docker stats
docker system prune

https://github.com/PanosK92/SpartanEngine/blob/master/runtime/Core/Debugging.h
    inline static bool m_validation_layer_enabled        = false;
    inline static bool m_gpu_assisted_validation_enabled = false;
    inline static bool m_logging_to_file_enabled         = false;