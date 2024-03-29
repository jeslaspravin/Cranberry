include(EngineProjectMacros)
include(EngineCMakeHelpers)

# Since Clang creates more targets
get_filename_component(directory_name ${CMAKE_CURRENT_LIST_DIR} NAME)

# Finding LLVM libraries
# Set folder so that llvm generated targets stays separated
set (cmake_folder_backup ${CMAKE_FOLDER})
set (CMAKE_FOLDER XGenTgts/LLVM)
make_find_package_hints(LLVM ${Cranberry_LLVM_INSTALL_PATH} llvm_hints)
find_package(LLVM 17
    REQUIRED CONFIG
    HINTS ${llvm_hints})    
if (DEFINED LLVM_FOUND AND ${LLVM_FOUND})
    message ("-- LLVM found dir ${LLVM_INSTALL_PREFIX}")
else ()
    message (FATAL_ERROR "LLVM package not found, Make sure to configure \"Cranberry_LLVM_INSTALL_PATH\" config CACHE with path to llvm install")
endif ()
# Finding Clang libraries
make_find_package_hints(CLang ${Cranberry_LLVM_INSTALL_PATH} clang_hints)
find_package(Clang
    REQUIRED CONFIG
    HINTS ${clang_hints})
if (DEFINED Clang_FOUND AND ${Clang_FOUND})
    message ("-- Clang include dir ${CLANG_INCLUDE_DIRS}")
else ()
    message (FATAL_ERROR "Clang package not found, Make sure to configure \"Cranberry_LLVM_INSTALL_PATH\" config CACHE with path to llvm install")
endif ()
# Reset folder
set (CMAKE_FOLDER ${cmake_folder_backup})

map_imported_targets_config(TARGETS libclang 
    # clangAST
    # Below are libs clangAST pulls
    # LLVMSupport LLVMCore LLVMBinaryFormat LLVMBitstreamReader LLVMFrontendOpenMP LLVMRemarks
    # clangBasic clangLex
)
set(private_libraries
    libclang
    # clangAST # Contains implementations for AST, Necessary to implement some custom parse function that are not exposed in libclang
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
target_compile_options(${target_name} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MP>)
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