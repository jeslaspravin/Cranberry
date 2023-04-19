include(EngineProjectMacros)

set(private_modules
    ProgramCore
    EngineRenderer
    Application
    CoreObjects
)
if (${Cranberry_STATIC_MODULES})
    if(${WIN32} OR ${LINUX})
        set (private_modules ${private_modules} VulkanRHI)
    elseif(${APPLE})        
        message(FATAL_ERROR "Platform not supported!")
    endif()
endif ()

set(private_includes
    ${Cranberry_CPP_LIBS_PATH}/tinyobjloader
    ${Cranberry_CPP_LIBS_PATH}/stb
)

generate_cpp_console_project()

target_compile_options(${target_name} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MP>)

set_target_properties(${target_name} 
    PROPERTIES 
        WIN32_EXECUTABLE ${Cranberry_NATIVE_MAIN}
)

# To update below project when outdated
add_dependencies(${target_name} EngineShaders)
if (WIN32 OR LINUX)
    add_dependencies(${target_name} VulkanRHI)
endif(WIN32 OR LINUX)
