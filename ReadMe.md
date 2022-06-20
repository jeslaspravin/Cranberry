# Cranberry Game Engine

**Cranberry Game Engine** is my Personal hobby game engine. I work on this as best as I could during my holidays.

## Instructions

* Install CMake
* Run GenerateProject.bat to generate Visual Studio solution with one of following presets(eg., `GenerateProject.bat` or `GenerateProject.bat <preset> [Some Cmake arguments]..`)
Note that if using custom preset and not going to change dependencies run Setup.bat first
    - `Engine-StaticLinked` preset creates command `cmake -B StaticBuild -G "Visual Studio 16 2019" -A x64 -Thost=x64 -DCMAKE_INSTALL_PREFIX:STRING=${sourceDir}/Installed -Dengine_static_modules:BOOL=ON`
    - `Engine-DynamicLinked` preset creates command `cmake -B Build -G "Visual Studio 16 2019" -A x64 -Thost=x64 -DCMAKE_INSTALL_PREFIX:STRING=${sourceDir}/Installed -Dengine_static_modules:BOOL=OFF`
    - Hidden preset not available `BasePreset` preset creates command `cmake -B Build -DCMAKE_INSTALL_PREFIX:STRING=${sourceDir}/Installed -Dengine_static_modules:BOOL=OFF`
    - User preset not available under *CMakePresets.json* `Engine-WindowsDebug` creates command `cmake -B Build -G "Visual Studio 16 2019" -A x64 -Thost=x64 -DCMAKE_INSTALL_PREFIX:STRING=${sourceDir}/Installed -Dengine_static_modules:BOOL=OFF -Dllvm_install_path:PATH="D:/Workspace/VisualStudio/CppLibs/llvm/Debug"`
## Third parties
Licenses for third party packages used is placed under `Licenses` folder
### Current dependencies:
* **`SPIRVCross`**(Few necessary files are copied directly rather than using as include)
* **`ImGui`** and **`ImPlot`**(Embedded into the project)
* **`Vulkan`** headers
* **`glm`**
* **`tinyObjloader`**
* **`stb`**
* **`LLVM`**(**`CLang`**) CMake configs generated required some manual tweaking in LLVM project to support Debug,Development and Release config
* **`mimalloc`** CMake configs generated required some manual tweaking in mimalloc project to support Debug,Development and Release config
* **`xxHash`**(Embedded into the project)
* **`CoPaT`**(Embedded into the project)