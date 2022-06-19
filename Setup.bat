@echo off

echo cmake -P %~dp0External/CMakeLists.txt
cmake -P %~dp0External/CMakeLists.txt