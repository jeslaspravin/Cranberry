include(EngineProjectMacros)
include(EngineCMakeHelpers)

set(private_libraries )
set(public_libraries )
set(private_includes )
set(public_includes )
set(delay_load_dlls )

set (private_compile_defs )
set (public_compile_defs )

# Finding Mimalloc libraries
if (${Cranberry_ENABLE_MIMALLOC})
    set(mimalloc_lib_name mimalloc-static)
    # Manually set the Debug libraries path in mimalloc-debug.cmake file, Since mimalloc is not properly configured for static library in multi config mode
    # Also added development config to mimalloc cmake configs
    # Maybe I can move it to dll use once module manage is ready with malloc or may I can directly use platform library loader skipping module manager
    make_find_package_hints(mimalloc ${Cranberry_MIMALLOC_INSTALL_PATH} mimalloc_hints)
    find_package(mimalloc 2.1
        REQUIRED CONFIG
        HINTS ${mimalloc_hints}
    )
    map_imported_targets_config(TARGETS ${mimalloc_lib_name})
    
    get_target_property(lib_path ${mimalloc_lib_name} IMPORTED_LOCATION_RELEASE)
    get_target_property(dbg_lib_path ${mimalloc_lib_name} IMPORTED_LOCATION_DEBUG)
    if (DEFINED mimalloc_FOUND AND ${mimalloc_FOUND} AND DEFINED MIMALLOC_INCLUDE_DIR)
        message ("-- mimalloc include dir ${MIMALLOC_INCLUDE_DIR} release location ${lib_path} debug location ${dbg_lib_path}")
    else ()
        message (FATAL_ERROR "mimalloc package not found, Make sure to configure \"Cranberry_MIMALLOC_INSTALL_PATH\" config CACHE with path to mimalloc install")
    endif ()


    list (APPEND private_libraries ${mimalloc_lib_name})
    list (APPEND private_includes ${MIMALLOC_INCLUDE_DIR})
    list (APPEND private_compile_defs USE_MIMALLOC=1)
endif (${Cranberry_ENABLE_MIMALLOC})

# Finding Tracy libraries
if (${Cranberry_ENABLE_TRACY})    
    find_package(Threads REQUIRED)

    set(tracyclient_lib_name Tracy::TracyClient)    

    make_find_package_hints(Tracy ${Cranberry_TRACY_INSTALL_PATH} tracy_hints)
    find_package(Tracy
        REQUIRED CONFIG
        HINTS ${tracy_hints}
    )
    map_imported_targets_config(TARGETS ${tracyclient_lib_name})
    
    get_target_property(tracyclient_incl_path ${tracyclient_lib_name} INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(lib_path ${tracyclient_lib_name} IMPORTED_LOCATION_RELEASE)
    get_target_property(dbg_lib_path ${tracyclient_lib_name} IMPORTED_LOCATION_DEBUG)
    if (DEFINED Tracy_FOUND AND ${Tracy_FOUND})
        message ("-- Tracy release location ${lib_path} debug location ${dbg_lib_path}, Include path ${tracyclient_incl_path}")
    else ()
        message (FATAL_ERROR "Tracy package not found, Make sure to configure \"Cranberry_TRACY_INSTALL_PATH\" config CACHE with path to tracy cmake install")
    endif ()

    list (APPEND private_libraries ${tracyclient_lib_name})
    list (APPEND public_includes ${tracyclient_incl_path})
    list (APPEND public_compile_defs
        USE_TRACY_PROFILER
        HAS_ANY_PROFILER=1
    )
    if (${Cranberry_ENABLE_SECURE_PROFILING})
        list (APPEND private_compile_defs
            SECURE_PROFILING
        )   
    endif (${Cranberry_ENABLE_SECURE_PROFILING})

    if (${Cranberry_ENABLE_MEMORY_PROFILING})
        list (APPEND private_compile_defs
            ENABLE_MEMORY_PROFILING
        )   
    endif (${Cranberry_ENABLE_MEMORY_PROFILING})

    # Using static library since there are exported global variables and causes issue with delay loading
    # list (APPEND private_compile_defs TRACYCLIENT_LIB_PATH=$<TARGET_FILE:${tracyclient_lib_name}>)
    # get_filename_component(tracyclient_file_name ${lib_path} NAME_WLE)
    # list (APPEND delay_load_dlls ${tracyclient_file_name})    
endif (${Cranberry_ENABLE_TRACY})

# add glm
list (APPEND public_includes ${Cranberry_CPP_LIBS_PATH}/glm)

generate_engine_library()
target_compile_options(${target_name} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MP>)
# We do all non common but engine common defines once here as this is the module which probably will get added to every engine related projects/targets
target_compile_definitions(${target_name}
    PUBLIC 
        ${engine_def} 
        ${Cranberry_EXPERIMENTAL_def} 
        ${public_compile_defs}
        $<$<BOOL:${Cranberry_ENABLE_CONSOLE_LOG}>:LOG_TO_CONSOLE=1>
        $<$<BOOL:${Cranberry_ENABLE_VERBOSE_LOG}>:ENABLE_VERBOSE_LOG=1>
    PRIVATE
        ${private_compile_defs}
)
# Copying to other directories like Editor and Tools, In dynamic libraries
if (NOT ${Cranberry_STATIC_MODULES})
    get_engine_root($<TARGET_FILE_DIR:${target_name}> engine_root)
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${target_name}> ${engine_root}/Tools
        COMMAND_EXPAND_LISTS
    )
endif (NOT ${Cranberry_STATIC_MODULES})


# it is okay here as this will be base of all module and so no other modules exists yet
mark_delay_loaded_dlls(IGNORE_MODULES)
