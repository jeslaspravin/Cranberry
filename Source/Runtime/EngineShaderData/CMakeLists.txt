include(${cmake_script_dir}/EngineProjectMacros.cmake)
generate_cpp_static_lib()
# This is shared module
set_target_properties(${target_name} PROPERTIES 
    FOLDER Shared)