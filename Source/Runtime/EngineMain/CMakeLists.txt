include(${cmake_script_dir}/EngineProjectMacros.cmake)

set(private_dependencies
    ProgramCore
    EngineCore
    EngineRenderer
    EngineInputCore
    EnginePlatformCore
)

set(private_includes
    ${cpp_libs_path}/tinyobjloader
    ${cpp_libs_path}/stb
)

generate_cpp_console_project()

target_compile_options(${target_name} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MP>)
target_compile_definitions(${target_name} PRIVATE ${engine_def} ${experimental_def})
set_target_properties(${target_name} 
    PROPERTIES 
        WIN32_EXECUTABLE ${native_main}
)