assimp
bullet
directx-shader-compiler 1.8.2407-1 for vulkan_dxc lib
shaderc 2024.4-1 glslc
sdl2
freetype2

meshoptimizer-0.22-2 from AUR
compressonator-cli-bin 4.5.52-1 from AUR

sudo btrfs balance start /
sudo btrfs fi usage /

docker buildx build  -f Dockerfile-arch -t building-img:latest .
docker run --memory=8g --cpus=16 --rm -it building-img:latest
docker stats
docker system prune

TODO: pch file in CMAKE
