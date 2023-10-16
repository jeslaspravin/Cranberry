include(EngineProjectMacros)

set(private_modules
    ProgramCore
)

generate_cpp_console_project()

target_compile_options(${target_name} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MP>)
