include(EngineProjectMacros)
include(GeneratedReflectionsMacros)

set(private_modules
    ProgramCore
    CoreObjects
    EngineCore
)

set(public_modules 
)

set (interface_modules
)

generate_engine_editor_library()
target_compile_options(${target_name} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MP>)

generate_reflection()