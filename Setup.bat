@echo off

WHERE cmake >nul 2>nul

IF %ERRORLEVEL% EQU 0 (
    ECHO cmake -P %~dp0External/CMakeLists.txt
    cmake -P %~dp0External/CMakeLists.txt
) ELSE (
    ECHO "cmake not found, Make sure cmake is included in paths"
    pause
)
