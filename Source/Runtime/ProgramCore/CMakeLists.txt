include(EngineProjectMacros)
include(EngineCMakeHelpers)

set(private_libraries )
set(public_libraries )
set(private_includes )
set(public_includes )
set(delay_load_dlls )

set (private_compile_defs )

# Finding Mimalloc libraries
if (${enable_mimalloc})
    set(mimalloc_lib_name mimalloc-static)
    # Manually set the Debug libraries path in mimalloc-debug.cmake file, Since mimalloc is not properly configured for static library in multi config mode
    # Also added development config to mimalloc cmake configs
    # Maybe I can move it to dll use once module manage is ready with malloc or may I can directly use platform library loader skipping module manager
    make_find_package_hints(mimalloc ${mimalloc_install_path} mimalloc_hints)
    find_package(mimalloc 2.0
        REQUIRED CONFIG
        HINTS ${mimalloc_hints} ${mimalloc_debug_hints})
    map_imported_targets_config(TARGETS ${mimalloc_lib_name})
    
    get_target_property(lib_path ${mimalloc_lib_name} IMPORTED_LOCATION_RELEASE)
    get_target_property(dbg_lib_path ${mimalloc_lib_name} IMPORTED_LOCATION_DEBUG)
    if (DEFINED mimalloc_FOUND AND ${mimalloc_FOUND} AND DEFINED MIMALLOC_INCLUDE_DIR)
        message ("-- mimalloc include dir ${MIMALLOC_INCLUDE_DIR} release location ${lib_path} debug location ${dbg_lib_path}")
    else ()
        message (FATAL_ERROR "mimalloc package not found, Make sure to configure \"mimalloc_install_path\" config CACHE with path to mimalloc install")
    endif ()


    list (APPEND private_libraries ${mimalloc_lib_name})
    list (APPEND private_includes ${MIMALLOC_INCLUDE_DIR})
    list (APPEND private_compile_defs USE_MIMALLOC=1)
endif (${enable_mimalloc})

# add glm
list (APPEND public_includes ${cpp_libs_path}/glm)

generate_engine_library()
target_compile_options(${target_name} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MP>)
# We do all non common but engine common defines once here as this is the module which probably will get added to every engine related projects/targets
target_compile_definitions(${target_name}
    PUBLIC 
        ${engine_def} 
        ${experimental_def} 
        $<$<BOOL:${enable_console_log}>:LOG_TO_CONSOLE=1>
        $<$<BOOL:${enable_verbose_log}>:ENABLE_VERBOSE_LOG=1>
    PRIVATE
        ${private_compile_defs}
)
# Copying to other directories like Editor and Tools, In dynamic libraries
if (NOT ${engine_static_modules})
    get_engine_root($<TARGET_FILE_DIR:${target_name}> engine_root)
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${target_name}> ${engine_root}/Tools
        COMMAND_EXPAND_LISTS
    )
endif (NOT ${engine_static_modules})


# it is okay here as this will be base of all module and so no other modules exists yet
mark_delay_loaded_dlls(IGNORE_MODULES)
