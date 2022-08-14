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
    set (editor_modules 
        EditorCore
        CBEEditor
    )

    list(APPEND private_modules ${editor_modules})
    generate_cpp_console_project(TARGET_NAME ${CMAKE_PROJECT_NAME}Editor)

    # Mark all editor_modules as delay loaded
    set (delay_load_dlls ${editor_modules})
    mark_delay_loaded_dlls(IGNORE_MODULES)
    
else (${editor_build})
    generate_cpp_console_project(TARGET_NAME ${CMAKE_PROJECT_NAME})
endif (${editor_build})

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
