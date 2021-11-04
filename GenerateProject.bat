@echo off
set InstallPath=%~dp0Installed
Rem Replace \ to /
set InstallPath=%InstallPath:\=/%
echo "%InstallPath%"
echo cmake -B Generated -G "Visual Studio 16 2019 WIN64" -A x64 -DCMAKE_INSTALL_PREFIX:STRING=%InstallPath%
cmake -B Generated -G "Visual Studio 16 2019" -A x64 -DCMAKE_INSTALL_PREFIX:STRING=%InstallPath%