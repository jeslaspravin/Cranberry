# 
# \file GlobalConfig.cmake
# 
# \author Jeslas Pravin
# \date February 2022
# \copyright
#  Copyright (C) Jeslas Pravin, 2022-2023
#  @jeslaspravin pravinjeslas@gmail.com
#  License can be read in LICENSE file at this repository's root
# 

file(REAL_PATH "Scripts/CMake" cmake_script_dir BASE_DIRECTORY ${PROJECT_SOURCE_DIR})
# Setup module lookup directories, So that instead of including like include(${cmake_script_dir}/EngineFileUtilities.cmake) we can just do include(EngineFileUtilities)
list(APPEND CMAKE_MODULE_PATH ${cmake_script_dir})

# Make sure we are targetting 64 bit
if (NOT ${CMAKE_SIZEOF_VOID_P} EQUAL 8)    
    message(FATAL_ERROR "Only 64bit platform is supported!")
endif ()

# Setup project's configurations supported https://gitlab.kitware.com/cmake/community/-/wikis/FAQ#how-can-i-specify-my-own-configurations-for-generators-that-allow-it-
set(CMAKE_CONFIGURATION_TYPES Debug Development Release RelWithDebInfo)
set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING
    "Additional configuration if needed"
    FORCE)
# Set compiler flags
set(CMAKE_CXX_FLAGS_DEVELOPMENT ${CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT})
string(REPLACE "NDEBUG" "DEVELOPMENT" CMAKE_CXX_FLAGS_DEVELOPMENT ${CMAKE_CXX_FLAGS_DEVELOPMENT})
string(REPLACE "O2" "O1" CMAKE_CXX_FLAGS_DEVELOPMENT ${CMAKE_CXX_FLAGS_DEVELOPMENT})
set(CMAKE_CXX_FLAGS_DEVELOPMENT ${CMAKE_CXX_FLAGS_DEVELOPMENT} CACHE STRING 
    "CPP flags for Development configuration")
# set linker flags for exe, modules, static and shared libs
set(CMAKE_EXE_LINKER_FLAGS_DEVELOPMENT ${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO_INIT} CACHE STRING
    "CPP exe linker flags for Development configuration")
set(CMAKE_MODULE_LINKER_FLAGS_DEVELOPMENT ${CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO_INIT} CACHE STRING
    "CPP module linker flags for Development configuration")
set(CMAKE_STATIC_LINKER_FLAGS_DEVELOPMENT ${CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO_INIT} CACHE STRING
    "CPP static library linker flags for Development configuration")
set(CMAKE_SHARED_LINKER_FLAGS_DEVELOPMENT ${CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO_INIT} CACHE STRING
    "CPP exe shared library flags for Development configuration")

# Global compile and link options
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_C_STANDARD 17)

# Not using since mimalloc does not work
# add_compile_options(-fsanitize=address)

# POD/Variables in class has to initialized with {} to zero initialize if calling constructors that are not compiler generated
# add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/sdl>)

# Strictly conform to standard    
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/permissive->)

# Enable all warnings as errors
add_compile_options(
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<CXX_COMPILER_ID:MSVC>:/WX>
)
add_compile_options(
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:-Wall>
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:-Wextra>
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:-Wshadow>
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:-Wconversion>
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:-Wpedantic>
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:-Werror>
)

# Disabling unnecessary warnings
add_compile_options(
    $<$<CXX_COMPILER_ID:MSVC>:/wd4251> # dll-interface issues due to type size mismatch between dlls compiled from different compilers
    $<$<CXX_COMPILER_ID:MSVC>:/wd4275> # dll-interface data corruption due to accessing static variables which will be different across dll boundaries
    $<$<CXX_COMPILER_ID:MSVC>:/wd4324> # Warns using alignas to align structs
)