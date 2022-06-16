# 
# \file EngineCMakeHelpers.cmake
# 
# \author Jeslas Pravin
# \date June 2022
# \copyright
#  Copyright (C) Jeslas Pravin, Since 2022
#  @jeslaspravin pravinjeslas@gmail.com
#  License can be read in LICENSE file at this repository's root
# 

# Appends some cmake searching directories to given path
function(make_find_package_hints package_name base_dir out_hint_paths)
    set(${out_hint_paths} 
        ${base_dir}/lib/cmake/
        ${base_dir}/cmake/
        ${base_dir}/${package_name}/lib/cmake/
        ${base_dir}/${package_name}/cmake/
        PARENT_SCOPE
    )
endfunction()

# Using per target setting instead of CMake variable CMAKE_MAP_IMPORTED_CONFIG_<CONFIG> 
# as in future we might import engine to game project using find_package and we do not want this special case to be a hindrance
function(map_imported_targets_config)
    set(multi_value_args TARGETS)
    cmake_parse_arguments(map_imported_config "" "" "${multi_value_args}" ${ARGN})
    
    foreach (target_name ${map_imported_config_TARGETS})        
        # Debug and Release is same in our code base
        set_target_properties(${target_name} PROPERTIES
            MAP_IMPORTED_CONFIG_DEVELOPMENT Release
            MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
        )
    endforeach()
endfunction()

# Setup all CMAKE_RUNTIME_OUTPUT_DIRECTORY_<CONFIG> to a root_dir/<CONFIG>/rel_path
macro(setup_runtime_output_dir root_dir rel_path)
    foreach (config_name ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${config_name} config)
        set (CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config} "${root_dir}/${config_name}/${rel_path}")
    endforeach()
endmacro()