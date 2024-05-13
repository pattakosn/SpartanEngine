#if not exists libraries.7z
curl -kLSs "https://www.dropbox.com/scl/fi/2o73yleuxg1z57hium5y0/libraries.7z?rlkey=4jmru1hedj0vrhnoq1xmy9dez&st=mw7gxf7s&dl=1" third_party/libraries/libraries.7z
./build_scripts/7z.exe e third_party/libraries/libraries.7z -othird_party/libraries/ -aoa

#if not exists assets
curl -kLSs "https://www.dropbox.com/scl/fi/hagxxndy0dnq7pu0ufkxh/assets.7z?rlkey=gmwlxlhf6q3eubh7r50q2xp27&st=60lavvyz&dl=1" assets/assets.7z
./build_scripts/7z.exe e assets/assets.7z -oassets/ -aoa

mkdir -p binaries/data; cp -r data binaries/
cp third_party/libraries/{dxcompiler.dll,fmod.dll,fmodL.dll} binaries/

mkdir binaries/project;
cp -r assets/{models,music,terrain,materials} binaries/project/

 ./build_scripts/premake5.exe --file=build_scripts/premake.lua gmake2 vulkan
