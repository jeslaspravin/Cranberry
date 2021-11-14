include(${cmake_script_dir}/EngineProjectMacros.cmake)

set (private_dependencies    
    ProgramCore
    EngineShaderData
)
set (interface_dependencies
    EngineShaderData
)
set (public_dependencies
)

set(private_includes
)
# TODO : Move unnecessary public includes to private
set(public_includes
    ${cpp_libs_path}/glm
)

generate_engine_library()
target_compile_options(${target_name} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MP>)
# target_compile_options(${target_name} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/showIncludes>)