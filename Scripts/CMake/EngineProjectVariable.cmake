# 
# \file EngineProjectVariable.cmake
# 
# \author Jeslas Pravin
# \date January 2022
# \copyright
#  Copyright (C) Jeslas Pravin, Since 2022
#  @jeslaspravin pravinjeslas@gmail.com
#  License can be read in LICENSE file at this repository's root
# 

# Included at root CMakeLists.txt. DO NOT include it in any internal List files

include (StringUtilities)

##### ----CONFIG options start---- #####
# Setting Global properties
option (Cranberry_NATIVE_MAIN "Whether to use native main function as application entry?" ON)
option (Cranberry_EXPERIMENTAL "Defines EXPERIMENTAL macro for Engine C++ modules" ON)
# option (multithread "Whether to do multi threaded compilation" ON)
# Right now editor build is only supported
option (Cranberry_EDITOR_BUILD "Should be compiled as editor(Enables all editor modules)?" ON)
option (Cranberry_STATIC_MODULES "Should compile and link engine modules statically?" OFF)
option (Cranberry_ENABLE_CONSOLE_LOG "Should log write to console as well?" ON)
option (Cranberry_ENABLE_VERBOSE_LOG "Should write verbose log to log file?" OFF)

set (Cranberry_VULKAN_SDK_PATH $ENV{VULKAN_SDK} CACHE PATH "Vulkan SDK path")

# Setup.bat will populate this directory
set (Cranberry_CPP_LIBS_PATH ${CMAKE_SOURCE_DIR}/External CACHE PATH "Path to CPP libraries")
# LLVM installed path
set (Cranberry_LLVM_INSTALL_PATH ${Cranberry_CPP_LIBS_PATH}/llvm CACHE PATH "LLVM installed path(For libclang)")

option (Cranberry_ENABLE_MIMALLOC "Compile with mimalloc?" ON)
set (Cranberry_MIMALLOC_INSTALL_PATH ${Cranberry_CPP_LIBS_PATH}/mimalloc CACHE PATH "mimalloc installed path")

# Profiler options
option (Cranberry_ENABLE_SECURE_PROFILING "Should the profiler availability needs to be check every time?" OFF)
option (Cranberry_ENABLE_MEMORY_PROFILING "Should the memory allocations needs to be tracked?" OFF)
option (Cranberry_ENABLE_TRACY "Should use tracy for profiling?" ON)
set (Cranberry_TRACY_INSTALL_PATH ${Cranberry_CPP_LIBS_PATH}/tracy CACHE PATH "tracy installed path")

option (Cranberry_ENABLE_CLANG_FORMAT "If enabled, Creates clang formatting sources project" ON)

##### ----CONFIG options end---- #####

include (TestBigEndian)
TEST_BIG_ENDIAN(is_big_endian)

# Relative to target binary directory
set (target_generated_path Generated)
set (Cranberry_EXPERIMENTAL_def $<IF:$<BOOL:${Cranberry_EXPERIMENTAL}>,EXPERIMENTAL=1,EXPERIMENTAL=0>)
set (engine_def RENDERAPI_VULKAN=1 ENGINE_VERSION=0 ENGINE_MINOR_VERSION=1 ENGINE_PATCH_VERSION=0 
    ENGINE_NAME=${CMAKE_PROJECT_NAME}
    $<$<BOOL:${Cranberry_EDITOR_BUILD}>:EDITOR_BUILD=1>
    $<IF:${is_big_endian},BIG_ENDIAN=1,LITTLE_ENDIAN=1>
    )
set (configure_file_folder ConfigureFiles)

# Platform related, We define platforms but make them boolean for use with generator expressions easily, UNIX will be skipped instead define more specialized platforms like LINUX
set (all_platform_folders "Windows" "Linux" "Apple")
make_match_any_pattern_from_list(LIST ${all_platform_folders} OUT_PATTERN match_any_platform_folder)
if (DEFINED UNIX AND NOT DEFINED APPLE)
    set (LINUX 1)
    set (platform_folder "Linux")
    message(FATAL_ERROR "Linux Platform not supported!")
else ()
    set (LINUX 0)
endif ()
if (DEFINED APPLE)
    set (platform_folder "Apple")
    message(FATAL_ERROR "Apple Platform not supported!")
else ()
    set (APPLE 0)
endif ()
if (DEFINED WIN32)
    set (platform_folder "Windows")
else ()
    set (WIN32 0)
endif ()

# Setting all Runtime(exe, dll, so) to project binary directory by default
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
# Group projects under folders for easy access
set_property(GLOBAL PROPERTY USE_FOLDERS ON)