include(${cmake_script_dir}/EngineProjectMacros.cmake)

set(private_libraries
    EngineShaderData
)
set(private_includes
    ${vulkan_sdk_path}/include/vulkan    
)

generate_cpp_console_project()