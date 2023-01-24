# Cranberry Game Engine

**Cranberry Game Engine** is my Personal hobby game engine, This is my first from scratch game engine. I work on this as best as I could during my holidays.

Why is `Cranberry` the name? I was eating cranberry and checking Twitter for my daily dose of imposter syndrome. Then suddenly 💡 why not write my engine? I created the project while still eating the berries!

## Requirement
* Vulkan 1.3 supported hardware is required
* Windows platform

## Instructions

* Install CMake from [CMake]
* Install Visual Studio 2022 from [VisualStudio]
* Install Vulkan SDK from [VulkanSDK]. Necessary because I am using `glslangValidator`
* Run GenerateProject.bat to generate Visual Studio solution. This will by default generate solution for `Editor-DynamicLinked` preset under `Build` folder
* You can also run GenerateProject.bat with one of following presets(eg., `GenerateProject.bat` or `GenerateProject.bat <preset> [Some Cmake arguments]..`)
Note that if using preset and not going to change library dependencies run Setup.bat first(It will download archive with necessary libraries)
    - `Editor-DynamicLinked` preset creates command `cmake -B Build -G "Visual Studio 17 2022" -A x64 -Thost=x64 -DCMAKE_INSTALL_PREFIX:STRING=${sourceDir}/Installed -DCranberry_STATIC_MODULES:BOOL=OFF`
    - `Runtime-DynamicLinked` preset creates command `cmake -B RuntimeBuild -G "Visual Studio 17 2022" -A x64 -Thost=x64 -DCMAKE_INSTALL_PREFIX:STRING=${sourceDir}/Installed -DCranberry_STATIC_MODULES:BOOL=OFF -DCranberry_EDITOR_BUILD:BOOL=OFF`
    

## Code lookups
Right now I have two runtime applications `Cranberry` and `TestEngine`

* For `Cranberry` start from [Cranberry-Main]
    - Right now the scenes can be manually loaded using this line of code [CranberryAppLoadScene] at startup of the engine
    - If engine cannot find any valid `obj` file then it auto creates a default scene with bunch of cubes like in the [Screen Shot](#CranberryEditor) 
* For `TestEngine` start from [TestEngine-Main]. In order to run `TestEngine` additional Assets directory with raw assets are necessary. If you want to run `TestEngine.exe` download and extract [Assets.zip] in Runtime folder

## Features
Many features listed below are supported but tooling still needs to be developed
* Reflection generator for C++
* Reflection supports metadata classes and flags
* Shaders are reflected and Parameters can be addressed with names
* PBR Materials
* Image base lighting
* Point, Spot, Directional lights
* Shadows, Cube mapped shadows and Cascaded shadows
* Prototype garbage collector
* Object tree actions like deep copy, traversal etc.,
* Multiwindow widgets and Input handling
* Supported inputs mouse and keyboard
* World/Actor/Components
* Unity style prefabs
* Job system using [CoPaT]
    
## Third parties
Licenses for third party packages used is placed under `Licenses` folder
### Current dependencies:
* **`Vulkan`** headers
* **`glm`**
* **`tinyObjloader`**
* **`stb`**
* **`LLVM`**(**`CLang`**). CMake configs generated required some manual tweaking to support Debug,Development and Release config
* **`mimalloc`**. CMake configs generated required some manual tweaking to support Debug,Development and Release config
* **`tracy`** profiler. CMake configs generated required some manual tweaking project to support Debug,Development and Release config
* **`xxHash`** (Embedded into the project)
* **`CoPaT`** (Embedded into the project)
* **`SPIRVCross`** (Few necessary files are copied directly rather than using as include)
* **`ImGui`** and **`ImPlot`** (Embedded into the project)

## Some screen shots

### TestEngine
![TestEngineSS]

### CranberryEditor
![CranberryEdSS]

## Project boards
* [Cranberry Board-Trello] - This board will used for new features and research.
* [Cranberry Board-Github] - This board is for issues and bug fixes.

## PS
If you found any piece of this software helpful or used it yourself, Please feel free to share it with your circle. I had invested substantial amount of my personal time in this project and would love some feedback in return😄


[//]: # (Below are link reference definitions)
[CMake]: https://cmake.org/download/
[VisualStudio]: https://visualstudio.microsoft.com/downloads/
[VulkanSDK]: https://vulkan.lunarg.com/
[CoPaT]: https://github.com/jeslaspravin/CoPaT
[Assets.zip]: https://drive.google.com/file/d/1UenyueBcAzApXHIep02ZaDYXuvtzqNjP/view?usp=sharing

[Cranberry-Main]: https://github.com/jeslaspravin/Cranberry/blob/main/Source/Runtime/EngineModules/Cranberry/Private/EngineMain.cpp#L22
[TestEngine-Main]: https://github.com/jeslaspravin/Cranberry/blob/main/Source/Runtime/ExampleModules/TestEngine/Private/StartMain.cpp#L44
[TestEngineSS]: https://jeslaspravin.github.io/assets/images/CranberryEngine/TestEngine(08-01-2023).jpg
[CranberryEdSS]: https://jeslaspravin.github.io/assets/images/CranberryEngine/CranberryEngine(08-01-2023).jpg
[CranberryAppLoadScene]: https://github.com/jeslaspravin/Cranberry/blob/main/Source/Runtime/EngineModules/Cranberry/Private/CranberryEngineApp.cpp#L165
[Cranberry Board-Trello]: https://trello.com/b/ZvopPmvj
[Cranberry Board-Github]: https://github.com/users/jeslaspravin/projects/4 