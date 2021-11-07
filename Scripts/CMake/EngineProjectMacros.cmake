########################################################################################################
# C++ related functions
########################################################################################################

macro (cpp_common_variables)
    # Private dependencies
    list(LENGTH private_dependencies private_dependency_count)
    if(${private_dependency_count} GREATER 0)
        target_link_libraries(${target_name} PRIVATE ${private_dependencies})
    endif()
    # Public dependencies
    list(LENGTH public_dependencies public_dependency_count)
    if(${public_dependency_count} GREATER 0)
        target_link_libraries(${target_name} PRIVATE ${public_dependencies})
    endif()
    # Private includes
    list(LENGTH private_includes private_inl_count)
    if(${private_inl_count} GREATER 0)
        target_include_directories(${target_name} 
                PRIVATE
                    ${private_includes}
        )
    endif()
    # Public includes
    list(LENGTH public_includes public_inl_count)
    if(${public_inl_count} GREATER 0)
        target_include_directories(${target_name} 
            PUBLIC
                ${public_includes}
        )
    endif()

    # POD/Variables in class has to initialized with {} to zero initialize if calling constructors that are not compiler generated
    # target_compile_options(${target_name} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/sdl>)
endmacro ()

macro (generate_cpp_console_project)
    get_filename_component(target_name ${CMAKE_CURRENT_LIST_DIR} NAME)
    get_all_cpp_files(src_files)

    source_group(TREE ${CMAKE_CURRENT_LIST_DIR} PREFIX Sources FILES ${src_files})

    add_executable(${target_name} ${src_files})
    cpp_common_variables()
endmacro()

macro (generate_cpp_static_lib)
    get_filename_component(target_name ${CMAKE_CURRENT_LIST_DIR} NAME)
    get_all_cpp_files(src_files)

    source_group(TREE ${CMAKE_CURRENT_LIST_DIR} PREFIX Sources FILES ${src_files})

    add_library(${target_name} STATIC ${src_files})

    if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/Private)
        list(APPEND private_includes Private)
        list(REMOVE_DUPLICATES private_includes)
    endif()
    if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/Public)
        list(APPEND public_includes Public)
        list(REMOVE_DUPLICATES public_includes)
    endif()
    cpp_common_variables()
endmacro()

########################################################################################################
# C# related functions
########################################################################################################
macro(generate_csharp_console_project)
    include(${cmake_script_dir}/EngineFileUtilities.cmake)

    get_filename_component(target_name ${CMAKE_CURRENT_LIST_DIR} NAME)

    file (GLOB_RECURSE file_list 
        LIST_DIRECTORIES false
        RELATIVE ${CMAKE_CURRENT_LIST_DIR}
        CONFIGURE_DEPENDS
        *)
    add_executable(${target_name}
        ${file_list}
    )

    # Add in the .NET reference libraries. Append to some commonly needed references
    set (all_references 
        Microsoft.CSharp
        System
        System.Core
        System.Data.DataSetExtensions
        System.Data
        System.Xml
        System.Xml.Linq
        System.Net.Http
        ${references})
    # Set the .NET Framework version for the executable.
    set_target_properties(${target_name}
        PROPERTIES
            VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.7.2"
            # If necessary, link in other library/DLL references, such as 3rd party libraries.
            # VS_DOTNET_REFERENCE_MyThirdPartyLib /path/to/libs/MyThirdPartyLib.dll
        )
    
    set_property(TARGET ${target_name} PROPERTY VS_DOTNET_REFERENCES ${all_references})
    # Set the C# language version.
    set(CMAKE_CSharp_FLAGS "/langversion:7.3")

    # If necessary, link in other library dependencies that were built locally in this source tree.
    # target_link_libraries(MyWinFormApp MyLocalLib)
endmacro()

########################################################################################################
# Shader related functions
########################################################################################################

function(shader_outputs)    
    set(one_value_args OUTPUT_DIR OUTPUTS)
    set(multi_value_args SOURCES)
    cmake_parse_arguments(shader_outputs "" "${one_value_args}" "${multi_value_args}" ${ARGN})

    set(unique_shaders )
    foreach(src_file ${shader_outputs_SOURCES})
        # Matching files that ends with .glsl
        if(${src_file} MATCHES ".*[.glsl]$")
            # In order to remove all directories, And expose inner extension
            get_filename_component(src_wo_glsl ${src_file} NAME_WLE)
            # If include file then ignore it
            if(NOT ${src_wo_glsl} MATCHES ".*[.inl]$")
                # Replace any .vert, .frag ... but capturing matches before .extension
                string(REGEX REPLACE "(.*)[.].*" "\\1" src_wo_ext ${src_wo_glsl})
                list(APPEND unique_shaders ${shader_outputs_OUTPUT_DIR}/$<CONFIG>/Shaders/${src_wo_ext}.ref ${shader_outputs_OUTPUT_DIR}/$<CONFIG>/Shaders/${src_wo_ext}.shader)
            endif()
        endif(${src_file} MATCHES ".*[.glsl]$")
    endforeach()
    list(REMOVE_DUPLICATES unique_shaders)
    set(${shader_outputs_OUTPUTS} ${unique_shaders} PARENT_SCOPE)
endfunction()