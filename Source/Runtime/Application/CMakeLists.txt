include(${cmake_script_dir}/EngineProjectMacros.cmake)

set(private_modules
    ProgramCore
    EngineRenderer
)

set(public_modules    
)

set (interface_modules
)

set(private_includes
    ${cpp_libs_path}/stb
)

generate_engine_library()
target_compile_options(${target_name} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MP>)