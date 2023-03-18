@echo off
REM set InstallPath=%~dp0Installed
Rem Replace \ to /
REM set InstallPath=%InstallPath:\=/%
REM echo "%InstallPath%"
REM echo cmake -B StaticBuild -G "Visual Studio 17 2022" -A x64 -Thost=x64 -DCMAKE_INSTALL_PREFIX:STRING=%InstallPath% -DCranberry_STATIC_MODULES:BOOL=ON %*
REM cmake -B StaticBuild -G "Visual Studio 17 2022" -A x64 -Thost=x64 -DCMAKE_INSTALL_PREFIX:STRING=%InstallPath% -DCranberry_STATIC_MODULES:BOOL=ON %*

REM echo cmake -B Build -G "Visual Studio 17 2022" -A x64 -Thost=x64 -DCMAKE_INSTALL_PREFIX:STRING=%InstallPath% -DCranberry_STATIC_MODULES:BOOL=OFF %*
REM cmake -B Build -G "Visual Studio 17 2022" -A x64 -Thost=x64 -DCMAKE_INSTALL_PREFIX:STRING=%InstallPath% -DCranberry_STATIC_MODULES:BOOL=OFF %*

if "%~1"=="" (
    if NOT EXIST %~dp0/External/SetupSuccess.txt (
        call %~dp0/Setup.bat
    )
    
    echo cmake --preset Editor-DynamicLinked
    cmake --preset Editor-DynamicLinked
) else (
    echo cmake --preset %*
    cmake --preset %*
)