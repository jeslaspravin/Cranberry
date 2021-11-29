# Included at root CMakeLists.txt. DO NOT include it in any internal List files

file(REAL_PATH "Scripts/CMake" cmake_script_dir BASE_DIRECTORY ${PROJECT_SOURCE_DIR})

include (${cmake_script_dir}/StringUtilities.cmake)

# Setting Global properties
option (native_main "Whether to use native main function as application entry?" ON)
option (experimental "Defines EXPERIMENTAL macro for Engine C++ modules" ON)
# option (multithread "Whether to do multi threaded compilation" ON)
option (engine_static_modules "Should compile and link engine modules statically?" OFF)
option (enable_console_log "Should log write to console as well?" ON)

set (vulkan_sdk_path $ENV{VULKAN_SDK} CACHE PATH "Vulkan SDK path")
# TODO(Jeslas) : Change this to automatically resolve/download dependencies
set (cpp_libs_path $ENV{CPP_LIB} CACHE PATH "Path to CPP libraries")
# LLVM installed path
set (llvm_install_path ${cpp_libs_path}/llvm CACHE PATH "LLVM installed path(For libclang)")

# Relative to target binary directory
set (target_generated_path Generated)
set (experimental_def $<IF:$<BOOL:${experimental}>, EXPERIMENTAL=1, EXPERIMENTAL=0>)
set (engine_def RENDERAPI_VULKAN=1 ENGINE_VERSION=0 ENGINE_MINOR_VERSION=1 ENGINE_PATCH_VERSION=0 ENGINE_NAME=${CMAKE_PROJECT_NAME})
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

# Setting CMAKE Variables
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_C_STANDARD 17)
# Setting all Runtime(exe, dll, so) to project binary directory
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
# Group projects under folders for easy access
set_property(GLOBAL PROPERTY USE_FOLDERS ON)