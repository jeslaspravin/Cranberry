include(${cmake_script_dir}/EngineProjectMacros.cmake)
include(${cmake_script_dir}/EngineFileUtilities.cmake)

# Since Clang creates more targets
get_filename_component(directory_name ${CMAKE_CURRENT_LIST_DIR} NAME)
set (CMAKE_FOLDER Tools/${directory_name})

# Finding LLVM libraries
make_find_package_hints(LLVM ${llvm_install_path} llvm_hints)
find_package(LLVM 14
    REQUIRED CONFIG
    HINTS ${llvm_hints})    
if (DEFINED LLVM_FOUND AND ${LLVM_FOUND})
    message ("-- LLVM found dir ${LLVM_INSTALL_PREFIX}")
else ()
    message (FATAL_ERROR "LLVM package not found, Make sure to configure \"llvm_install_path\" config CACHE with path to llvm install")
endif ()
# Finding Clang libraries
make_find_package_hints(CLang ${llvm_install_path} clang_hints)
find_package(Clang
    REQUIRED CONFIG
    HINTS ${clang_hints})
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
        TEMPLATES_DIR="${CMAKE_CURRENT_LIST_DIR}/Templates"
)
mark_delay_loaded_dlls()
# we delay load here
#copy_target_exe_runtime_deps(${target_name})

# Add mustache templates to sources list
file (GLOB_RECURSE mustache_templates 
    LIST_DIRECTORIES false
    RELATIVE ${CMAKE_CURRENT_LIST_DIR}
    Templates/*.[Mm]ustache)
source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${mustache_templates})
target_sources(${target_name}
    PRIVATE
        ${mustache_templates}
)