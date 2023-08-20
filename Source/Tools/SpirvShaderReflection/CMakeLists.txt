include(EngineProjectMacros)

set(private_libraries
    EngineShaderData
)
set(private_includes
    ${Cranberry_VULKAN_SDK_PATH}/include   
)

generate_cpp_console_project()