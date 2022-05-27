include(EngineProjectMacros)

set(private_modules
    ProgramCore
)

set(public_modules    
)

set (interface_modules
)

generate_engine_library()
target_compile_options(${target_name} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MP>)