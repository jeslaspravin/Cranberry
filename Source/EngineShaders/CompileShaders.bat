@echo off
:: Store current directory
echo CWD : %cd%
:: cd to script drive + path
echo SCRIPT : %~dp0
pushd %~dp0

:: Abs path from rel path to current dir
CALL :absPath "../../Binaries"
set binariesPath=%retVal%
echo BIN : %binariesPath%
popd
pushd %binariesPath%\GameBuilder

echo EXE : GameBuilder.exe -mode shaderCompile -targetPath "%binariesPath%/EngineMain/x64/Debug" -intermediatePath "%binariesPath%/EngineMain/x64/Debug/Intermediate" -api vulkan
GameBuilder.exe -mode shaderCompile -targetPath "%binariesPath%/EngineMain/x64/Debug" -intermediatePath "%binariesPath%/EngineMain/x64/Debug/Intermediate" -api vulkan

echo EXE : GameBuilder.exe -mode shaderCompile -targetPath "%binariesPath%/EngineMain/x64/Release" -intermediatePath "%binariesPath%/EngineMain/x64/Release/Intermediate" -api vulkan
GameBuilder.exe -mode shaderCompile -targetPath "%binariesPath%/EngineMain/x64/Release" -intermediatePath "%binariesPath%/EngineMain/x64/Release/Intermediate" -api vulkan

rem Just copying as there is not change btw non experimental and experimental right now
rmdir /s /q "%binariesPath%/EngineMain/x64/ReleaseExperimental/Shaders"
xcopy /s /i /f "%binariesPath%/EngineMain/x64/Release/Shaders" "%binariesPath%/EngineMain/x64/ReleaseExperimental/Shaders"
rem echo EXE : GameBuilder.exe -mode shaderCompile -targetPath "%binariesPath%/EngineMain/x64/ReleaseExperimental" -intermediatePath "%binariesPath%/EngineMain/x64/ReleaseExperimental/Intermediate" -api vulkan
rem GameBuilder.exe -mode shaderCompile -targetPath "%binariesPath%/EngineMain/x64/ReleaseExperimental" -intermediatePath "%binariesPath%/EngineMain/x64/ReleaseExperimental/Intermediate" -api vulkan

rem Just copying as there is not change btw non experimental and experimental right now
rmdir /s /q "%binariesPath%/EngineMain/x64/DebugExperimental/Shaders"
xcopy /s /i /f "%binariesPath%/EngineMain/x64/Debug/Shaders" "%binariesPath%/EngineMain/x64/DebugExperimental/Shaders"
rem echo EXE : GameBuilder.exe -mode shaderCompile -targetPath "%binariesPath%/EngineMain/x64/DebugExperimental" -intermediatePath "%binariesPath%/EngineMain/x64/DebugExperimental/Intermediate" -api vulkan
rem GameBuilder.exe -mode shaderCompile -targetPath "%binariesPath%/EngineMain/x64/DebugExperimental" -intermediatePath "%binariesPath%/EngineMain/x64/DebugExperimental/Intermediate" -api vulkan

popd

EXIT /B

:absPath
	SET retVal=%~f1
	EXIT /B