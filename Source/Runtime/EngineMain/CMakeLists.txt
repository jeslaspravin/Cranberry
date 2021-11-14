include(${cmake_script_dir}/EngineProjectMacros.cmake)

set(private_dependencies
    ProgramCore
    EngineRenderer
    Application
    EngineInputCore
)

set(private_includes
    ${cpp_libs_path}/tinyobjloader
    ${cpp_libs_path}/stb
)

generate_cpp_console_project()

target_compile_options(${target_name} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MP>)
set_target_properties(${target_name} 
    PROPERTIES 
        WIN32_EXECUTABLE ${native_main}
)

add_dependencies(${target_name} EngineShaders)
if (WIN32 OR LINUX)
    add_dependencies(${target_name} VulkanRHI)
endif(WIN32 OR LINUX)
