include(EngineProjectMacros)

get_filename_component(target_name ${CMAKE_CURRENT_LIST_DIR} NAME)
file(GLOB_RECURSE src_files LIST_DIRECTORIES false 
    RELATIVE ${CMAKE_CURRENT_LIST_DIR}
    *)
source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${src_files})

set (shader_output_dir "${PROJECT_BINARY_DIR}/$<CONFIG>/Runtime")
shader_outputs(
    SOURCES ${src_files} 
    OUTPUT_DIR ${shader_output_dir} 
    OUTPUTS compiled_outputs)
add_custom_command(OUTPUT ${compiled_outputs}
    DEPENDS GameBuilder SpirvShaderReflection ${src_files}
    COMMAND GameBuilder 
        -mode shaderCompile 
        -targetPath "\"${shader_output_dir}\"" 
        -intermediatePath "\"${PROJECT_BINARY_DIR}/Intermediate/$<CONFIG>\"" 
        -api vulkan 
        -glsl-version 450
    WORKING_DIRECTORY $<TARGET_FILE_DIR:GameBuilder>
    USES_TERMINAL
    COMMAND_EXPAND_LISTS
    )
add_custom_target(${target_name} ALL
    SOURCES ${src_files}
    DEPENDS ${compiled_outputs}
)