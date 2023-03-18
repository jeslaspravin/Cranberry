include(EngineProjectMacros)

set (private_modules    
    ProgramCore
)

set (private_libraries
    EngineShaderData
)
set (interface_libraries
    EngineShaderData
)

# EngineShaderData is general library so we need to manually include to propagate transitive includes and compile definitions
set(public_includes
    ${Cranberry_CPP_LIBS_PATH}/glm
    $<TARGET_PROPERTY:EngineShaderData,INTERFACE_INCLUDE_DIRECTORIES>
)

generate_engine_library()
target_compile_options(${target_name} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MP>)
# target_compile_options(${target_name} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/showIncludes>)