include(${cmake_script_dir}/EngineProjectMacros.cmake)
include(${cmake_script_dir}/GeneratedReflectionsMacros.cmake)

set(private_modules
    ProgramCore
)

set(public_modules 
    ReflectionRuntime
)

set (interface_modules
)

generate_engine_library()
target_compile_options(${target_name} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MP>)

generate_reflection()