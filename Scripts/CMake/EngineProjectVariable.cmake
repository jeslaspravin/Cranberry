
# Setting Global properties
option (native_main "Whether to use native main function as application entry?" ON)
option (experimental "Defines EXPERIMENTAL macro for Engine C++ modules" ON)

set (vulkan_sdk_path $ENV{VULKAN_SDK} CACHE PATH "Vulkan SDK path")
# TODO(Jeslas) : Change this to automatically resolve/download dependencies
set (cpp_libs_path $ENV{CPP_LIB} CACHE PATH "Path to CPP libraries")

# Setting CMAKE Variables
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_C_STANDARD 17)
# Setting all Runtime(exe, dll, so) to project binary directory
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

file(REAL_PATH "Scripts/CMake" cmake_script_dir BASE_DIRECTORY ${PROJECT_SOURCE_DIR})