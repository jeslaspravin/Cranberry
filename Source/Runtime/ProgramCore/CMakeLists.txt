include(${cmake_script_dir}/EngineProjectMacros.cmake)

set(private_dependencies
)

set(private_includes
)
set(public_includes
    ${cpp_libs_path}/glm
)

generate_cpp_shared_lib()
target_compile_options(${target_name} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MP>)
target_compile_definitions(${target_name} PRIVATE ${engine_def} ${experimental_def})