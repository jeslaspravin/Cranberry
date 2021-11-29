include(${cmake_script_dir}/EngineProjectMacros.cmake)

set(private_modules
)

set(private_includes
)
set(public_includes
    ${cpp_libs_path}/glm
)

generate_engine_library()
target_compile_options(${target_name} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MP>)
# We do all non common but engine common defines once here as this is the module which probably will get added to every engine related projects/targets
target_compile_definitions(${target_name} PUBLIC 
    ${engine_def} 
    ${experimental_def} 
# Path where all engine modules exists for use with delayed loaded dlls
    ENGINE_MODULES_PATH="$<TARGET_FILE_DIR:${target_name}>"
    $<$<BOOL:${enable_console_log}>:LOG_TO_CONSOLE=1>
)