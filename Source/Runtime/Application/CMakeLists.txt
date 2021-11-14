include(${cmake_script_dir}/EngineProjectMacros.cmake)

set(private_dependencies
    ProgramCore
    EngineRenderer
)

set(public_dependencies    
)

set (interface_dependencies
)

generate_engine_library()
target_compile_options(${target_name} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MP>)