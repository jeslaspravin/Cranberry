@echo off
:: Store current directory
set cwd=%cd%
echo CWD : %cwd%
:: cd to script drive + path
echo SCRIPT : %~dp0
cd %~dp0

:: Abs path from rel path to current dir
CALL :absPath "../../Binaries"
set binariesPath=%retVal%
echo BIN : %binariesPath%
cd %binariesPath%/GameBuilder

echo EXE : GameBuilder.exe -mode shaderCompile -targetPath "%binariesPath%/EngineMain/x64/Debug" -intermediatePath "%binariesPath%/EngineMain/x64/Debug/Intermediate" -api vulkan
GameBuilder.exe -mode shaderCompile -targetPath "%binariesPath%/EngineMain/x64/Debug" -intermediatePath "%binariesPath%/EngineMain/x64/Debug/Intermediate" -api vulkan

echo EXE : GameBuilder.exe -mode shaderCompile -targetPath "%binariesPath%/EngineMain/x64/Release" -intermediatePath "%binariesPath%/EngineMain/x64/Release/Intermediate" -api vulkan
GameBuilder.exe -mode shaderCompile -targetPath "%binariesPath%/EngineMain/x64/Release" -intermediatePath "%binariesPath%/EngineMain/x64/Release/Intermediate" -api vulkan

cd %cwd%

EXIT /B

:absPath
	SET retVal=%~f1
	EXIT /B