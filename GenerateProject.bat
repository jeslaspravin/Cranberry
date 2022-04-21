@echo off
REM set InstallPath=%~dp0Installed
Rem Replace \ to /
REM set InstallPath=%InstallPath:\=/%
REM echo "%InstallPath%"
REM echo cmake -B StaticBuild -G "Visual Studio 17 2022" -A x64 -Thost=x64 -DCMAKE_INSTALL_PREFIX:STRING=%InstallPath% -Dengine_static_modules:BOOL=ON %*
REM cmake -B StaticBuild -G "Visual Studio 17 2022" -A x64 -Thost=x64 -DCMAKE_INSTALL_PREFIX:STRING=%InstallPath% -Dengine_static_modules:BOOL=ON %*

REM echo cmake -B Build -G "Visual Studio 17 2022" -A x64 -Thost=x64 -DCMAKE_INSTALL_PREFIX:STRING=%InstallPath% -Dengine_static_modules:BOOL=OFF %*
REM cmake -B Build -G "Visual Studio 17 2022" -A x64 -Thost=x64 -DCMAKE_INSTALL_PREFIX:STRING=%InstallPath% -Dengine_static_modules:BOOL=OFF %*

echo cmake --preset %*
cmake --preset %*