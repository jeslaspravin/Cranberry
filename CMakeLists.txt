cmake_minimum_required(VERSION 3.22)

project (MyArdentGameEngine VERSION 0.0.1 LANGUAGES C CXX CSharp)
include(Scripts/CMake/EngineProjectVariable.cmake)
include(Scripts/CMake/EngineFileUtilities.cmake)
add_subdirectory(Source)
add_subdirectory(Scripts)
