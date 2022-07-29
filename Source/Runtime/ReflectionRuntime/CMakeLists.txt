include(EngineProjectMacros)

set(private_modules
)

set(public_modules 
    ProgramCore   
)

set (interface_modules
)

generate_engine_library()
target_compile_options(${target_name} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MP>)