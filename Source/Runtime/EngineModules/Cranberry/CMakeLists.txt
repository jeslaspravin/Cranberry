include(EngineProjectMacros)

set(private_modules
    ProgramCore
    EngineRenderer
    Application
    CoreObjects
    EngineCore
)
if (${engine_static_modules})
    if(${WIN32} OR ${LINUX})
        set (private_modules ${private_modules} VulkanRHI)
    elseif(${APPLE})        
        message(FATAL_ERROR "Platform not supported!")
    endif()
endif ()

if (${editor_build})
generate_cpp_console_project(TARGET_NAME ${CMAKE_PROJECT_NAME}Editor)
else ()
generate_cpp_console_project(TARGET_NAME ${CMAKE_PROJECT_NAME})
endif ()

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
