cmake_minimum_required(VERSION 3.22)

set (project_name CranberryEngine)
project (${project_name} VERSION 0.0.1 LANGUAGES C CXX CSharp)
include(Scripts/CMake/EngineProjectVariable.cmake)
include(Scripts/CMake/EngineFileUtilities.cmake)
add_subdirectory(Source)
