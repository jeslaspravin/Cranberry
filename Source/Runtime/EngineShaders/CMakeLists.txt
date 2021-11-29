include(${cmake_script_dir}/EngineProjectMacros.cmake)

get_filename_component(target_name ${CMAKE_CURRENT_LIST_DIR} NAME)
file(GLOB_RECURSE src_files LIST_DIRECTORIES false 
    RELATIVE ${CMAKE_CURRENT_LIST_DIR}
    *)
source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${src_files})

shader_outputs(
    SOURCES ${src_files} 
    OUTPUT_DIR ${PROJECT_BINARY_DIR} 
    OUTPUTS compiled_outputs)
# Cannot use $<CONFIG> generative expression here
get_target_property(builder_dir GameBuilder RUNTIME_OUTPUT_DIRECTORY)
add_custom_command(OUTPUT ${compiled_outputs}
    DEPENDS GameBuilder SpirvShaderReflection ${src_files}
    COMMAND GameBuilder -mode shaderCompile -targetPath "\"${PROJECT_BINARY_DIR}/$<CONFIG>\"" -intermediatePath "\"${PROJECT_BINARY_DIR}/Intermediate/$<CONFIG>\"" -api vulkan
    WORKING_DIRECTORY ${builder_dir}/$<CONFIG>
    USES_TERMINAL
    COMMAND_EXPAND_LISTS
    )
add_custom_target(${target_name} ALL
    SOURCES ${src_files}
    DEPENDS ${compiled_outputs}
)
# This also goes with engine
set_target_properties(${target_name} PROPERTIES 
    FOLDER ${CMAKE_PROJECT_NAME})