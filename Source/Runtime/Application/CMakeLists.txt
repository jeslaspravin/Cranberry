include(EngineProjectMacros)

set(private_modules
)

set(public_modules 
    ProgramCore   
    EngineRenderer
)

set (interface_modules
)

set(private_includes
    ${Cranberry_CPP_LIBS_PATH}/stb
)

generate_engine_library()
target_compile_options(${target_name} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MP>)