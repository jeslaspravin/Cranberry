include(${cmake_script_dir}/EngineProjectMacros.cmake)

# Since Clang creates more targets
get_filename_component(directory_name ${CMAKE_CURRENT_LIST_DIR} NAME)
set (CMAKE_FOLDER Tools/${directory_name})

# Finding LLVM libraries
find_package(LLVM 13
    REQUIRED CONFIG
    HINTS ${llvm_install_path}/lib/cmake)    
if (DEFINED LLVM_FOUND AND ${LLVM_FOUND})
    message ("-- LLVM found dir ${LLVM_INSTALL_PREFIX}")
else ()
    message (FATAL_ERROR "LLVM package not found, Make sure to configure \"llvm_install_path\" config CACHE with path to llvm install")
endif ()
# Finding Clang libraries
find_package(Clang
    REQUIRED CONFIG
    HINTS ${llvm_install_path}/lib/cmake)
if (DEFINED Clang_FOUND AND ${Clang_FOUND})
    message ("-- Clang include dir ${CLANG_INCLUDE_DIRS}")
else ()
    message (FATAL_ERROR "Clang package not found, Make sure to configure \"llvm_install_path\" config CACHE with path to llvm install")
endif ()

set(private_libraries
    libclang
)
set(private_includes
    ${CLANG_INCLUDE_DIRS}
)
set(private_modules
    ProgramCore
    ReflectionRuntime
)

generate_cpp_console_project()

set(delay_load_dlls
    libclang
)
target_compile_definitions(${target_name} 
    PRIVATE
        LLVM_INSTALL_PATH="${LLVM_INSTALL_PREFIX}"
)
mark_delay_loaded_dlls()
# we delay load here
#copy_target_exe_runtime_deps(${target_name})

# Add mustache templates to sources list
file (GLOB_RECURSE mustache_templates 
    LIST_DIRECTORIES false
    RELATIVE ${CMAKE_CURRENT_LIST_DIR}
    CONFIGURE_DEPENDS
    Templates/*.[Mm]ustache)
source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${mustache_templates})
target_sources(${target_name}
    PRIVATE
        ${mustache_templates}
)