set "cwd=%cd%"
cd ../../Binaries/GameBuilder
GameBuilder.exe -mode shaderCompile -targetPath "D:/Workspaces/Visual Studio 2017/GameEngine/Binaries/EngineMain/x64/Debug" -api vulkan
GameBuilder.exe -mode shaderCompile -targetPath "D:/Workspaces/Visual Studio 2017/GameEngine/Binaries/EngineMain/x64/Release" -api vulkan
cd %cwd%