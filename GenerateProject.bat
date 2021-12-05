@echo off
set InstallPath=%~dp0Installed
Rem Replace \ to /
set InstallPath=%InstallPath:\=/%
echo "%InstallPath%"
REM echo cmake -B StaticBuild -G "Visual Studio 16 2019" -A x64 -Thost=x64 -DCMAKE_INSTALL_PREFIX:STRING=%InstallPath% -Dengine_static_modules:BOOL=ON %*
REM cmake -B StaticBuild -G "Visual Studio 16 2019" -A x64 -Thost=x64 -DCMAKE_INSTALL_PREFIX:STRING=%InstallPath% -Dengine_static_modules:BOOL=ON %*

echo cmake -B Build -G "Visual Studio 16 2019" -A x64 -Thost=x64 -DCMAKE_INSTALL_PREFIX:STRING=%InstallPath% -Dengine_static_modules:BOOL=OFF %*
cmake -B Build -G "Visual Studio 16 2019" -A x64 -Thost=x64 -DCMAKE_INSTALL_PREFIX:STRING=%InstallPath% -Dengine_static_modules:BOOL=OFF %*