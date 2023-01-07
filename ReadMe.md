# Cranberry Game Engine

**Cranberry Game Engine** is my Personal hobby game engine, This is my first from scratch game engine. I work on this as best as I could during my holidays.

Why is `Cranberry` the name? I was eating cranberry and checking Twitter for my daily dose of imposter syndrome. Then suddenly 💡 why not write my engine? I created the project while still eating the berries!

## Requirement
* Vulkan 1.3 supported hardware is required

## Instructions

* Install CMake from [CMake]
* Install Visual Studio 2022 from [VisualStudio]
* Install Vulkan SDK from [VulkanSDK]
* Run GenerateProject.bat to generate Visual Studio solution with one of following presets(eg., `GenerateProject.bat` or `GenerateProject.bat <preset> [Some Cmake arguments]..`)
Note that if using custom preset and not going to change dependencies run Setup.bat first
    - `Editor-DynamicLinked` preset creates command `cmake -B Build -G "Visual Studio 17 2022" -A x64 -Thost=x64 -DCMAKE_INSTALL_PREFIX:STRING=${sourceDir}/Installed -Dengine_static_modules:BOOL=OFF`
    - `Runtime-DynamicLinked` preset creates command `cmake -B RuntimeBuild -G "Visual Studio 17 2022" -A x64 -Thost=x64 -DCMAKE_INSTALL_PREFIX:STRING=${sourceDir}/Installed -Dengine_static_modules:BOOL=OFF -Deditor_build:BOOL=OFF`
    
## Third parties
Licenses for third party packages used is placed under `Licenses` folder
### Current dependencies:
* **`SPIRVCross`**(Few necessary files are copied directly rather than using as include)
* **`ImGui`**(Embedded into the project)
* **`Vulkan`** headers
* **`glm`**
* **`tinyObjloader`**
* **`stb`**
* **`LLVM`**(**`CLang`**) CMake configs generated required some manual tweaking in LLVM project to support Debug,Development and Release config
* **`mimalloc`** CMake configs generated required some manual tweaking in mimalloc project to support Debug,Development and Release config
* **`xxHash`**(Embedded into the project)
* **`CoPaT`**(Embedded into the project)

[//]: # (Below are link reference definitions)
[CMake]: https://cmake.org/download/
[VisualStudio]: https://visualstudio.microsoft.com/downloads/
[VulkanSDK]: https://vulkan.lunarg.com/