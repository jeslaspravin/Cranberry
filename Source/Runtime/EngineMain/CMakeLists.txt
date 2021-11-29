include(${cmake_script_dir}/EngineProjectMacros.cmake)

set(private_modules
    ProgramCore
    EngineRenderer
    Application
    EngineInputCore
)
if (${engine_static_modules})
    if(${WIN32} OR ${LINUX})
        set (private_modules ${private_modules} VulkanRHI)
    elseif(${APPLE})        
        message(FATAL_ERROR "Platform not supported!")
    endif()
endif ()

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

# To update below project when outdated
add_dependencies(${target_name} EngineShaders)
if (WIN32 OR LINUX)
    add_dependencies(${target_name} VulkanRHI)
endif(WIN32 OR LINUX)
