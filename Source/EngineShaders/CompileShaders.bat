set "cwd=%cd%"
cd ../../Binaries/GameBuilder
GameBuilder.exe -mode shaderCompile -targetPath "D:/Workspaces/Visual Studio 2017/GameEngine/Binaries/EngineMain/x64/Debug" -intermediatePath "D:/Workspaces/Visual Studio 2017/GameEngine/Binaries/EngineMain/x64/Debug/Intermediate" -api vulkan
GameBuilder.exe -mode shaderCompile -targetPath "D:/Workspaces/Visual Studio 2017/GameEngine/Binaries/EngineMain/x64/Release" -intermediatePath "D:/Workspaces/Visual Studio 2017/GameEngine/Binaries/EngineMain/x64/Release/Intermediate" -api vulkan
cd %cwd%