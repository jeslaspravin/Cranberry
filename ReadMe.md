#GameEngine
My Personal hobby game engine

##Instructions
* Install CMake
* Run GenerateProject.bat to generate Visual Studio solution with one of following presets(eg., `GenerateProject.bat <preset> [Some Cmake arguments]..`)
    - `Engine-StaticLinked` preset creates command `cmake -B StaticBuild -G "Visual Studio 16 2019" -A x64 -Thost=x64 -DCMAKE_INSTALL_PREFIX:STRING=${sourceDir}/Installed -Dengine_static_modules:BOOL=ON`
    - `Engine-DynamicLinked` preset creates command `cmake -B Build -G "Visual Studio 16 2019" -A x64 -Thost=x64 -DCMAKE_INSTALL_PREFIX:STRING=${sourceDir}/Installed -Dengine_static_modules:BOOL=OFF`
    - Hidden preset not available `BasePreset` preset creates command `cmake -B Build -DCMAKE_INSTALL_PREFIX:STRING=${sourceDir}/Installed -Dengine_static_modules:BOOL=OFF`
    - User preset not available under *CMakePresets.json* `Engine-WindowsDebug` creates command `cmake -B Build -G "Visual Studio 16 2019" -A x64 -Thost=x64 -DCMAKE_INSTALL_PREFIX:STRING=${sourceDir}/Installed -Dengine_static_modules:BOOL=OFF -Dllvm_install_path:PATH="D:/Workspace/VisualStudio/CppLibs/llvm/Debug"`

##Current dependencies:
* Vulkan headers
* glm
* tinyObjloader
* SPIRVCross(Few necessary files are copied directly rather than using as include)
* stb
* ImGui and ImPlot(Embedded into the project)