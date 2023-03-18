include(EngineProjectMacros)
generate_cpp_static_lib()
# This is shared module
set_target_properties(${target_name} PROPERTIES 
    FOLDER Shared)