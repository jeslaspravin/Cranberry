@echo off
set InstallPath=%~dp0Installed
Rem Replace \ to /
set InstallPath=%InstallPath:\=/%
echo "%InstallPath%"
REM echo cmake -B GeneratedStaticBuild -G "Visual Studio 16 2019 WIN64" -A x64 -DCMAKE_INSTALL_PREFIX:STRING=%InstallPath% -Dengine_static_modules:BOOL=ON
REM cmake -B GeneratedStaticBuild -G "Visual Studio 16 2019" -A x64 -DCMAKE_INSTALL_PREFIX:STRING=%InstallPath% -Dengine_static_modules:BOOL=ON

echo cmake -B Generated -G "Visual Studio 16 2019 WIN64" -A x64 -DCMAKE_INSTALL_PREFIX:STRING=%InstallPath% -Dengine_static_modules:BOOL=OFF
cmake -B Generated -G "Visual Studio 16 2019" -A x64 -DCMAKE_INSTALL_PREFIX:STRING=%InstallPath% -Dengine_static_modules:BOOL=OFF