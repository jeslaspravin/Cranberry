include(${cmake_script_dir}/EngineProjectMacros.cmake)

set (private_dependencies
    ProgramCore
    EngineRenderer
    Application
)
set (interface_dependencies
)
set (public_dependencies
)

set(private_includes
)
# TODO : Move unnecessary public includes to private
set(public_includes
    ${vulkan_sdk_path}/include/vulkan
    ${cpp_libs_path}/glm
)

generate_engine_library()
target_compile_options(${target_name} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MP>)