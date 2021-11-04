include(${cmake_script_dir}/EngineProjectMacros.cmake)

set(private_dependencies
    EngineShaderData
)
set(private_includes
    ${vulkan_sdk_path}/include/vulkan
    ${cpp_libs_path}/glm
    ${cpp_libs_path}/tinyobjloader
    ${cpp_libs_path}/stb
)

generate_cpp_console_project()

target_compile_options(${target_name} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MP>)
target_compile_definitions(${target_name} PRIVATE RENDERAPI_VULKAN=1 ENGINE_VERSION=0 ENGINE_MINOR_VERSION=1 $<IF:$<BOOL:${experimental}>, EXPERIMENTAL=1, EXPERIMENTAL=0>)
set_target_properties(${target_name} 
    PROPERTIES 
        WIN32_EXECUTABLE ${native_main}
)