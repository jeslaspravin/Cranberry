include(EngineProjectMacros)
include(GeneratedReflectionsMacros)

set(private_modules
    ProgramCore
    CoreObjects
    EngineCore
    EditorCore
)

set(public_modules 
)

set (interface_modules
)

set(private_includes
    ${Cranberry_CPP_LIBS_PATH}/tinyobjloader
    ${Cranberry_CPP_LIBS_PATH}/stb
)

generate_engine_editor_library()
target_compile_options(${target_name} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MP>)

generate_reflection()