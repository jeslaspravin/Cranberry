# 
# \file GlobalConfig.cmake
# 
# \author Jeslas Pravin
# \date February 2022
# \copyright
#  Copyright (C) Jeslas Pravin, Since 2022
#  @jeslaspravin pravinjeslas@gmail.com
#  License can be read in LICENSE file at this repository's root
# 

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

